#ifndef BOOTSTRAP_SERVER_H
#define BOOTSTRAP_SERVER_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <tuple>
#include <sys/poll.h>
#include "../json.hpp"

using json = nlohmann::json;

class BootstrapServer {
        public:
                void startServer(std::string &port);
        private:
                void listenForConnections();

                void *get_in_addr(struct sockaddr *sa);
                void add_to_pfds(int newfd, std::vector<struct pollfd> &pfds);
                void del_from_pfds(size_t index, std::vector<struct pollfd> &pfds);

                void processSnapshotRequest(json requestJson);
                void processListFilesRequest(json requestJson, int peerSocket);
                void processPeerWithFileRequest(json requestJson, int peerSocket);

                void handleSocketClose(int fd);




                // networking 
                void connectToPeerServer(std::string &peerServerName, int port);

                void sendMessage(const std::string &peerServerName, const std::string &payload);

                // data
                std::unordered_map<std::string, int> nameToPort;
                std::unordered_map<std::string, int> nameToFd;
                std::unordered_map<int, std::string> fdToName;
                std::unordered_map<std::string, std::vector<std::string>> nameToFiles;
                int serverSocket;
                std::vector<struct pollfd> pfds;
};

#endif