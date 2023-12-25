#ifndef INCLUDED_PEER
#define INCLUDED_PEER

#include <string>


namespace peer {
        class Peer {
        public:
                Peer(int port, std::string sharedDir);
                ~Peer();

                // server methods
                void startServer(int port);
                void acceptConnections();
                void serveFileRequests();

                // client methods
                void connectToPeer(const std::string& peerIp, int peerPort);
                void requestFile(const std::string& filename);

                // common methods
                void listSharedFiled();
                void shutdown();
        private:
                int port;
                std::string sharedDir;

                // can have some stuff to handle errors
        };
}


#endif 