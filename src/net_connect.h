//
// Created by liwen on 2024-12-23.
//

#ifndef GB28181_CLIENT_NET_CONNECT_H
#define GB28181_CLIENT_NET_CONNECT_H

#include "log_helper.h"
#include <string>
#include <netinet/in.h>

NS_BEGIN

class NetConnect {
public:
    NetConnect();
    ~NetConnect();
    int connect(int localPort, std::string rtpIp, int rtpPort, std::string protocol);
    void close();
    int sendPacket(const char * data, int length);
public:
    static int allocSocketPort();
    static const bool isBigEndianNet();
private:
    bool isTcpConnected(int socket);
    static bool isPortAvailable(int port);
    static int getRandomAvailablePort(int minPort, int maxPort);
private:
    int sock_fd_;
    std::string rtp_protocol_;
    sockaddr_in server_address_;
    
};

NS_END

#endif //GB28181_CLIENT_NET_CONNECT_H
