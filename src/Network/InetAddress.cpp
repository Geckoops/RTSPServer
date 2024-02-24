#include "InetAddress.h"

Ipv4Address::Ipv4Address() {}

Ipv4Address::Ipv4Address(std::string ip, uint16_t port) : ip_(ip), port_(port) {
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_port = htons(port);
}

void Ipv4Address::setAddr(std::string ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    addr_.sin_port = htons(port);
}

std::string Ipv4Address::getIp() { return ip_; }

uint16_t Ipv4Address::getPort() { return port_; }

struct sockaddr* Ipv4Address::getAddr() { return (struct sockaddr*)&addr_; }
