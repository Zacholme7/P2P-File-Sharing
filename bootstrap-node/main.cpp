#include "bootstrapServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 50000

int main() {
        BootstrapServer bootstrapServer;
        std::string port = "50000";
        bootstrapServer.startServer(port);
        return 0;
}