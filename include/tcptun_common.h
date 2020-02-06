//
// Created by lwj on 2020/2/3.
//

#ifndef TCPTUN_TCPTUN_COMMON_H
#define TCPTUN_TCPTUN_COMMON_H
#include <string>

namespace tcptun {

typedef struct {
  std::string ip;
  int32_t port;
} ip_port_t;

int32_t AddEvent2Epoll(const int32_t &epoll_fd, const int32_t &fd, const uint32_t &events);

int set_non_blocking(const int32_t &fd);

int new_listen_socket(const std::string &ip, const size_t &port, int &fd);

int new_connected_socket(const std::string &remote_ip, const size_t &remote_port, int &fd);

void write_u32(char *p, uint32_t l);

uint32_t read_u32(const char *p);

int64_t getnowtime_ms();

}

#endif //TCPTUN_TCPTUN_COMMON_H
