#include "bootstrapServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <iostream>



int main() {
        BootstrapServer bootstrapServer;
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
                std::cerr << "Failed to create bootstrap the server" << std::endl;
                return 1;
        }

        // configure the socket internet address
        // adjust this to the modern version
        sockaddr_in sockaddr{};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        sockaddr.sin_port = htons(50000);

        // bind the socket
        if (bind(serverSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                std::cerr << "Failed to bind to the port 50000" << std::endl;
                return 1;
        }

        // listen on the socket
        listen(serverSocket, 10);

        // continuously listen for connections
        int addrlen = sizeof(sockaddr);
        while (true) {
                // accept the new peer
                std::cout << "Waiting for a new connection..." << std::endl;
                int newPeerFd = accept(serverSocket, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                if(newPeerFd < 0) {
                        std::cerr << "Failed to accept new connection" << std::endl;
                        continue;
                }

                // grab the new peer connection and start a thread to listen to it
                std::thread newPeerThread(&BootstrapServer::listenToPeer, &bootstrapServer, newPeerFd);
                bootstrapServer.peerThreads.push_back(std::move(newPeerThread));
        }
}