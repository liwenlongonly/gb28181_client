//
// Created by jqq on 2022/10/25.
//

#ifndef HTTPSERVER_LOG_HELPER_H
#define HTTPSERVER_LOG_HELPER_H

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "macro_definition.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define MAIN_LOG "MainLog"
#define HTTP_LOG "HttpLog"
#define MSG_LOG  "MsgLog"
#define SIP_LOG  "SipLog"
#define SQL_LOG  "SQLLog"

#define MAIN_LOG_FILE "logs/mainLog.log"
#define HTTP_LOG_FILE "logs/httpLog.log"
#define MSG_LOG_FILE "logs/msgLog.log"
#define SIP_LOG_FILE "logs/sipLog.log"
#define SQL_LOG_FILE "logs/sqlLog.log"

#define GET_LOGGER(name) spdlog::get(name)

#define LOG_TRACE(name, ...) SPDLOG_LOGGER_TRACE(spdlog::get(name), __VA_ARGS__)
#define LOG_DEBUG(name, ...) SPDLOG_LOGGER_DEBUG(spdlog::get(name), __VA_ARGS__)
#define LOG_INFO(name,...) SPDLOG_LOGGER_INFO(spdlog::get(name), __VA_ARGS__)
#define LOG_WARN(name,...) SPDLOG_LOGGER_WARN(spdlog::get(name), __VA_ARGS__)
#define LOG_ERROR(name,...) SPDLOG_LOGGER_ERROR(spdlog::get(name), __VA_ARGS__)
#define LOG_CRITICAL(name,...) SPDLOG_LOGGER_CRITICAL(spdlog::get(name), __VA_ARGS__)

NS_BEGIN

extern std::string startLogStr;

class LogHelper {
public:
    LogHelper();
    ~LogHelper();
    static void logBasicInit();
    static void logInit();
private:
    static void createLogger(const std::string &tag,
                             const std::string &log_path,
                             const int &log_max_size_mb,
                             const int &log_max_count);
};

NS_END

#endif //HTTPSERVER_LOG_HELPER_H
