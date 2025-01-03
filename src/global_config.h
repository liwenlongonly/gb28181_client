//
// Created by jqq on 2022/10/25.
//

#ifndef HTTPSERVER_GLOBAL_CONFIG_H
#define HTTPSERVER_GLOBAL_CONFIG_H

#include "Singleton.h"
#include "macro_definition.h"
#include "log_helper.h"

NS_BEGIN

class GlobalConfig final {
public:
    ~GlobalConfig();

    struct {
        int http_port{8080};
    }server;

    // 视频文件的存储路径
    struct {
        std::string path;
    }video;

    struct {
        spdlog::level::level_enum level{spdlog::level::info};
        int max_size_mb{50};
        int max_count{3};
    }log;

private:
    GlobalConfig();
    friend Singleton<GlobalConfig>;
};

NS_END

#endif //HTTPSERVER_GLOBAL_CONFIG_H
