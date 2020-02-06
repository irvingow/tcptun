//
// Created by lwj on 2020/2/3.
//

#include "tcptun_common.h"
#include "tcptun_connection_manager.h"
#include "parse_config.h"
#include <glog/logging.h>
#include <sys/epoll.h>
#include <memory>

using tcptun::ip_port_t;

int32_t run(const std::string &config_file_path) {
    SystemConfig *instance = SystemConfig::GetInstance(config_file_path);
    auto system_config = instance->system_config();
    if (!system_config->parse_flag) {
        LOG(ERROR) << "failed to parse config file";
        return -1;
    }
    const std::string local_ip = system_config->listen_ip;
    const size_t local_port = system_config->listen_port;
    const std::string remote_ip = system_config->remote_ip;
    const size_t remote_port = system_config->remote_port;
    int epoll_fd = -1;
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOG(ERROR) << "failed to call epoll_create1 error:" << strerror(errno);
        return -2;
    }
    int local_listen_fd = -1;
    auto ret = tcptun::new_listen_socket(local_ip, local_port, local_listen_fd);
    if (ret < 0) {
        LOG(ERROR) << "failed to call new_listen_socket local_ip:" << local_ip << " local_port:" << local_port;
        return -3;
    }
    const int32_t maxevent = 64;
    struct epoll_event events[maxevent];
    ret = tcptun::AddEvent2Epoll(epoll_fd, local_listen_fd, EPOLLIN);
    if (ret < 0) {
        close(epoll_fd);
        close(local_listen_fd);
        LOG(ERROR) << "failed to call AddEvent2Epoll epoll_fd:" << epoll_fd << " local_listen_fd:" << local_listen_fd;
        return -4;
    }
    ip_port_t server_info;
    server_info.port = remote_port;
    server_info.ip = remote_ip;
    std::shared_ptr<tcptun::ConnectionManager>
        sp_tcptun_cm(new tcptun::ConnectionManager(local_listen_fd, 0, server_info));
    int peer_connected_fd = -1;
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, maxevent, -1);
        ret = nfds;
        if (nfds < 0) {
            if (errno != EINTR) {
                LOG(ERROR) << "epoll_wait return error:" << strerror(errno);
                break;
            }
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == local_listen_fd) {
                auto new_peer_connected_fd = sp_tcptun_cm->HandleNewConnection(false);
                if (new_peer_connected_fd < 0) {
                    LOG(ERROR) << "failed to call tcptun::ConnectionManager HandleNewConnection";
                    continue;
                }
                peer_connected_fd = new_peer_connected_fd;
                auto temp = tcptun::AddEvent2Epoll(epoll_fd, new_peer_connected_fd, EPOLLIN);
                ///todo how to handle this issue is a problem but fortunately it will barely happen
                if (temp < 0) {
                    LOG(ERROR) << "failed to call AddEvent2Epoll epoll_fd:" << epoll_fd << " new_peer_connected_fd:"
                               << new_peer_connected_fd;
                }
            } else if (events[i].data.fd == peer_connected_fd) {
                auto correspond_fd = sp_tcptun_cm->RecvDataFromPeer();
                if (correspond_fd < 0) {
                    LOG(ERROR) << "failed to call tcptun::ConnectionManager RecvDataFromPeer ret:"<<correspond_fd;
                    continue;
                }
                struct epoll_event temp_event = {0};
                temp_event.data.fd = correspond_fd;
                temp_event.events = EPOLLIN;
                auto temp = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, correspond_fd, &temp_event);
                if (temp < 0 && errno != EEXIST) {
                    LOG(ERROR) << "failed to call epoll_ctl ADD epoll_fd:" << epoll_fd << " correspond_fd:"
                               << correspond_fd;
                }
            }
            else{
                sp_tcptun_cm->RecvDataFromOutside(events[i].data.fd);
            }
        }
    }
    return ret;
}

int main(int argc, char *argv[]) {
    google::InitGoogleLogging("INFO");
    FLAGS_logtostderr = true;
    if (argc != 2) {
        LOG(ERROR) << "usage:" << argv[0] << " config_json_path";
        return 0;
    }
    const std::string config_file_path(argv[1]);
    run(config_file_path);
    return 0;
}




















