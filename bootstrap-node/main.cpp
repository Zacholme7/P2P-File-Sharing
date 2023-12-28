#include "bootstrapNode.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <unordered_map>

// helper function to serialize our active peers map into string for sending over the network
std::string serializeMap(const std::unordered_map<std::string, int> &map) {
        std::string serializedMap;
        for(const auto &pair: map) {
                serializedMap += pair.first + ":" + std::to_string(pair.second) + ";";
        }
        return serializedMap;
}

int main() {
        BootstrapNode bootstrapNode;
        int bootstrapNodeSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (bootstrapNodeSocket == -1) {
                std::cerr << "Failed to create the server" << std::endl;
                return 1;
        }

        // configure the socket internet address
        sockaddr_in sockaddr{};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        sockaddr.sin_port = htons(50000);

        // bind the socket
        if (bind(bootstrapNodeSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                std::cerr << "Failed to bind to the port 50000" << std::endl;
                return 1;
        }

        // listen on the socket
        listen(bootstrapNodeSocket, 10);

        // continuously listen for connections
        int addrlen = sizeof(sockaddr);
        while (true) {
                // accept the new peer
                int newPeerFd = accept(bootstrapNodeSocket, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                if(newPeerFd < 0) {
                        std::cerr << "Failed to accept new connection" << std::endl;
                        continue;
                }

                // read the name of the newly connected client
                // record the connected peer for future communcation
                std::string peerName;
                char buffer[1024];
                int bytesRead = recv(newPeerFd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        peerName = buffer;
                        std::cout << "connected peer " << peerName << std::endl;
                }

                // new peer will connect
                // send a peer update message to the new peer
                // send the new peer all of the current Connected peers

                // offidically register the new peer
                bootstrapNode.registerPeer(peerName);
                // send a peer update message to all of the connected peers
                // send all of the connected peers the new peer

        }
}