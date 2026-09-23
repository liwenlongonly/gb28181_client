//
// Created by liwen on 2024-12-23.
//

#include <netinet/in.h>
#include <sstream>
#include "sip_client.h"
#include "eXosip2/eXosip.h"
#include <osip2/osip.h>
#include "pugixml.hpp"
#include <tuple>
#include "device_cfg.h"
#include "stream_event_monitor.h"

NS_BEGIN

SipClient::SipClient() {

}

SipClient::~SipClient() {

}

int SipClient::start(std::shared_ptr<DeviceConfig> config) {
    device_config_ = config;
    is_running_ = true;
    LOG_INFO(SIP_LOG, "sip init begin.");
    sip_context_ = eXosip_malloc();

    if (OSIP_SUCCESS != eXosip_init(sip_context_)) {
        LOG_ERROR(SIP_LOG, "sip init failed.");
        eXosip_quit(sip_context_);
        sip_context_ = nullptr;
        return -1;
    }

    if (OSIP_SUCCESS != eXosip_listen_addr(sip_context_, IPPROTO_UDP, nullptr, device_config_->localPort, AF_INET, 0)) {
        LOG_CRITICAL(SIP_LOG, "sip port bind failed.");
        eXosip_quit(sip_context_);
        sip_context_ = nullptr;
        return -2;
    }

    std::ostringstream from_uri;
    std::ostringstream contact;
    std::ostringstream proxy_uri;

    // local ip & port
    char buffer[128] = {0};
    eXosip_guess_localip(sip_context_, AF_INET, buffer, sizeof(buffer));
    local_ip_ = std::string(buffer);
    LOG_INFO(SIP_LOG, "local ip is {}", local_ip_);

    from_uri << "sip:" << device_config_->deviceSipId << "@" << local_ip_ << ":" << device_config_->localPort;
    contact << "sip:" << device_config_->deviceSipId << "@" << local_ip_ << ":" << device_config_->localPort;
    proxy_uri << "sip:" << device_config_->serverSipId << "@" << device_config_->serverIp << ":" << device_config_->serverPort;

    from_sip_ = from_uri.str();
    to_sip_ = proxy_uri.str();

    LOG_INFO(SIP_LOG, "from uri is {}", from_sip_);
    LOG_INFO(SIP_LOG, "contact is {}", contact.str());
    LOG_INFO(SIP_LOG, "proxy_uri is {}", to_sip_);

    // clear auth
    eXosip_clear_authentication_info(sip_context_);

    osip_message_t *register_message = nullptr;
    register_id_ = eXosip_register_build_initial_register(sip_context_, from_sip_.c_str(),
                                                          to_sip_.c_str(),
                                                          contact.str().c_str(), 3600, &register_message);

    if (nullptr == register_message) {
        LOG_ERROR(SIP_LOG, "eXosip_register_build_initial_register failed");
        eXosip_quit(sip_context_);
        sip_context_ = nullptr;
        return -3;
    }
    eXosip_lock(sip_context_);
    eXosip_register_send_register(sip_context_, register_id_, register_message);
    eXosip_unlock(sip_context_);

    loop_th_ = std::thread(&SipClient::processRequest, this);

    std::thread heartbeat_task_thread(&SipClient::heartbeatTask, this);
    heartbeat_task_thread.detach();
    return 0;
}

void SipClient::close() {
    is_register_ = false;
    is_running_ = false;
    if (loop_th_.joinable()) {
        loop_th_.join();
    }
    if(sip_context_){
        eXosip_lock(sip_context_);
        // 1. 创建注销请求
        osip_message_t *unreg = nullptr;
        int ret = eXosip_register_build_register(sip_context_, register_id_, 0, &unreg); // 0 表示 Expires 为 0，即注销
        if (ret != 0) {
            LOG_ERROR(SIP_LOG, "failed to build unregister request");
        }
        // 2. 设置必要的头部（可选）
        // osip_message_set_header(unreg, "User-Agent", "MyCamera/1.0");
        // 3. 发送注销请求
        ret = eXosip_register_send_register(sip_context_, register_id_, unreg);
        if (ret != 0) {
            LOG_ERROR(SIP_LOG,"failed to send unregister request.");
        } else {
            LOG_INFO(SIP_LOG,"unregister request sent success.");
        }
        eXosip_register_remove(sip_context_, register_id_);
        eXosip_unlock(sip_context_);

        eXosip_quit(sip_context_);
        sip_context_ = nullptr;
    }
    LOG_INFO(SIP_LOG,"SipClient::close() finish");
}

void SipClient::setEventDelegate(std::shared_ptr<StreamEventMonitor> monitor) {
    stream_event_monitor_ = monitor;
}

void SipClient::sendBye(int did) {
    if (!sip_context_ || did < 0) {
        LOG_ERROR(SIP_LOG, "sendBye: invalid context or did={}", did);
        return;
    }
    osip_message_t *bye = nullptr;
    eXosip_lock(sip_context_);
    int ret = eXosip_call_build_request(sip_context_, did, "BYE", &bye);
    if (ret != 0 || bye == nullptr) {
        eXosip_unlock(sip_context_);
        LOG_ERROR(SIP_LOG, "sendBye: build BYE request failed, did={}", did);
        return;
    }
    ret = eXosip_call_send_request(sip_context_, did, bye);
    eXosip_unlock(sip_context_);
    if (ret != 0) {
        LOG_ERROR(SIP_LOG, "sendBye: send BYE failed, did={}", did);
    } else {
        LOG_INFO(SIP_LOG, "sendBye: BYE sent successfully, did={}", did);
    }
}

void SipClient::processRequest() {
    while (is_running_) {
        auto evt = std::shared_ptr<eXosip_event_t>(
                eXosip_event_wait(sip_context_, 0, 100),eXosip_event_free);

        eXosip_lock(sip_context_);
        eXosip_automatic_action(sip_context_);
        eXosip_unlock(sip_context_);

        if (evt == nullptr) {
            continue;
        }

        osip_message_t *msg = evt->request; // 获取请求消息
        std::string callID;
        if (msg != NULL) {
            // 解析消息中的 Call-ID
            osip_call_id_t * call_id = osip_message_get_call_id(msg);
            callID = std::string(call_id->number);
        }

        switch (evt->type) {
            case eXosip_event_type::EXOSIP_REGISTRATION_SUCCESS: {
                LOG_INFO(SIP_LOG, "{}---recv REGISTRATION_SUCCESS",device_config_->deviceSipId);
                is_register_ = true;
                break;
            }
            case eXosip_event_type::EXOSIP_REGISTRATION_FAILURE: {
                LOG_INFO(SIP_LOG, "{}---recv REGISTRATION_FAILURE",device_config_->deviceSipId);
                if (evt->response == nullptr) {
                    LOG_ERROR(SIP_LOG, "register 401 has no response !!!");
                    break;
                }

                if (401 == evt->response->status_code) {
                    osip_www_authenticate_t *www_authenticate_header;

                    osip_message_get_www_authenticate(evt->response, 0, &www_authenticate_header);

                    if (eXosip_add_authentication_info(sip_context_,
                                                       device_config_->deviceSipId.c_str(),
                                                       device_config_->username.c_str(),
                                                       device_config_->password.c_str(),
                                                       "MD5", www_authenticate_header->realm)) {
                        LOG_ERROR(SIP_LOG, "{}---register add auth failed!",device_config_->deviceSipId);
                        break;
                    }else{
                        LOG_INFO(SIP_LOG, "{}---register add auth success!",device_config_->deviceSipId);
                    }
                }
                break;
            }
            case eXosip_event_type::EXOSIP_MESSAGE_NEW: {
                LOG_INFO(SIP_LOG, "{}----recv MESSAGE_NEW",device_config_->deviceSipId);

                if (MSG_IS_MESSAGE(evt->request)) {
                    osip_body_t *body = nullptr;
                    osip_message_get_body(evt->request, 0, &body);
                    if (body != nullptr) {
                        LOG_INFO(SIP_LOG, "new message request: \n{}", body->body);
                    }

                    this->sendResponseOk(evt);

                    auto cmd_sn = this->get_cmd(body->body);
                    std::string cmd = std::get<0>(cmd_sn);
                    std::string sn = std::get<1>(cmd_sn);
                    LOG_INFO(SIP_LOG, "{}---recv new cmd: {}", device_config_->deviceSipId, cmd);
                    if ("Catalog" == cmd) {
                        this->processCatalogQuery(sn);
                    } else if ("DeviceStatus" == cmd) {
                        this->processDeviceStatusQuery(sn);
                    } else if ("DeviceInfo" == cmd) {
                        this->processDeviceInfoQuery(sn);
                    } else if ("DeviceControl" == cmd) {
                        this->processDeviceControlQuery(sn, body->body);
                    } else if ("RecordInfo" == cmd) {
                        this->processRecordInfoQuery(sn);
                    } else if ("PlayBackControl" == cmd) {
                        this->processPlaybackControlQuery(sn, body->body);
                    } else {
                        LOG_ERROR(SIP_LOG, "unhandled cmd: {}", cmd);
                    }
                } else if (MSG_IS_BYE(evt->request)) {
                    LOG_INFO(SIP_LOG, "recv BYE message");
                    this->sendResponseOk(evt);
                    break;
                }
                break;
            }
            case eXosip_event_type::EXOSIP_CALL_INVITE: {
                LOG_INFO(SIP_LOG, "{}---got CALL_INVITE", device_config_->deviceSipId);

                auto sdp_msg = eXosip_get_remote_sdp(sip_context_, evt->did);
                if (!sdp_msg) {
                    LOG_ERROR(SIP_LOG, "eXosip_get_remote_sdp failed");
                    break;
                }

                auto connection = eXosip_get_video_connection(sdp_msg);
                if (!connection) {
                    LOG_ERROR(SIP_LOG, "eXosip_get_video_connection failed");
                    break;
                }

                std::string rtp_ip = connection->c_addr;

                auto video_sdp = eXosip_get_video_media(sdp_msg);
                if (!video_sdp) {
                    LOG_ERROR(SIP_LOG, "eXosip_get_video_media failed");
                    break;
                }

                int rtp_port = atoi(video_sdp->m_port);

                LOG_INFO(SIP_LOG, "{}---rtp server: {}:{}",device_config_->deviceSipId ,rtp_ip, rtp_port);

                std::string rtp_protocol = video_sdp->m_proto;

                LOG_INFO(SIP_LOG, "rtp protocol: {}", rtp_protocol);

                osip_body_t *sdp_body = NULL;
                osip_message_get_body(evt->request, 0, &sdp_body);
                if (nullptr == sdp_body) {
                    LOG_ERROR(SIP_LOG, "osip_message_get_body failed");
                    break;
                }

                int localRtpPort = 0;
                if(auto delegate = stream_event_monitor_.lock()){
                    localRtpPort = delegate->onAllocRtpPort();
                }

                std::string body = sdp_body->body;

                // 解析 SDP 中的 session name (s= 字段)
                std::string sessionName = "Play";
                auto s_index = body.find("s=");
                if (s_index != std::string::npos) {
                    auto s_line = body.substr(s_index + 2);
                    auto s_end = s_line.find("\r\n");
                    if (s_end != std::string::npos) {
                        sessionName = s_line.substr(0, s_end);
                    }
                }
                LOG_INFO(SIP_LOG, "session name: {}", sessionName);

                // 回放模式下，解析时间范围 (t= 字段)
                if ("Playback" == sessionName) {
                    auto t_index = body.find("t=");
                    if (t_index != std::string::npos) {
                        auto t_line = body.substr(t_index + 2);
                        auto t_end = t_line.find("\r\n");
                        if (t_end != std::string::npos) {
                            std::string timeRange = t_line.substr(0, t_end);
                            LOG_INFO(SIP_LOG, "playback time range: {}", timeRange);
                        }
                    }
                }

                auto y_sdp_first_index = body.find("y=");
                auto y_sdp = body.substr(y_sdp_first_index);
                auto y_sdp_last_index = y_sdp.find("\r\n");
                std::string ssrc = y_sdp.substr(2, y_sdp_last_index - 1);
                uint32_t result = static_cast<uint32_t>(std::stoul(ssrc));
                LOG_INFO(SIP_LOG, "ssrc: {}", result);

                caller_param_ = std::make_shared<CallerParam>(rtp_ip, rtp_port,
                                                            rtp_protocol, result,
                                                            localRtpPort, sessionName);

                std::stringstream ss;
                ss << "v=0\r\n";
                ss << "o=" << device_config_->deviceSipId << " 0 0 IN IP4 " << local_ip_ << "\r\n";
                ss << "s=" << sessionName << "\r\n";
                ss << "c=IN IP4 " << local_ip_ << "\r\n";
                ss << "t=0 0\r\n";
                if (rtp_protocol == "TCP/RTP/AVP") {
                    ss << "m=video " << localRtpPort << " TCP/RTP/AVP 96\r\n";
                } else {
                    ss << "m=video " << localRtpPort << " RTP/AVP 96\r\n";
                }
                ss << "a=sendonly\r\n";
                ss << "a=rtpmap:96 PS/90000\r\n";
                ss << "y=" << ssrc << "\r\n";
                std::string sdp_output_str = ss.str();

                osip_message_t *message = evt->request;
                int status = eXosip_call_build_answer(sip_context_, evt->tid, 200, &message);

                if (status != 0) {
                    LOG_ERROR(SIP_LOG, "call invite build answer failed");
                    break;
                }

                osip_message_set_content_type(message, "APPLICATION/SDP");
                osip_message_set_body(message, sdp_output_str.c_str(), sdp_output_str.size());

                eXosip_call_send_answer(sip_context_, evt->tid, 200, message);

                break;
            }
            case eXosip_event_type::EXOSIP_CALL_ACK: {
                std::string sessionType = caller_param_ ? caller_param_->sessionName : "Unknown";
                LOG_INFO(SIP_LOG, "EXOSIP_CALL_ACK tag:{}--{} session:{} did:{} begin pushing rtp stream...", from_sip_, callID, sessionType, evt->did);
                // 存储对话ID，用于回放完成后发送 BYE
                if(caller_param_){
                    caller_param_->dialogId = evt->did;
                }
                auto delegate = stream_event_monitor_.lock();
                if(caller_param_ && delegate){
                    delegate->onStartPushStream(callID, caller_param_);
                }
                break;
            }
            case eXosip_event_type::EXOSIP_CALL_CLOSED: {
                LOG_INFO(SIP_LOG, "EXOSIP_CALL_CLOSED tag:{}--{} stop pushing rtp stream...", from_sip_, callID);
                if(auto delegate = stream_event_monitor_.lock()){
                    delegate->onStopPushStream(callID);
                }
                break;
            }
            case eXosip_event_type::EXOSIP_MESSAGE_ANSWERED: {
                if (evt->response == nullptr) {
                    LOG_ERROR(SIP_LOG, "answred has no response !!!");
                    break;
                }

                if (200 == evt->response->status_code) {
                    LOG_INFO(SIP_LOG, "recv MESSAGE_ANSWERED: Ok");
                }
                break;
            }

            default: {
                LOG_INFO(SIP_LOG, "unhandled sip evt type: {} info:{}", evt->type, evt->textinfo);
                break;
            }
        }
    }
}

void SipClient::processCatalogQuery(std::string sn) {
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>Catalog</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<SumNum>" << 1 << "</SumNum>\r\n";
    ss << "<DeviceList Num=\"" << 1 << "\">\r\n";
    ss << "<Item>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Manufacturer>" << device_config_->manufacture << "</Manufacturer>\r\n";
    ss << "<Status>ON</Status>\r\n";
    ss << "<Name>"<< device_config_->deviceName <<"</Name>\r\n";
    ss << "<ParentID>" << device_config_->serverSipId << "</ParentID>\r\n";
    ss << "</Item>\r\n";
    ss << "</DeviceList>\r\n";
    ss << "</Response>\r\n";
    LOG_INFO(SIP_LOG, "Catalog response: \n{}", ss.str());
    auto request = createMsg();
    if (request != NULL) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

void SipClient::processDeviceStatusQuery(std::string sn) {
    std::stringstream ss;

    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char curtime[72] = {0};
    sprintf(curtime, "%d-%d-%dT%02d:%02d:%02d", (timeinfo->tm_year + 1900), (timeinfo->tm_mon + 1),
            timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>DeviceStatus</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Result>OK</Result>\r\n";
    ss << "<Online>ONLINE</Online>\r\n";
    ss << "<Status>OK</Status>\r\n";
    ss << "<DeviceTime>" << curtime << "</DeviceTime>\r\n";
    ss << "<Alarmstatus Num=\"0\">\r\n";
    ss << "</Alarmstatus>\r\n";
    ss << "<Encode>ON</Encode>\r\n";
    ss << "<Record>OFF</Record>\r\n";
    ss << "</Response>\r\n";

    LOG_INFO(SIP_LOG, "devicestatus response: \n{}", ss.str());
    auto request = createMsg();
    if (request != NULL) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

void SipClient::processDeviceInfoQuery(std::string sn) {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>DeviceInfo</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Result>OK</Result>\r\n";
    ss << "<DeviceType>simulate client</DeviceType>\r\n";
    ss << "<Manufacturer>"<< device_config_->manufacture << "</Manufacturer>\r\n";
    ss << "<Model>28181</Model>\r\n";
    ss << "<Firmware>fireware</Firmware>\r\n";
    ss << "<MaxCamera>1</MaxCamera>\r\n";
    ss << "<MaxAlarm>0</MaxAlarm>\r\n";
    ss << "</Response>\r\n";

    LOG_INFO(SIP_LOG, "deviceinfo response: \n{}", ss.str());
    auto request = createMsg();
    if (request != NULL) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

void SipClient::processDeviceControlQuery(std::string sn, const std::string &body) {
    // 解析控制命令类型
    pugi::xml_document document;
    if (!document.load_string(body.c_str())) {
        LOG_ERROR(SIP_LOG, "processDeviceControlQuery: cannot parse xml body");
        return;
    }

    pugi::xml_node root_node = document.first_child();
    if (!root_node) {
        LOG_ERROR(SIP_LOG, "processDeviceControlQuery: cannot get root node");
        return;
    }

    // 识别具体的设备控制命令
    std::string controlCmd;
    if (root_node.child("PTZCmd")) {
        controlCmd = "PTZCmd";
        std::string ptzCmd = root_node.child_value("PTZCmd");
        LOG_INFO(SIP_LOG, "DeviceControl PTZCmd: {}", ptzCmd);
    } else if (root_node.child("TeleBoot")) {
        controlCmd = "TeleBoot";
        LOG_INFO(SIP_LOG, "DeviceControl TeleBoot: {}", root_node.child_value("TeleBoot"));
    } else if (root_node.child("RecordCmd")) {
        controlCmd = "RecordCmd";
        LOG_INFO(SIP_LOG, "DeviceControl RecordCmd: {}", root_node.child_value("RecordCmd"));
    } else if (root_node.child("GuardCmd")) {
        controlCmd = "GuardCmd";
        LOG_INFO(SIP_LOG, "DeviceControl GuardCmd: {}", root_node.child_value("GuardCmd"));
    } else if (root_node.child("AlarmCmd")) {
        controlCmd = "AlarmCmd";
        LOG_INFO(SIP_LOG, "DeviceControl AlarmCmd: {}", root_node.child_value("AlarmCmd"));
    } else if (root_node.child("IFameCmd")) {
        controlCmd = "IFameCmd";
        LOG_INFO(SIP_LOG, "DeviceControl IFameCmd: {}", root_node.child_value("IFameCmd"));
    } else {
        controlCmd = "Unknown";
        LOG_WARN(SIP_LOG, "DeviceControl: unrecognized control command");
    }

    // 构建 DeviceControl 响应
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>DeviceControl</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Result>OK</Result>\r\n";
    ss << "</Response>\r\n";

    LOG_INFO(SIP_LOG, "DeviceControl response: \n{}", ss.str());
    auto request = createMsg();
    if (request != NULL) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

void SipClient::processRecordInfoQuery(std::string sn) {
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>RecordInfo</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<SumNum>" << 1 << "</SumNum>\r\n";
    ss << "<RecordList Num=\"" << 1 << "\">\r\n";
    ss << "<Item>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Name>record1</Name>\r\n";
    ss << "<StartTime>" << device_config_->recordStartTime << "</StartTime>\r\n";
    ss << "<EndTime>" << device_config_->recordEndTime << "</EndTime>\r\n";
    ss << "<Secrecy>0</Secrecy>\r\n";
    ss << "<Type>time</Type>\r\n";
    ss << "<FileSize>0</FileSize>\r\n";
    ss << "</Item>\r\n";
    ss << "</RecordList>\r\n";
    ss << "</Response>\r\n";

    LOG_INFO(SIP_LOG, "RecordInfo response: \n{}", ss.str());
    auto request = createMsg();
    if (request != nullptr) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

void SipClient::processPlaybackControlQuery(std::string sn, const std::string &body) {
    // 解析回放控制 XML
    pugi::xml_document document;
    if (!document.load_string(body.c_str())) {
        LOG_ERROR(SIP_LOG, "processPlaybackControlQuery: cannot parse xml body");
        return;
    }

    pugi::xml_node root_node = document.first_child();
    if (!root_node) {
        LOG_ERROR(SIP_LOG, "processPlaybackControlQuery: cannot get root node");
        return;
    }

    // 识别回放控制动作
    std::string action;
    if (root_node.child("Play")) {
        action = "Play";
        LOG_INFO(SIP_LOG, "PlaybackControl: Play (resume)");
    } else if (root_node.child("Pause")) {
        action = "Pause";
        LOG_INFO(SIP_LOG, "PlaybackControl: Pause");
    } else if (root_node.child("Stop")) {
        action = "Stop";
        LOG_INFO(SIP_LOG, "PlaybackControl: Stop");
    } else if (root_node.child("PlaySeek")) {
        action = "PlaySeek";
        std::string seekTime = root_node.child_value("PlaySeek");
        LOG_INFO(SIP_LOG, "PlaybackControl: PlaySeek to {}", seekTime);
    } else if (root_node.child("PlaySpeed")) {
        action = "PlaySpeed";
        std::string speed = root_node.child_value("PlaySpeed");
        LOG_INFO(SIP_LOG, "PlaybackControl: PlaySpeed x{}", speed);
    } else if (root_node.child("PlayRewind")) {
        action = "PlayRewind";
        std::string speed = root_node.child_value("PlayRewind");
        LOG_INFO(SIP_LOG, "PlaybackControl: PlayRewind x{}", speed);
    } else {
        action = "Unknown";
        LOG_WARN(SIP_LOG, "PlaybackControl: unrecognized control action");
    }

    // 构建 PlayBackControl 响应
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
    ss << "<Response>\r\n";
    ss << "<CmdType>PlayBackControl</CmdType>\r\n";
    ss << "<SN>" << sn << "</SN>\r\n";
    ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
    ss << "<Result>OK</Result>\r\n";
    ss << "</Response>\r\n";

    LOG_INFO(SIP_LOG, "PlaybackControl response: \n{}", ss.str());
    auto request = createMsg();
    if (request != nullptr) {
        osip_message_set_content_type(request, "Application/MANSCDP+xml");
        osip_message_set_body(request, ss.str().c_str(), strlen(ss.str().c_str()));
        sendRequest(request);
    }
}

int SipClient::get_sn() {
    if (sn >= SN_MAX) {
        sn = 0;
    }
    sn++;
    return sn;
}

void SipClient::heartbeatTask() {
    while (true) {
        if (is_register_) {
            std::stringstream ss;
            ss << "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n";
            ss << "<Notify>\r\n";
            ss << "<CmdType>Keepalive</CmdType>\r\n";
            ss << "<SN>" << get_sn() << "</SN>\r\n";
            ss << "<DeviceID>" << device_config_->deviceSipId << "</DeviceID>\r\n";
            ss << "<Status>OK</Status>\r\n";
            ss << "</Notify>\r\n";
            std::string kpStr = ss.str();
            LOG_INFO(SIP_LOG,"keepalive sip: \n {}", kpStr);
            osip_message_t *request = createMsg();
            if (request != NULL) {
                osip_message_set_content_type(request, "Application/MANSCDP+xml");
                osip_message_set_body(request, kpStr.c_str(), strlen(kpStr.c_str()));
                sendRequest(request);
                LOG_INFO(SIP_LOG, "sent heartbeat finsh.");
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

osip_message_t *SipClient::createMsg() {

    osip_message_t *request = nullptr;
    auto status = eXosip_message_build_request(sip_context_, &request, "MESSAGE",
                                               to_sip_.c_str(), from_sip_.c_str(), nullptr);
    if (OSIP_SUCCESS != status) {
        LOG_ERROR(SIP_LOG, "build request failed: {}", status);
    }

    return request;
}

void SipClient::sendRequest(osip_message_t *request) {
    eXosip_lock(sip_context_);
    eXosip_message_send_request(sip_context_, request);
    eXosip_unlock(sip_context_);
}

void SipClient::sendResponse(std::shared_ptr<eXosip_event_t> evt, osip_message_t *msg) {
    eXosip_lock(sip_context_);
    eXosip_message_send_answer(sip_context_, evt->tid, 200, msg);
    eXosip_unlock(sip_context_);
}

void SipClient::sendResponseOk(std::shared_ptr<eXosip_event_t> evt) {
    auto msg = evt->request;
    eXosip_message_build_answer(sip_context_, evt->tid, 200, &msg);
    sendResponse(evt, msg);
}

std::tuple<std::string, std::string> SipClient::get_cmd(const char *body) {
    pugi::xml_document document;

    if (!document.load_string(body)) {
        LOG_ERROR(SIP_LOG, "cannot parse the xml");
        return std::make_tuple("", "");
    }

    pugi::xml_node root_node = document.first_child();

    if (!root_node) {
        LOG_ERROR(SIP_LOG, "cannot get root node of xml");
        return std::make_tuple("", "");
    }

    std::string root_name = root_node.name();
    if ("Query" != root_name && "Control" != root_name) {
        LOG_ERROR(SIP_LOG, "invalid query xml with root: {}", root_name);
        return std::make_tuple("", "");
    }

    auto cmd_node = root_node.child("CmdType");

    if (!cmd_node) {
        LOG_ERROR(SIP_LOG, "cannot get the cmd type");
        return std::make_tuple("", "");
    }

    auto sn_node = root_node.child("SN");

    if (!sn_node) {
        LOG_ERROR(SIP_LOG, "cannot get the SN");
        return std::make_tuple("", "");
    }

    std::string cmd = cmd_node.child_value();
    std::string sn = sn_node.child_value();

    return make_tuple(cmd, sn);
}

NS_END
