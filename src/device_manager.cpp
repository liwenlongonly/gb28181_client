#include "rapidjson/Rjson.hpp"
#include "device_manager.h"
#include "gb28181_device.h"
#include "device_cfg.h"
#include "sqlite_utils.h"
#include "file_utils.h"

using namespace rapidjson;

NS_BEGIN

void reply_error(std::string& reply, int code, std::string const& msg) {
    Document wdc = Rjson::rWriteDC();
    Rjson::rAdd(wdc, "code", code);

    if (msg.empty()){
        Rjson::rAdd(wdc, "msg", StringRef(get_error_msg(code)));
    }
    else{
        Rjson::rAdd(wdc, "msg", StringRef(msg));
    }
    reply = Rjson::ToString(wdc);
}

DeviceManager::~DeviceManager() {

}

void DeviceManager::initFromDB() {
    if(!sqlite_utils_){
        sqlite_utils_ = std::make_shared<SQLiteUtils>();
    }
    sqlite_utils_->initDB(TABLE_NAME);
    auto deviceCfg = sqlite_utils_->queryAllDevice();
    for (auto devcfg : deviceCfg) {
        auto device = std::make_shared<GB28181Device>();
        device->init(devcfg);
        (*device_map_)[devcfg->deviceSipId] = device;
        if(1 == devcfg->deviceStatus){
            int ret = device->start();
            if(ret == ERROR_OK){
                LOG_INFO(MSG_LOG,"device sip id: {} start success.", devcfg->deviceSipId);
            }else{
                LOG_INFO(MSG_LOG,"device sip id: {} start fail.", devcfg->deviceSipId);
            }
        }
    }
}

void DeviceManager::deviceCreate(const std::string &reqBody,std::string &reply) {
    Document dc;
    try {
        Rjson::Parse(dc, reqBody);

        std::string server_sip_id;
        if(!Rjson::GetStringV(server_sip_id,"server_sip_id", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : server_sip_id");
        }

        std::string register_domain;
        if(!Rjson::GetStringV(register_domain,"register_domain", &dc)){
            //return reply_error(reply, ERROR_MISS_PARAM, "miss param : register_domain");
        }

        std::string server_ip;
        if(!Rjson::GetStringV(server_ip,"server_ip", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : server_ip");
        }

        int server_port;
        if(!Rjson::GetIntV(server_port,"server_port", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : server_port");
        }

        std::string device_sip_id;
        if(!Rjson::GetStringV(device_sip_id,"device_sip_id", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : device_sip_id");
        }

        int local_port;
        if(!Rjson::GetIntV(local_port, "local_port", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : local_port");
        }

        std::string username;
        if(!Rjson::GetStringV(username, "username", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : username");
        }

        std::string password;
        if(!Rjson::GetStringV(password, "password", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : password");
        }

        std::string manufacture;
        if(!Rjson::GetStringV(manufacture, "manufacture", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : manufacture");
        }

        if(manufacture.empty()){
            manufacture = "iLong";
        }

        std::string device_name;
        if(!Rjson::GetStringV(device_name,"device_name", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : device_name");
        }

        std::string file_path;
        if(!Rjson::GetStringV(file_path,"file_path", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : file_path");
        }

        if(!isFileExist(file_path)){
            return reply_error(reply, ERROR_FILE_NOT_FOUND);
        }

        if(isInMap(device_sip_id)){
            return reply_error(reply, ERROR_DEVICE_ALREADY_EXISTS);
        }

        if(sqlite_utils_ && sqlite_utils_->isExistLocalHost(local_port)){
            return reply_error(reply, ERROR_LOCAL_HOST_ALREADY_EXISTS);
        }

        auto devcfg = std::make_shared<DeviceConfig>();
        devcfg->serverSipId = server_sip_id;
        devcfg->serverIp = server_ip;
        devcfg->serverPort = server_port;
        devcfg->deviceSipId = device_sip_id;
        devcfg->localPort = local_port;
        devcfg->username = username;
        devcfg->password = password;
        devcfg->manufacture = manufacture;
        devcfg->filePath = file_path;
        devcfg->deviceName = device_name;

        auto device = std::make_shared<GB28181Device>();
        device->init(devcfg);

        if(sqlite_utils_){
           bool ret = sqlite_utils_->addDeviceInfo(devcfg);
           if(ret){
               // 插入设备
               insertDevice(devcfg->deviceSipId, device);
           }else{
               return reply_error(reply, ERROR_EXEC_DATA_BASE);
           }
        }

        return reply_error(reply, ERROR_OK);
    }
    catch (GeneralException2& e) {
        std::string err_str = fmt::format("error:{}, {}", e.err_code(), e.err_str());
        return reply_error(reply, ERROR_INVALID_PARAM, err_str);
    }
    catch (std::exception& e) {
        return reply_error(reply, ERROR_INVALID_PARAM, e.what());
    }
}

void DeviceManager::deviceDelete(const std::string &reqBody,std::string &reply) {
    Document dc;
    try {
        Rjson::Parse(dc, reqBody);

        std::string device_sip_id;
        if(!Rjson::GetStringV(device_sip_id,"device_sip_id", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : device_sip_id");
        }

        if(!isInMap(device_sip_id)){
            return reply_error(reply, ERROR_DEVICE_NOT_FOUND);
        }

        if(sqlite_utils_){
            bool  ret = sqlite_utils_->removeDeviceInfo(device_sip_id);
            if(ret){
                deleteDevice(device_sip_id);
            }else{
                return reply_error(reply, ERROR_EXEC_DATA_BASE);
            }
        }

        return reply_error(reply, ERROR_OK);
    }
    catch (GeneralException2& e) {
        std::string err_str = fmt::format("error:{}, {}", e.err_code(), e.err_str());
        return reply_error(reply, ERROR_INVALID_PARAM, err_str);
    }
    catch (std::exception& e) {
        return reply_error(reply, ERROR_INVALID_PARAM, e.what());
    }
}

void DeviceManager::deviceStart(const std::string &reqBody,std::string &reply) {
    Document dc;
    try {
        Rjson::Parse(dc, reqBody);

        std::string device_sip_id;
        if(!Rjson::GetStringV(device_sip_id,"device_sip_id", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : device_sip_id");
        }
        if(!isInMap(device_sip_id)){
            return reply_error(reply, ERROR_DEVICE_NOT_FOUND);
        }
        auto deviceMap = getDeviceMap();
        if(deviceMap->find(device_sip_id) != deviceMap->end()){
             auto device = (*deviceMap)[device_sip_id];
             int ret = device->start();
             if(ret == 0 && sqlite_utils_){
                sqlite_utils_->modifyDeviceStatus(device_sip_id, 1);
             }
        }else{
            return reply_error(reply, ERROR_DEVICE_NOT_FOUND, "device not find.");
        }
        return reply_error(reply, ERROR_OK);
    }
    catch (GeneralException2& e) {
        std::string err_str = fmt::format("error:{}, {}", e.err_code(), e.err_str());
        return reply_error(reply, ERROR_INVALID_PARAM, err_str);
    }
    catch (std::exception& e) {
        return reply_error(reply, ERROR_INVALID_PARAM, e.what());
    }
}

void DeviceManager::deviceStop(const std::string &reqBody,std::string &reply) {
    Document dc;
    try {
        Rjson::Parse(dc, reqBody);

        std::string device_sip_id;
        if(!Rjson::GetStringV(device_sip_id,"device_sip_id", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : device_sip_id");
        }
        if(!isInMap(device_sip_id)){
            return reply_error(reply, ERROR_DEVICE_NOT_FOUND);
        }
        auto deviceMap = getDeviceMap();
        if(deviceMap->find(device_sip_id) != deviceMap->end()){
            auto device = (*deviceMap)[device_sip_id];
            device->close();
            if(sqlite_utils_){
                sqlite_utils_->modifyDeviceStatus(device_sip_id, 0);
            }
        }else{
            return reply_error(reply, ERROR_DEVICE_NOT_FOUND, "device not find.");
        }

        return reply_error(reply, ERROR_OK);
    }
    catch (GeneralException2& e) {
        std::string err_str = fmt::format("error:{}, {}", e.err_code(), e.err_str());
        return reply_error(reply, ERROR_INVALID_PARAM, err_str);
    }
    catch (std::exception& e) {
        return reply_error(reply, ERROR_INVALID_PARAM, e.what());
    }
}

void DeviceManager::deviceList(const std::string &reqBody,std::string &reply) {
    Document dc;
    try {
        Rjson::Parse(dc, reqBody);
        int page_size{};
        if(!Rjson::GetIntV(page_size,"page_size", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : page_size");
        }
        int page_num{};
        if(!Rjson::GetIntV(page_num,"page_num", &dc)){
            return reply_error(reply, ERROR_MISS_PARAM, "miss param : page_num");
        }
        auto deviceVec = sqlite_utils_->queryDevice(page_size, page_num);
        int total = sqlite_utils_->getDeviceTotalCount();
        Document docObj;
        docObj.SetObject();
        docObj.AddMember("code", 0, docObj.GetAllocator());
        docObj.AddMember("msg", "success", docObj.GetAllocator());

        Value data = Rjson::rObject();
        data.AddMember("total", total, docObj.GetAllocator());
        Value items = Rjson::rArray();
        // 构建返回数据
        for (auto item : deviceVec) {
            Value device = Rjson::rObject();
            device.AddMember("server_sip_id",item->serverSipId, docObj.GetAllocator());
            device.AddMember("server_port",item->serverPort, docObj.GetAllocator());
            device.AddMember("server_ip",item->serverIp, docObj.GetAllocator());
            device.AddMember("device_sip_id",item->deviceSipId, docObj.GetAllocator());
            device.AddMember("device_name",item->deviceName, docObj.GetAllocator());
            device.AddMember("local_port",item->localPort, docObj.GetAllocator());
            device.AddMember("device_status",item->deviceStatus == 0? false:true, docObj.GetAllocator());
            device.AddMember("created_at",item->createdAt, docObj.GetAllocator());
            items.PushBack(device, docObj.GetAllocator());
        }

        data.AddMember("items", items, docObj.GetAllocator());

        docObj.AddMember("data", data, docObj.GetAllocator());

        rapidjson::StringBuffer strBuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
        docObj.Accept(writer);
        reply = strBuf.GetString();
    }
    catch (GeneralException2& e) {
        std::string err_str = fmt::format("error:{}, {}", e.err_code(), e.err_str());
        return reply_error(reply, ERROR_INVALID_PARAM, err_str);
    }
    catch (std::exception& e) {
        return reply_error(reply, ERROR_INVALID_PARAM, e.what());
    }
}

// private
bool DeviceManager::isInMap(const std::string &sip){
    auto mapPtr = getDeviceMap();
    if(mapPtr->find(sip) != mapPtr->end()){
        return true;
    }
    return false;
}

void DeviceManager::insertDevice(const std::string &key, std::shared_ptr<VirtualDeviceI> device){
    std::lock_guard<std::mutex> lock(map_lock_);
    if(device_map_.use_count()!=0){
        MapPtr newMapPtr(new Map(*device_map_));
        device_map_.swap(newMapPtr);
    }
    (*device_map_)[key] = device;
}

void DeviceManager::deleteDevice(const std::string &key) {
        std::lock_guard<std::mutex> lock(map_lock_);
        if(device_map_.use_count()!=0){
            MapPtr newMapPtr(new Map(*device_map_));
            auto iter = newMapPtr->find(key);
            if(iter!=newMapPtr->end()){
                deleteFile(iter->second->getConfig()->filePath);
                newMapPtr->erase(iter);
            }
            device_map_.swap(newMapPtr);
        }
}

NS_END