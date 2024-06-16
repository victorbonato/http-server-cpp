#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

struct http_request_line process_request_line(std::string request_string);
bool file_exists(std::string target);

int main(int argc, char** argv)
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible
    // when running tests.
    std::cout << "Logs from your program will appear here!\n";

    // Create socket and assign its file descriptor to server_fd
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    // Create address for the socket to use
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);

    // Bind the socket to the created address
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }

    // Define a maximum of 5 simultaneous connections before refusing connections
    int connection_backlog = 5;
    // Start to listen on the socket
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    std::cout << "Waiting for a client to connect...\n";

    // Accept a new connection from a client and store the file descriptor in
    // client_fd
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr,
        (socklen_t*)&client_addr_len);

    if (client_fd == -1) {
        std::cerr << "Error while establishing connection with client" << std::endl;
        return 1;
    }
    std::cout << "Client connected\n";

    // Read client message
    char client_msg_buffer[1024];
    ssize_t read_size = read(client_fd, &client_msg_buffer, sizeof(client_msg_buffer));
    if (read_size == -1) {
        std::cerr << "Error while reading client request" << std::endl;
        return 1;
    }
    std::string client_msg(client_msg_buffer);
    std::cerr << "Client Message (length: " << client_msg.size() << ")" << std::endl;
    std::clog << client_msg << std::endl;

    std::string response = client_msg.rfind("GET / HTTP/1.1\r\n") == 0 ? "HTTP/1.1 200 OK\r\n\r\n" : "HTTP/1.1 404 Not Found\r\n\r\n";

    // bool file_found = file_exists(target);

    send(client_fd, response.c_str(), response.size(), 0);

    close(client_fd);
    close(server_fd);

    return 0;
}

bool file_exists(std::string target)
{
    if (target == "/")
        return true;

    std::filesystem::path target_path = "files" + target;

    return (std::filesystem::exists(target_path)) ? true : false;
}
