#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;
const string ROOT_DIRECTORY = "./";  // Serve files from the current directory

void send_response(int client_socket, const string &response) {
    send(client_socket, response.c_str(), response.size(), 0);
}

string get_http_response(const string &status, const string &content_type, const string &content) {
    stringstream response;
    response << "HTTP/1.1 " << status << "\n";
    response << "Content-Type: " << content_type << "\n";
    response << "Content-Length: " << content.length() << "\n";
    response << "\n";
    response << content;
    return response.str();
}

string read_file(const string &filename) {
    ifstream file(ROOT_DIRECTORY + filename);
    if (file) {
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "";
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error in socket creation");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error in binding");
    exit(1);

    }


    // Listen for incoming connections
    if (listen(server_socket, 10) == 0) {
        cout << "Listening..." << endl;
    } else {
        perror("Error in listening");
        exit(1);
    }

    addr_size = sizeof(client_addr);

     while (true) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);

        // Handle the HTTP GET request
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        cout << "Received: " << buffer << endl;

        string request(buffer);

        if (request.find("GET") != 0) {
            string response = get_http_response("400 Bad Request", "text/plain", "Bad Request");
            send_response(client_socket, response);
        } else {
            // Extract the requested file
            size_t start = request.find(" ") + 1;
            size_t end = request.find(" ", start);
            string filename = request.substr(start, end - start);

            if (filename == "/unauthorized.html") {
                string response = get_http_response("401 Unauthorized", "text/plain", "Unauthorized");
                send_response(client_socket, response);
            } else {
                filename = ROOT_DIRECTORY + filename;

                if (filename.find(ROOT_DIRECTORY) != 0) {
                    string response = get_http_response("403 Forbidden", "text/plain", "Forbidden");
                    send_response(client_socket, response);
                } else {
                    string content = read_file(filename);
                    if (!content.empty()) {
                        string response = get_http_response("200 OK", "text/html", content);
                        send_response(client_socket, response);
                    } else {
                        string response = get_http_response("404 Not Found", "text/plain", "Not Found");
                        send_response(client_socket, response);
                    }
                }
            }
        }

        close(client_socket);
    }

    close(server_socket);

    return 0;
}