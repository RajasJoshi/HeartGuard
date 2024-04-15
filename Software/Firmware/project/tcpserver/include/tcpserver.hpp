#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <boost/lockfree/spsc_queue.hpp>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "ecg.hpp"
#include "ppg.hpp"

class TcpServer {
 public:
  TcpServer();
  ~TcpServer();

  void start(std::unique_ptr<ECG>& ecg_ptr, std::unique_ptr<PPG>& ppg_ptr);

 private:
  void setupSocket();
  /*
   * @brief Worker thread function.
   * @param tcp_server Pointer to the TcpServer object.
   * @param queue Reference to the queue.
   */
  static void workerThreadFunc(
      TcpServer* server_ptr,
      boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>>&
          data_queue) {
    while (true) {
      std::string message;
      // Pop data from the queue
      if (!data_queue.pop(message)) {
        std::this_thread::yield();
      } else {
        std::lock_guard<std::mutex> lock(server_ptr->socket_mutex);
        if (server_ptr->socket_connected) {
          if (send(server_ptr->client_socket, message.c_str(), message.length(),
                   0) < 0) {
            std::cerr << "Error sending data: " << strerror(errno) << std::endl;
            server_ptr->socket_connected = false;  // Manage connection state
            break;                                 // Exit the loop on error
          }
        }
      }
    }
  }

  const int port = 5000;  // Example port number
  int server_socket, client_socket;
  struct sockaddr_in address;
  bool socket_connected = false;
  std::mutex socket_mutex;  // Mutex for socket access
  std::thread worker1;
  std::thread worker2;
};

#endif  // TCPSERVER_HPP