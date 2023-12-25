#include "peer.h"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

using namespace peer;


int main(int argc, char* argv[]) {
        // parse the cli arugments
        int port = 10;
        std::string sharedDir = "home";

        Peer myPeer(port, sharedDir);
        myPeer.startServer(port); // start the server in a sepearate thread

        std::string userInput;
        while (true) {
                std::cout << "Enter command: ";
                std::getline(std::cin, userInput);

                if (userInput == "exit") {
                        break;
                }

                // process the command

        }


        myPeer.shutdown();
}


/*

int main(int argc, char* argv[]) {
 //       Peer peer = Peer("127.0.0.1", 8080);

        int pid = fork();


        // create a socket, do some error checking here
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // listen on port 9999 on any address
        sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = INADDR_ANY;
        sockaddr.sin_port = htons(9999);

        // bind the socket, need some erro checking
        int res = bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

        // start listening for connections, hold at most 10 in the queue
        listen(sockfd, 10);


        // grab a connection from the queue
        int addrlen = sizeof(sockaddr);
        int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);

        // read from the connection
        char buffer[100];
        int bytesRead = read(connection, buffer, 100);
        std::cout << "The message was: " << buffer;

        close(connection);
        close(sockfd);

        return 0;
}

*/
