#include "http_tcpServer_linux.h"

using namespace http;

int main() {
        TcpServer server = TcpServer("127.0.0.1", 8080);
        server.startListen();
        return 0;
}

