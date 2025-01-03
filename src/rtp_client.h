//
// Created by liwen on 2024-12-24.
//

#ifndef GB28181_CLIENT_RTP_CLIENT_H
#define GB28181_CLIENT_RTP_CLIENT_H

#include "log_helper.h"
#include <memory>
#include "rtp_client_i.h"

NS_BEGIN

class DeviceConfig;
class NetConnect;
class CallerParam;

class RtpClient {
public:
    RtpClient();
    virtual ~RtpClient();
    void init(std::shared_ptr<DeviceConfig> config, std::shared_ptr<RtpConnectDelegate> delegate);
    int startPushStream(const std::string &callId, std::shared_ptr<CallerParam> rtpParam);
    void close();
private:
    void process();
private:
    std::shared_ptr<NetConnect> net_connect_;
    std::shared_ptr<DeviceConfig> device_config_;
    std::shared_ptr<CallerParam> rtp_param_;
    std::thread process_th_;
    std::atomic_bool is_runing_{false};
    std::string call_id_;
    std::weak_ptr<RtpConnectDelegate> delegate_;

};

NS_END

#endif //GB28181_CLIENT_RTP_CLIENT_H
