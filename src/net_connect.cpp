//
// Created by liwen on 2024-12-23.
//

#include "net_connect.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

NS_BEGIN

#define MIN_PORT  10000
#define MAX_PORT  20000
// 函数：判断主机字节序
const bool NetConnect::isBigEndianNet() {
    static int bigEndian{-1};
    if(bigEndian == -1){
        union {
            unsigned int i;
            unsigned char c[4];
        } test;

        test.i = 0x01020304;

        if (test.c[0] == 1) {
            // 大端字节序
            bigEndian = 1;
        } else {
            bigEndian = 0;
        }
    }
    return bigEndian == 1 ? true : false;
}

// 检查指定端口是否可用
bool NetConnect::isPortAvailable(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_ERROR(MSG_LOG, "Socket creation failed!");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 尝试绑定套接字到指定端口
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        ::close(sockfd);
        return false;  // 端口被占用
    }

    ::close(sockfd);
    return true;  // 端口可用
}

// 获取指定范围内随机可用端口
int NetConnect::getRandomAvailablePort(int minPort, int maxPort) {
    // 设置随机数种子
    std::srand(std::time(0));

    bool portFound = false;
    int chosenPort = 0;

    // 尝试从该范围内随机选择一个可用端口
    while (!portFound) {
        // 生成随机端口号
        chosenPort = std::rand() % (maxPort - minPort + 1) + minPort;
        LOG_INFO(MSG_LOG, "Trying port: {}", chosenPort);
        // 检查端口是否可用
        if (isPortAvailable(chosenPort)) {
            portFound = true;
        }
    }
    return chosenPort;
}

NetConnect::NetConnect() {

}

NetConnect::~NetConnect() {

}

int NetConnect::allocSocketPort() {
    return getRandomAvailablePort(MIN_PORT, MAX_PORT);
}

bool NetConnect::isTcpConnected(int sock){
    struct tcp_info info;
    int len = sizeof(struct tcp_info);
    getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    if (info.tcpi_state == TCP_ESTABLISHED){
        return true;
    }
    return false;
}

int NetConnect::connect(int localPort, std::string rtpIp, int rtpPort, std::string protocol) {
    rtp_protocol_ = protocol;
    if (rtp_protocol_ == "TCP/RTP/AVP") {
        sock_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    } else {
        sock_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    sockaddr_in bind_address;
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(localPort);
    bind_address.sin_addr.s_addr = INADDR_ANY;

    auto bind_status = ::bind(sock_fd_, (struct sockaddr *) &bind_address, sizeof(bind_address));
    if (bind_status) {
        LOG_ERROR(MSG_LOG, "socket bind failed: {}", bind_status);
        ::close(sock_fd_);
        return bind_status;
    }

    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(rtpPort);
    inet_pton(AF_INET, rtpIp.c_str(), &server_address_.sin_addr);

    if (rtp_protocol_ == "TCP/RTP/AVP") {
        auto connect_status = ::connect(sock_fd_, (struct sockaddr *) &server_address_, sizeof(server_address_));
        if (connect_status < 0) {
            LOG_ERROR(MSG_LOG, "socket connect failed: {}", connect_status);
            ::close(sock_fd_);
            return connect_status;
        }
    }
    return sock_fd_;
}

void NetConnect::close() {
    if (sock_fd_ >= 0) {
        ::close(sock_fd_);
        sock_fd_ = -1;
    }
}

/*GB28181的TCP码流遵循的标准是RFC4571（RTP OVER TCP），具体类型是：
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    ---------------------------------------------------------------
   |             LENGTH            |  RTP or RTCP packet ...       |
    ---------------------------------------------------------------
 * */

int NetConnect::sendPacket(const char *data, int length) {
    int ret{0};
    if (rtp_protocol_ == "TCP/RTP/AVP") {
        if(!isTcpConnected(sock_fd_)){
            return -2;
        }
        // rtp 封装的时候rtp_buffer多分配了4个字节
        uint8_t * rtpPacket = ((uint8_t*)data) - 2;
        rtpPacket[0] = (length >> 8) & 0xff;
        rtpPacket[1] = length & 0xff;
        ret = ::send(sock_fd_, rtpPacket, length + 2, 0);
        if (ret <= 0) {
            return -1;
        }
    } else {
        int ret = ::sendto(sock_fd_, data, length, 0, (struct sockaddr *) &server_address_, sizeof(server_address_));
        if (ret < 0) {
            return -1;
        }
    }
    return ret;
}

NS_END