#include "tcpserver.hpp"

/**
 * @brief Starts the ECG sensor.
 */
TcpServer::TcpServer() { running = true; }

/**
 * @brief Starts the ECG sensor.
 * @param ads_ptr Pointer to the ecg object.
 */
void TcpServer::start(std::unique_ptr<ECG>& ecg_ptr) {
  setupSocket();

  while (running) {
    std::string message;
    if (!ecg_ptr->ecgtcpqueue.pop(message)) {
      std::this_thread::yield();
    } else {
      std::cout << "Message: " << message << std::endl;
      if (socket_connected) {
        if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
          std::cerr << "Error sending data" << std::endl;
          socket_connected = false;  // Handle connection issue
          // Potentially attempt to re-establish connection
        }
      }
    }
  }
}

void TcpServer::setupSocket() {
  // Create the server socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    std::cerr << "Error creating socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Configure address structure
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // Bind the socket
  if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
    std::cerr << "Error binding socket" << std::endl;
    exit(EXIT_FAILURE);
  }
  // Start listening
  if (listen(server_socket, SOMAXCONN) < 0) {
    std::cerr << "Error in listen()" << std::endl;
    exit(EXIT_FAILURE);
  }
  // Accept a connection
  socklen_t addrlen = sizeof(address);
  client_socket = accept(server_socket, (struct sockaddr*)&address, &addrlen);
  if (client_socket < 0) {
    std::cerr << "Error accepting client" << std::endl;
  } else {
    std::cout << "Client connected" << std::endl;
    socket_connected = true;
  }
}

/**
 * @brief Stops the Tcp Server.
 */
void TcpServer::stop() {
  running = false;

  if (server_socket >= 0) {
    if (close(server_socket) == -1) {
      std::cerr << "Error closing server socket: " << strerror(errno)
                << std::endl;
    } else {
      server_socket = -1;
      std::cout << "Server socket closed" << std::endl;
    }
  }
  if (client_socket >= 0) {
    if (close(client_socket) == -1) {
      std::cerr << "Error closing client socket: " << strerror(errno)
                << std::endl;
    } else {
      client_socket = -1;
      std::cout << "Client socket closed" << std::endl;
    }
  }
}

/**
 * @brief Destructor for the ECG sensor.
 */
TcpServer::~TcpServer() { stop(); }