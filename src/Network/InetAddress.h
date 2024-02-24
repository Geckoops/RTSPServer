#ifndef INETADDRESS_H
#define INETADDRESS_H
#include <stdint.h>

#include <string>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#include <WinSock2.h>
#endif

class Ipv4Address {
   public:
    Ipv4Address();
    Ipv4Address(std::string ip, uint16_t port);
    void setAddr(std::string ip, uint16_t port);
    std::string getIp();
    uint16_t getPort();
    struct sockaddr* getAddr();

   private:
    std::string ip_;
    uint16_t port_;
    struct sockaddr_in addr_;
};

#endif  // INETADDRESS_H