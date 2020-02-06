//
// Created by lwj on 2020/2/3.
//

#include <cstring>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <glog/logging.h>
#include "random_generator.h"
#include "tcptun_common.h"
#include "tcptun_connection_manager.h"

namespace tcptun {

ConnectionManager::ConnectionManager(const int32_t &local_listen_fd,
                                     const int32_t &peer_connected_fd,
                                     ip_port_t ip_port)
    : local_listen_fd_(local_listen_fd),
      peer_connected_fd_(peer_connected_fd),
      remote_server_info_(std::move(ip_port)) {
    auto ret = set_non_blocking(local_listen_fd_);
    if (ret < 0)
        LOG(ERROR) << "failed to call set_non_blocking to local_listen_fd:" << local_listen_fd;
    if (peer_connected_fd != 0) {
        ret = set_non_blocking(peer_connected_fd);
        if (ret < 0)
            LOG(ERROR) << "failed to call set_non_blocking to peer_connected_fd:" << peer_connected_fd;
    }
}

int32_t ConnectionManager::HandleNewConnection(bool is_client) {
    if (is_client) {
        auto new_conn_fd = accept(local_listen_fd_, nullptr, nullptr);
        if (new_conn_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            LOG(ERROR) << "tcptun client failed to call accept, error:" << strerror(errno);
            return -1;
        }
        uint32_t conn_id = 0;
        while (true) {
            conn_id = 0;
            auto ret = RandomNumberGenerator::GetInstance()->GetRandomNumberNonZero(conn_id);
            if (ret < 0) {
                LOG(ERROR) << "failed to call GetRandomNumberNonZero ret" << ret;
                return -2;
            }
            if (!connid2outside_connectionfd_.count(conn_id))
                break;
        }
        connid2outside_connectionfd_[conn_id] = new_conn_fd;
        outside_connectionfd_2connid_[new_conn_fd] = conn_id;
        return new_conn_fd;
    } else {
        auto new_peer_fd = accept(local_listen_fd_, nullptr, nullptr);
        if (new_peer_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            LOG(ERROR) << "tcptun server failed to call accept, error:" << strerror(errno);
            return -1;
        }
        if (peer_connected_fd_ != 0)
            close(peer_connected_fd_);
        ///we will just use the new fd to replace the old,so for a single
        ///tcptun server it can just handle one tcptun client at the same time
        ///if you want to use more tcptun client, you can run the same
        ///number of tcptun servers as tcptun clients
        peer_connected_fd_ = new_peer_fd;
        for(auto& ele : outside_connectionfd_2connid_)
            close(ele.first);
        connid2outside_connectionfd_.clear();
        outside_connectionfd_2connid_.clear();
        bzero(recv_buf, sizeof(recv_buf));
        return new_peer_fd;
    }
}

int32_t ConnectionManager::RecvDataFromPeer() {
    bzero(recv_buf, sizeof(recv_buf));
    recv_len = recv(peer_connected_fd_, recv_buf, sizeof(recv_buf), 0);
    auto ret = recv_len;
    if (ret < 0) {
        LOG(ERROR) << "failed to call recv error" << strerror(errno);
        return -1;
    } else if (ret == 0) {
        LOG(INFO) << "peer closed";
        ///a closing fd will be moved by epoll, so we don't need to worry about it
        close(peer_connected_fd_);
        peer_connected_fd_ = 0;
        return 0;
    }
    auto conn_id = read_u32(recv_buf);
    if (!connid2outside_connectionfd_.count(conn_id)) {
        ///only tcptun_server can run to here, means we need to establish a new connection to server
        int32_t connected_fd = -1;
        auto ncs_ret = new_connected_socket(remote_server_info_.ip, remote_server_info_.port, connected_fd);
        if (ncs_ret < 0) {
            LOG(ERROR) << "failed to call new_connected_socket ret:" << ncs_ret;
            return -3;
        }
        ret = set_non_blocking(ncs_ret);
        if (ret < 0)
            LOG(WARNING) << "failed to call set_non_blocking on new_connected_fd:" << ncs_ret;
        outside_connectionfd_2connid_[connected_fd] = conn_id;
        connid2outside_connectionfd_[conn_id] = connected_fd;
    }
    ///now we need to send the data that we received from peer to outside corresponding connection
    ret = send(connid2outside_connectionfd_[conn_id], recv_buf + 4, recv_len - 4, 0);
    if (ret < 0) {
        LOG(ERROR) << "failed to call send for fd:" << connid2outside_connectionfd_[conn_id] << " error:"
                   << strerror(errno);
        return -4;
    } else {
        if (ret != (recv_len - 4)) {
            ///todo
            LOG(WARNING) << "failed to send all the data for fd:" << connid2outside_connectionfd_[conn_id];
            ///we need to handle the issue when tcp buffer don't have enough space for us to send data
        }
    }
    return connid2outside_connectionfd_[conn_id];
}

int32_t ConnectionManager::RecvDataFromOutside(const int32_t &readable_fd) {
    if (!outside_connectionfd_2connid_.count(readable_fd)) {
        LOG(WARNING) << "readable_fd is not recorded:" << readable_fd;
        return -1;
    }
    bzero(recv_buf, sizeof(recv_buf));
    ///we need to leave space before datafor conn_id
    recv_len = recv(readable_fd, recv_buf + 4, sizeof(recv_buf) - 4, 0);
    auto ret = recv_len;
    if (ret < 0) {
        LOG(ERROR) << "failed to call recv error" << strerror(errno);
        return -2;
    } else if (ret == 0) {
        LOG(INFO) << "outside connection closed";
        ///a closing fd will be moved by epoll, so we don't need to worry about it
        close(readable_fd);
        uint32_t conn_id = outside_connectionfd_2connid_[readable_fd];
        outside_connectionfd_2connid_.erase(readable_fd);
        connid2outside_connectionfd_.erase(conn_id);
        return -3;
    }
    write_u32(recv_buf, outside_connectionfd_2connid_[readable_fd]);
    ret = send(peer_connected_fd_, recv_buf, recv_len + 4, 0);
    if (ret < 0) {
        LOG(ERROR) << "failed to call send for peer_connected_fd:" << peer_connected_fd_ << " error:"
                   << strerror(errno);
        return -4;
    } else {
        if (ret != (recv_len + 4)) {
            ///todo
            LOG(WARNING) << "failed to send all the data for peer_connected_fd:" << peer_connected_fd_;
            ///we need to handle the issue when tcp buffer don't have enough space for us to send data
        }
    }
    return 0;
}

}


















