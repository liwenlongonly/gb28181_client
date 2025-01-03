//
// Created by liwen on 2024-12-23.
//

#ifndef GB28181_CLIENT_GB28181_DEVICE_H
#define GB28181_CLIENT_GB28181_DEVICE_H

#include <map>
#include "log_helper.h"
#include "stream_event_monitor.h"
#include "virtual_device_i.h"
#include "reflection_factory.hpp"
#include "rtp_client_i.h"

NS_BEGIN

class DeviceConfig;
class SipClient;
class RtpClient;

// 虚拟GB28181 摄像头
class GB28181Device : public VirtualDeviceI,
                      public StreamEventMonitor,
                      public RtpConnectDelegate,
                      public std::enable_shared_from_this<GB28181Device>{
public:
    using Map = std::map<std::string, std::shared_ptr<RtpClient>>;
    GB28181Device();
    virtual ~GB28181Device();
    virtual void init(std::shared_ptr<DeviceConfig> config) override;

    virtual std::shared_ptr<DeviceConfig> getConfig() override;

    virtual int start() override;
    virtual void close() override;

public:
    virtual int onAllocRtpPort() override;
    virtual void onStartPushStream(const std::string &callId, std::shared_ptr<CallerParam> param) override;
    virtual void onStopPushStream(const std::string &callId) override;

    virtual void onNetConnectError(const std::string &callID) override;

private:
    std::shared_ptr<DeviceConfig> device_config_;
    std::shared_ptr<SipClient> sip_client_;
    Map rtp_client_map_;
    std::mutex map_lock_;
};

NS_END

#endif //GB28181_CLIENT_GB28181_DEVICE_H
