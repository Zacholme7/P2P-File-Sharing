#ifndef INCLUDED_PEER
#define INCLUDED_PEER

#include <string>
#include <type_traits>


namespace peer {
        class Peer {
        public:
                Peer(int port, std::string sharedDir);
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

                void processCommand(std::string& command);
        private:
                // command execution
                void connectToPeer(int port, std::string &ip);

                // persistent client functions
                void listenToClient(int clientSocket);
                int port;
                std::string sharedDir;


                // can have some stuff to handle errors
        };
}


#endif 