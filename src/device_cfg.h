//
// Created by liwen on 2024-12-24.
//

#ifndef GB28181_CLIENT_DEVICE_CFG_H
#define GB28181_CLIENT_DEVICE_CFG_H

#include <string>
#include "macro_definition.h"

NS_BEGIN

class DeviceConfig{
public:
    virtual ~DeviceConfig(){};
    std::string serverSipId;
    std::string serverIp;
    int serverPort;
    std::string deviceSipId;
    int localPort;
    std::string username;
    std::string password;
    std::string manufacture;
    // 设备名称
    std::string deviceName;
    // 文件路径
    std::string filePath;
    int deviceStatus{0};
    std::string createdAt; // 创建时间，用来返回数据用
};

NS_END

#endif //GB28181_CLIENT_DEVICE_CFG_H
