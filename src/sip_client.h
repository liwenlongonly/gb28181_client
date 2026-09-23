//
// Created by liwen on 2024-12-23.
//

#ifndef GB28181_CLIENT_SIP_CLIENT_H
#define GB28181_CLIENT_SIP_CLIENT_H

#include "log_helper.h"
#include "eXosip2/eXosip.h"

NS_BEGIN

class DeviceConfig;
class StreamEventMonitor;

class CallerParam{
public:
    CallerParam(){};
    CallerParam(const std::string &rtpIp,
                const int rtpPort,
                const std::string &rtpProtocol,
                const uint32_t &ssrc,
                const int localRtpPort,
                const std::string &sessionName = "Play"):rtpIp{rtpIp},
                rtpPort{rtpPort},
                rtpProtocol{rtpProtocol},
                ssrc{ssrc},
                localRtpPort{localRtpPort},
                sessionName{sessionName}{
    }
    std::string rtpIp;
    int localRtpPort{};
    int rtpPort{};
    std::string rtpProtocol;
    uint32_t ssrc;
    // 会话名称: "Play" 实时点播, "Playback" 回放
    std::string sessionName;
    // eXosip 对话ID，用于回放完成后发送 BYE
    int dialogId{-1};
};

class SipClient {
public:
    SipClient();
    ~SipClient();
    int start(std::shared_ptr<DeviceConfig> config);
    void close();

    void setEventDelegate(std::shared_ptr<StreamEventMonitor> monitor);

    // 主动发送 BYE 终止通话（用于回放完成后）
    void sendBye(int did);

private:
    void processRequest();

    void processCatalogQuery(std::string sn);

    void processDeviceInfoQuery(std::string sn);

    void processDeviceStatusQuery(std::string sn);

    void processDeviceControlQuery(std::string sn, const std::string &body);

    void processRecordInfoQuery(std::string sn);

    void processPlaybackControlQuery(std::string sn, const std::string &body);

    void heartbeatTask();

    void sendRequest(osip_message_t * request);

    void sendResponse(std::shared_ptr<eXosip_event_t> evt, osip_message_t * msg);

    osip_message_t * createMsg();

    void sendResponseOk(std::shared_ptr<eXosip_event_t> evt);

    std::tuple<std::string, std::string> get_cmd(const char * body);
    int get_sn();
private:

    eXosip_t* sip_context_;

    std::string from_sip_;
    std::string to_sip_;

    std::string local_ip_;
    std::shared_ptr<DeviceConfig> device_config_;
    std::shared_ptr<CallerParam> caller_param_;

    std::atomic_bool is_register_{false};
    std::atomic_bool is_running_{false};
    int register_id_{};
    std::thread loop_th_;

    std::weak_ptr<StreamEventMonitor> stream_event_monitor_;
    int SN_MAX = 99999999;
    int sn;

};

NS_END

#endif //GB28181_CLIENT_SIP_CLIENT_H
