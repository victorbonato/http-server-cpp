#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct http_request_line {
  std::string method;
  std::string target;
  std::string http_version;
};

struct http_request_line process_request_line(std::string request_string);
bool file_exists(std::string target);

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                         (socklen_t *)&client_addr_len);
  std::cout << "Client connected\n";

  char buffer[1024];
  read(client_fd, &buffer, sizeof(buffer));
  std::string request(buffer);
  struct http_request_line processed_request_line = process_request_line(request);

  std::string target = processed_request_line.target;
  bool file_found = file_exists(target);

  char http_200_ok[24] = "HTTP/1.1 200 OK\r\n\r\n";
  char http_404_not_found[31] = "HTTP/1.1 404 Not Found\r\n\r\n";

  if (file_found) {
    send(client_fd, &http_200_ok, sizeof(http_200_ok), 0);
  } else {
    send(client_fd, &http_404_not_found, sizeof(http_200_ok), 0);
  }

  close(client_fd);
  close(server_fd);

  return 0;
}


struct http_request_line process_request_line(std::string request_string) {
  std::istringstream request_stream(request_string);
  std::string request_line;
  std::getline(request_stream, request_line);

  struct http_request_line request;
  std::istringstream request_line_stream(request_line);

  std::getline(request_line_stream, request.method, ' ');
  std::getline(request_line_stream, request.target, ' ');
  std::getline(request_line_stream, request.http_version, '\r');

  return request;
}

bool file_exists(std::string target) {
  if (target == "/") return true;

  std::filesystem::path target_path = "files" + target;

  return (std::filesystem::exists(target_path)) ? true : false;
}
