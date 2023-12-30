#ifndef INCLUDED_PEER
#define INCLUDED_PEER

#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <vector>

namespace peer {
        class Peer {
        public:
                Peer(std::string& name);
                ~Peer();

                // Starts the server
                void startServer(int port);

                // Contacts the bootstrap server to get initial snapshot
                void contactBootstrap();

                // Processes a command from the cli
                void processCommand(std::string& command);
        private:
                // Processes the initial snapshot message from the bootstrap with all current peers
                void processBootstrapSnapshot();

                // Processes a file request
                void processFileRequest();
                void requestFileRequest();

                // Handles list file request
                void processAllFiles();
                void requestAllFiles();

                // Processes the message from the 
                void connectToPeerServer(int port, const std::string &ip, std::string &peerServerName);

                // Internal helper function to send messages to another peer or bootstrap server
                void sendMessage(const std::string &serverName, const std::string &payload);

                // Will continuoulsy listen form messages from a connected peer 
                void listenToClient(int clientSocket);

                std::string name;
                int serverSocket;
                std::unordered_map<std::string, int> connectedServers;
                std::vector<std::thread> clientThreads;
                std::mutex mutex;
        };
}

#endif 