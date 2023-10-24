#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

const int MAX_BUFFER_SIZE = 1024;

// Function to read and write data to the server
string fetchFromServer(const string& serverAddress, const string& fileToFetch) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    struct hostent* server = gethostbyname(serverAddress.c_str());
    if (server == NULL) {
        perror("Error, no such host");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serverAddr.sin_addr.s_addr, server->h_length);
    serverAddr.sin_port = htons(80);

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error connecting");
        exit(1);
    }

    ostringstream request;
    request << "GET " << fileToFetch << " HTTP/1.1\r\n";
    request << "Host: " << serverAddress << "\r\n";
    request << "Connection: close\r\n\r\n";
    string getRequest = request.str();

    if (send(sockfd, getRequest.c_str(), getRequest.length(), 0) < 0) {
        perror("Error sending request");
        exit(1);
    }

    char buffer[MAX_BUFFER_SIZE];
    string response;

    while (true) {
        int bytesRead = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }
        response.append(buffer, bytesRead);
    }

    close(sockfd);
    return response;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: retriever <server_address> <file_to_fetch>" << endl;
        return 1;
    }

    const string serverAddress = argv[1];
    const string fileToFetch = argv[2];

    string response = fetchFromServer(serverAddress, fileToFetch);

    // Check HTTP status code
    istringstream responseStream(response);
    string httpVersion;
    int statusCode;
    responseStream >> httpVersion >> statusCode;

    if (statusCode == 200) {
        string content = response.substr(response.find("\r\n\r\n") + 4);
        cout << "Fetched content:\n" << content << endl;

        // Save the content to a file
        ofstream outputFile("downloaded.html");
        outputFile << content;
        outputFile.close();
    } else {
        cerr << "Server returned error code: " << statusCode << endl;
        cerr << "Error Content:\n" << response << endl;
    }

    return 0;
}
