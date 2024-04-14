#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <boost/lockfree/spsc_queue.hpp>
#include <fstream>
#include <queue>

#include "ecg.hpp"

class TcpServer {
 public:
  TcpServer();                                // Constructor
  ~TcpServer();                               // Destructor
  void start(std::unique_ptr<ECG>& ecg_ptr);  // Start the ECG sensor

 private:
  // Socket variables
  int server_socket;  // Socket for the connection
  int client_socket;  // Socket for the client
  struct sockaddr_in address;
  const int port = 5000;          // Example port number
  bool socket_connected = false;  // Flag to track connection status

  void setupSocket(void);  // Function to set up the socket
};

#endif  // TCPSERVER_HPP