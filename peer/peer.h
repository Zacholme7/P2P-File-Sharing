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

                void startServer(int port);
                void processCommand(std::string& command);

        private:
                void connectToPeerServer(int port, const std::string &ip, std::string &peerServerName);
                void sendMessage(const std::string &serverName, const std::string &message);
                void listenToClient(int clientSocket);
                void listenToServer();

                std::string name;
                int serverSocket;
                // 
                std::unordered_map<std::string, int> connectedServers;
                std::vector<std::thread> clientThreads;
                std::mutex mutex;
        };
}

#endif 