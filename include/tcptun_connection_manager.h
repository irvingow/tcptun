//
// Created by lwj on 2020/2/3.
//

#ifndef TCPTUN_TCPTUN_CONNECTION_MANAGER_H
#define TCPTUN_TCPTUN_CONNECTION_MANAGER_H

#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include "tcptun_common.h"

namespace tcptun {

class ConnectionManager {
 public:
  /**
   * Constructor
   * @param local_listen_fd local listen fd, set it NON_BLOCKING before pass it as a param
   * @param peer_connected_fd connected fd to remote for tcptun client, it's the connected fd to tcptun server,
   * for tcptun server it's the connected fd to outside server, set it NON_BLOCKING before pass it as a param
   * @param ip_port the ip and port information of remote server
   */
  ConnectionManager(const int32_t &local_listen_fd,const int32_t& peer_connected_fd, ip_port_t ip_port);
  /**
   * handle the issue when new connection comes
   * @param is_client if tcptun client call this function, set is_client as true, for tcptun server set it as false
   * @return below zero for error, zero for everythis is fine
   */
  int32_t HandleNewConnection(bool is_client);
  int32_t RecvDataFromPeer();
  int32_t RecvDataFromOutside(const int32_t& readable_fd);
 private:
  int32_t local_listen_fd_;
  ///connected fd to peer, for tcptun_client peer is tcptun_server
  ///for tcptun_server peer is tcptun_client
  int32_t peer_connected_fd_;
  char recv_buf[2048];
  int32_t recv_len;
  ///outside connections, for tcptun_client outside connections are connections from its clients
  ///for tcptun_server outside connections are connections from its server
  ///for both client and server value is conn_id that identify the connection
  std::unordered_map<int32_t, uint32_t> outside_connectionfd_2connid_;
  ///key is conn_id and value is the connection fd
  std::unordered_map<uint32_t, int32_t> connid2outside_connectionfd_;
  ///remote server info
  ///for tcptun_client remote server info is the info of tcptun server
  ///for tcptun_server remote server info is the info of another outside server
  ip_port_t remote_server_info_;
};

}

#endif //TCPTUN_TCPTUN_CONNECTION_MANAGER_H
