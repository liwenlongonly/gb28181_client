//
// Created by liwen on 2024-12-23.
//

#include "gb28181_device.h"
#include "device_cfg.h"
#include "sip_client.h"
#include "rtp_client.h"
#include "net_connect.h"

NS_BEGIN

GB28181Device::GB28181Device() {

}

GB28181Device::~GB28181Device() {

}

void GB28181Device::init(std::shared_ptr<DeviceConfig> config) {
    device_config_ = config;
    sip_client_ = std::make_shared<SipClient>();
    sip_client_->setEventDelegate(shared_from_this());
}

int GB28181Device::start() {
    if(sip_client_){
      return sip_client_->start(device_config_);
    }
    return -1;
}

void GB28181Device::close() {
    if(sip_client_){
        sip_client_->close();
    }
    std::shared_ptr<Map> mapPtr;
    {
        std::lock_guard<std::mutex> lock(map_lock_);
        mapPtr = std::make_shared<Map>(rtp_client_map_);
        rtp_client_map_.clear();
    }
    for (auto[key, value]:*mapPtr) {
        value->close();
    }
}

int GB28181Device::onAllocRtpPort() {
    return NetConnect::allocSocketPort();
}

void GB28181Device::onStartPushStream(const std::string &callId, std::shared_ptr<CallerParam> param) {
    {
        std::lock_guard<std::mutex> lock(map_lock_);
        auto iter = rtp_client_map_.find(callId);
        if(iter != rtp_client_map_.end()){
            LOG_WARN(MSG_LOG, "callid already exits.");
            return;
        }
    }
    auto rtpClient = std::make_shared<RtpClient>();
    rtpClient->init(device_config_, shared_from_this());
    rtpClient->startPushStream(callId, param);
    {
        std::lock_guard<std::mutex> lock(map_lock_);
        rtp_client_map_[callId] = rtpClient;
    }
}

void GB28181Device::onStopPushStream(const std::string &callId) {
    std::shared_ptr<RtpClient> rtpClient;
    {
        std::lock_guard<std::mutex> lock(map_lock_);
        auto iter = rtp_client_map_.find(callId);
        if(iter != rtp_client_map_.end()){
            rtpClient = iter->second;
            rtp_client_map_.erase(iter);
        }
    }
    if(rtpClient){
        rtpClient->close();
    }
}

void GB28181Device::onNetConnectError(const std::string &callID) {
    // 在一个新的线程中释放
    std::thread([this,callID]{
        this->onStopPushStream(callID);
    }).detach();
}

std::shared_ptr<DeviceConfig> GB28181Device::getConfig() {
        return device_config_;
}

NS_END