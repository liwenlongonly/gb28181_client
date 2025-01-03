/******************************************************************
 * Filename: stream_event_monitor.h
 * Author: ilong
 * Description: rtp stream event monitor
 * Version: 1.0.0
 * Created: 2024-12-26
 * Last Modified: 2024-12-26
 ******************************************************************/
//
#ifndef GB28181_CLIENT_STREAM_EVENT_MONITOR_H
#define GB28181_CLIENT_STREAM_EVENT_MONITOR_H

#include <memory>
#include "macro_definition.h"

NS_BEGIN

class CallerParam;

class StreamEventMonitor{
public:
    StreamEventMonitor(){}
    virtual ~StreamEventMonitor(){}

    virtual int onAllocRtpPort() = 0;

    virtual void onStartPushStream(const std::string &callId, std::shared_ptr<CallerParam> param) = 0;

    virtual void onStopPushStream(const std::string &callId) = 0;
};

NS_END

#endif //GB28181_CLIENT_STREAM_EVENT_MONITOR_H
