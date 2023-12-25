#include "peer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <string>
#include <iostream>

namespace peer {
        Peer::Peer(int port, std::string sharedDir) 
                : port(port), sharedDir(sharedDir) {};

        Peer::~Peer() {};

        void Peer::startServer(int port) {
                // create the socket (IPv4, TCP)
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd == -1) {
                        std::cout << "Failled to create socket" << std::endl;
                        exit(EXIT_FAILURE);
                }

                // Listen to the passed port on any address
                sockaddr_in sockaddr;
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_addr.s_addr = INADDR_ANY;
                sockaddr.sin_port = htons(port);

                // bind the file descriptor to the address
                if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                        std::cout << "Failed to bind to the port" << std::endl;
                        exit(EXIT_FAILURE);
                }

                // start the server
                listen(sockfd, 10);
        }
}