//
// Created by jqq on 2022/10/25.
//

#include "log_helper.h"
#include "global_config.h"

NS_BEGIN

std::string startLogStr = R"(
       GGGGG  BBBBB    2222    88888   1   88888   1
      G       B    B  2    2  8     8  1  8     8  1
      G  GG   BBBBB      2     88888   1   88888   1
      G    G  B    B   2      8     8  1  8     8  1
       GGGG   BBBBB   222222   88888   1   88888   1
)";

LogHelper::LogHelper() {

}

LogHelper::~LogHelper() {

}

void LogHelper::logBasicInit(){
    auto config = Singleton<GlobalConfig>::instance();

    LogHelper::createLogger(MAIN_LOG, MAIN_LOG_FILE, config->log.max_size_mb, config->log.max_count);

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_level(config->log.level);
}

void LogHelper::logInit() {

    auto config = Singleton<GlobalConfig>::instance();

    spdlog::drop(MAIN_LOG);

    LogHelper::createLogger(MAIN_LOG, MAIN_LOG_FILE, config->log.max_size_mb, config->log.max_count);
    LogHelper::createLogger(HTTP_LOG, HTTP_LOG_FILE, config->log.max_size_mb, config->log.max_count);
    LogHelper::createLogger(MSG_LOG, MSG_LOG_FILE, config->log.max_size_mb, config->log.max_count);
    LogHelper::createLogger(SIP_LOG, SIP_LOG_FILE, config->log.max_size_mb, config->log.max_count);
    LogHelper::createLogger(SQL_LOG, SQL_LOG_FILE, config->log.max_size_mb, config->log.max_count);

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_level(config->log.level);
}

void LogHelper::createLogger(const std::string &tag,
                             const std::string &log_path,
                             const int &log_max_size_mb,
                             const int &log_max_count) {

    auto sink1 = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink1->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%t][%l][%s:%#] %v%$");
    sink1->set_level(spdlog::level::debug);

    auto sink2 = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path, 1024 * 1024 * log_max_size_mb, log_max_count);
    sink2->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%t][%l][%s:%#] %v");
    sink2->set_level(spdlog::level::debug);

    std::vector<spdlog::sink_ptr> sinks = { sink1,sink2 };
    auto logger = std::make_shared<spdlog::logger>(tag, sinks.begin(), sinks.end());

    spdlog::register_logger(logger);

}

NS_END