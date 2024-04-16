#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

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

class TcpClient {
 public:
  TcpClient();
  ~TcpClient();

  void start(std::unique_ptr<std::thread>& receive_ptr);

 private:
  void setupSocket();
  /*
   * @brief Worker thread function.
   * @param tcp_server Pointer to the TcpServer object.
   * @param queue Reference to the queue.
   */
  static void workerThreadFunc(
        TcpServer* client_ptr,
        boost::lockfree::spsc_queue<std::string, boost::lockfree::capacity<1024>>&
            data_queue) {
    while (true) {
        char buffer[1024];
        std::string message;
        if (!data_queue.pop(message)) {
            std::cerr << "Queue Empty" << std::endl;
            std::this_thread::yield();
        }
        else{
            std::lock_guard<std::mutex> lock(client_ptr->socket_mutex);
            int bytes_received = recv(client_ptr->server_socket, buffer, sizeof(buffer), 0);
            if (bytes_received < 0) {
                std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                client_ptr->socket_connected = false;  // Manage connection state
                break;                                 // Exit the loop on error
            } else if (bytes_received == 0) {
                // Connection closed by the client
                std::cerr << "Connection closed by client" << std::endl;
                client_ptr->socket_connected = false;  // Manage connection state
                break;                                 // Exit the loop
            } else {
                // Append received data to the message string
                message.append(buffer, bytes_received);
                // Push data to the queue
                
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