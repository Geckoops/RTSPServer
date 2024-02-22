#ifndef SOCKETSOPS_H
#define SOCKETSOPS_H
#include <stdint.h>

#include <string>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#include <WinSock2.h>
#endif  //_WIN32

namespace sockets {

/* TCP_API */
int createTcpSock();

bool connect(int sockfd, std::string ip, uint16_t port, int timeout);
bool listen(int sockfd, int backlog);
int accept(int sockfd);

int send(int sockfd, const void* buf, int size);
int recv(int sockfd, void* buf, int size);

std::string getPeerIp(int sockfd);
int16_t getPeerPort(int sockfd);

void setNonBlockAndCloseOnExec(int sockfd);
void ignoreSigPipeOnSocket(int socketfd);
int setNonBlock(int sockfd);                 // 设置非阻塞模式
int setBlock(int sockfd, int writeTimeout);  // 设置阻塞模式
void setReuseAddr(int sockfd, int on);

int getPeerAddr(int sockfd, struct sockaddr_in* addr);

/* UDP_API */
int createUdpSock();

int sendto(int sockfd, const void* buf, int len,
           const struct sockaddr* destAddr);
int recvfrom(int sockfd, void* buf, int size, struct sockaddr* srcAddr);

/* Utils */
bool bind(int sockfd, std::string ip, uint16_t port);
void close(int sockfd);

std::string getLocalIp();

}  // namespace sockets

#endif  // SOCKETSOPS_H