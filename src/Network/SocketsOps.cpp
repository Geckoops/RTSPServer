#include "SocketsOps.h"

#include <fcntl.h>
#include <sys/types.h>

#include "Log.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#endif  //_WIN32

int sockets::createTcpSock() {
#ifndef _WIN32
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);

#else
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);

    if (ret == SOCKET_ERROR) {
        LOGE("设置非阻塞失败");
    }
#endif

    return sockfd;
}

int sockets::createUdpSock() {
#ifndef _WIN32
    int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
#else
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);

    if (ret == SOCKET_ERROR) {
        LOGE("设置非阻塞失败");
    }
#endif

    return sockfd;
}

bool sockets::bind(int sockfd, std::string ip, uint16_t port) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGE("::bind error, fd=%d, ip=%s, port=%d", sockfd, ip.c_str(), port);
        return false;
    }
    return true;
}

bool sockets::listen(int sockfd, int backlog) {
    if (::listen(sockfd, backlog) < 0) {
        LOGE("::listen error, fd=%d, backlog=%d", sockfd, backlog);
        return false;
    }
    return true;
}

int sockets::accept(int sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr_in);

    int connfd = accept(sockfd, (struct sockaddr*)&addr, &addr_len);
    setNonBlockAndCloseOnExec(connfd);
    ignoreSigPipeOnSocket(connfd);

    return connfd;
}

int sockets::send(int sockfd, const void* buf, int size) {
#ifndef _WIN32
    return write(sockfd, buf, size);
#else
    return ::send(sockfd, (char*)buf, size, 0);
#endif
}

int sockets::sendto(int sockfd, const void* buf, int len,
                    const struct sockaddr* destAddr) {
    socklen_t addr_len = sizeof(struct sockaddr);
    return sendto(sockfd, (char*)buf, len, 0, destAddr, addr_len);
}

int sockets::recv(int sockfd, void* buf, int size) {
#ifndef _WIN32
    return read(sockfd, buf, size);
#else
    return ::recv(sockfd, (char*)buf, size, 0);
#endif
}

int sockets::recvfrom(int sockfd, void* buf, int len,
                      struct sockaddr* srcAddr) {
    socklen_t addr_len = sizeof(struct sockaddr);
    return recvfrom(sockfd, (char*)buf, len, 0, srcAddr, &addr_len);
}

int sockets::setNonBlock(int sockfd) {
#ifndef _WIN32
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#else

    unsigned long ul = 1;
    int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long*)&ul);  // 设置非阻塞

    if (ret == SOCKET_ERROR) {
        return -1;
    }
#endif

    return 0;
}

int sockets::setBlock(int sockfd, int writeTimeout) {
#ifndef _WIN32
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    if (writeTimeout > 0) {
        struct timeval tv = {writeTimeout / 1000, (writeTimeout % 1000) * 1000};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
    }
#endif

    return 0;
}

void sockets::setReuseAddr(int sockfd, int on) {
    int optval = on ? 1 : 0;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval,
               sizeof(optval));
}

void sockets::setNonBlockAndCloseOnExec(int sockfd) {
#ifndef _WIN32
    // non-block
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = fcntl(sockfd, F_SETFD, flags);
#endif
}

void sockets::ignoreSigPipeOnSocket(int socketfd) {
#ifndef _WIN32
    int option = 1;
    setsockopt(socketfd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
#endif
}

std::string sockets::getPeerIp(int sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
        return inet_ntoa(addr.sin_addr);
    }

    return "0.0.0.0";
}

int16_t sockets::getPeerPort(int sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
        return ntohs(addr.sin_port);
    }

    return 0;
}

int sockets::getPeerAddr(int sockfd, struct sockaddr_in* addr) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    return getpeername(sockfd, (struct sockaddr*)addr, &addr_len);
}

void sockets::close(int sockfd) {
#ifndef _WIN32
    close(sockfd);
#else
    closesocket(sockfd);
#endif
}

bool sockets::connect(int sockfd, std::string ip, uint16_t port, int timeout) {
    bool is_connected = true;
    if (timeout > 0) {
        sockets::setNonBlock(sockfd);
    }

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (::connect(sockfd, (struct sockaddr*)&addr, addr_len) < 0) {
        if (timeout > 0) {
            is_connected = false;
            fd_set fd_write;
            FD_ZERO(&fd_write);
            FD_SET(sockfd, &fd_write);
            struct timeval tv = {timeout / 1000, timeout % 1000 * 1000};
            select(sockfd + 1, NULL, &fd_write, NULL, &tv);
            if (FD_ISSET(sockfd, &fd_write)) {
                is_connected = true;
            }
            sockets::setBlock(sockfd, 0);
        } else {
            is_connected = false;
        }
    }

    return is_connected;
}

std::string sockets::getLocalIp() { return "0.0.0.0"; }