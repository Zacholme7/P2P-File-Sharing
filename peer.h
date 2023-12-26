#ifndef INCLUDED_PEER
#define INCLUDED_PEER

#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>


namespace peer {
        class Peer {
        public:
                Peer(std::string&);
                ~Peer();

                // server methods
                void startServer(int port);
                void handleConnection();
                void serveFileRequests();

                // client methods
                void requestFile(const std::string& filename);

                // common methods
                void listSharedFiled();
                void shutdown();

                void sendMessage(int serverFd, std::string& message);

                void processCommand(std::string& command);
        private:
                // command execution
                void connectToPeer(int port, std::string &ip);

                std::string name;

                // persistent client functions
                void listenToClient(int clientSocket);
                int port;
                std::string sharedDir;


                std::unordered_map<std::string, int> connectedPeers;

                // can have some stuff to handle errors
        };
}


#endif 