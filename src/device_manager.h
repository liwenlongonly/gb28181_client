/******************************************************************
 * Filename: device_manager.h
 * Author: liwen
 * Description: Device Management.
 * Version: 1.0.0
 * Created: 2024-12-27
 * Last Modified: 2024-12-27
 ******************************************************************/
//

#ifndef GB28181_CLIENT_DEVICE_MANAGER_H
#define GB28181_CLIENT_DEVICE_MANAGER_H

#include <map>

#include "log_helper.h"
#include "Singleton.h"
#include "virtual_device_i.h"
#include "basic_error.h"

NS_BEGIN

class SQLiteUtils;

extern void reply_error(std::string& reply, int code, std::string const& msg="");

class DeviceManager {
public:
    using Map = std::map<std::string, std::shared_ptr<VirtualDeviceI>>;
    using MapPtr = std::shared_ptr<Map>;

    ~DeviceManager();
    void initFromDB();
    // 设备创建
    void deviceCreate(const std::string &reqBody,std::string &reply);
    // 设备启动
    void deviceStart(const std::string &reqBody,std::string &reply);
    // 设备关闭
    void deviceStop(const std::string &reqBody,std::string &reply);
    // 设备删除
    void deviceDelete(const std::string &reqBody,std::string &reply);
    // 获取设备列表
    void deviceList(const std::string &reqBody, std::string &reply);
private:
    friend Singleton<DeviceManager>;

    DeviceManager(){
        device_map_ = std::make_shared<Map>();
    }

    MapPtr getDeviceMap(){
        std::lock_guard<std::mutex> lock(map_lock_);
        return device_map_;
    }

    bool isInMap(const std::string &sip);

    void insertDevice(const std::string &key, std::shared_ptr<VirtualDeviceI> device);

    void deleteDevice(const std::string &key);

private:
    MapPtr device_map_; // map的读写 使用 copy-on-write
    std::mutex map_lock_;

    std::shared_ptr<SQLiteUtils> sqlite_utils_;
};

NS_END

#endif //GB28181_CLIENT_DEVICE_MANAGER_H
