#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class Request {
public:
    std::string method;
    std::string target;
    std::string http_version;
    std::map<std::string, std::string> headers;

    Request(std::string message)
    {
        std::istringstream message_stream(message);

        std::string status_line;
        std::getline(message_stream, status_line) >> std::ws;
        std::istringstream status_line_stream(status_line);
        std::getline(status_line_stream, this->method, ' ');
        std::getline(status_line_stream, this->target, ' ');
        std::getline(status_line_stream, this->http_version, '\r');

        std::string header;
        while (std::getline(message_stream, header) >> std::ws) {
            std::istringstream header_stream(header);
            std::string header_key, header_value;
            std::getline(header_stream, header_key, ':');
            std::getline(header_stream >> std::ws, header_value, '\r');

            this->headers.insert(std::pair<std::string, std::string>(header_key, header_value));
        }
    }
};

class Response {
public:
    std::string status_line;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    std::map<int, std::string> status_to_response = {
        { 200, "HTTP/1.1 200 OK\r\n" },
        { 404, "HTTP/1.1 404 Not Found\r\n" }
    };

    Response() { }

    Response(int status, std::vector<std::pair<std::string, std::string>> extra_headers, std::string body)
    {
        this->status_line = status_to_response[status];
        this->headers.push_back(std::pair<std::string, std::string>("Content-Type", "text/plain"));
        this->headers.push_back(std::pair<std::string, std::string>("Content-Length", std::to_string(body.size())));

        for (std::pair<std::string, std::string>& header : extra_headers) {
            this->headers.push_back(std::pair<std::string, std::string>(header.first, header.second));
        }

        this->body = body;
    }

    std::string to_string()
    {
        std::string result;

        result += status_line;
        for (std::pair<std::string, std::string> header : this->headers) {
            result += header.first + ": " + header.second + "\r\n";
        }
        result += "\r\n";
        result += body;

        return result;
    }
};

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

    // Create request object
    Request request = Request(client_msg);

    Response response;

    std::vector<std::pair<std::string, std::string>> extra_headers;

    std::string prefix_echo = "/echo";
    std::string prefix_user_agent = "/user-agent";
    if (request.target == "/") {
        response = Response(200, extra_headers, "");
    } else if (request.target.rfind(prefix_echo, 0) == 0) {
        response = Response(200, extra_headers, request.target.substr(prefix_echo.length() + 1));
    } else if (request.target.rfind(prefix_user_agent, 0) == 0) {
        std::string user_agent = request.headers["User-Agent"];
        response = Response(200, extra_headers, user_agent);
    } else {
        response = Response(404, extra_headers, "");
    }

    std::string response_string = response.to_string();

    std::clog << "Response: \n"
              << response_string << std::endl;

    send(client_fd, response_string.c_str(), response_string.size(), 0);

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
