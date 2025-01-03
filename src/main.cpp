#include <string>
#include "cxxopts.h"
#include "SimpleIni.h"
#include "global_config.h"
#include "http_session.h"
#include "dump_trace.h"
#include "device_manager.h"
#include "file_utils.h"

USING_NS

int main(int argc, const char* argv[]) {
    // 初始 log 初始化
    ilong::LogHelper::logBasicInit();
    LOG_INFO(MAIN_LOG,"{}", startLogStr);

    // 捕获异常信号 关闭 http server
    HttpSession httpSession;
    registerExceptionHandler([&httpSession](int signum){
        if(SIGTERM == signum){
            LOG_INFO(MAIN_LOG, "program active exit!");
        } else{
            LOG_ERROR(MAIN_LOG, "program abnormal exit!");
        }
        // 关闭socket 连接
        httpSession.stop();
    });

    auto config = Singleton<GlobalConfig>::instance();

    cxxopts::ParseResult options_result;
    try {
        cxxopts::Options options(argv[0], "a gb28181 client - mainly used for testing");
        options.add_options()
            ("h, help", "Print usage")
            ("conf", "config file path", cxxopts::value<std::string>())
            ;
        options_result = options.parse(argc, argv);
        if (options_result.count("help"))
        {
            std::cout << options.help() << std::endl;
            exit(0);
        }

    } catch (const cxxopts::OptionException& e) {
        LOG_ERROR(MAIN_LOG,"error parsing options: {}", e.what());
        exit(1);
    }
    // 读取配置文件的地址
    std::string conf;
    if (!options_result.count("conf")) {
        conf = "cfg.ini";
    } else {
        conf = options_result["conf"].as<std::string>();
    }
    LOG_INFO(MAIN_LOG,"read main arg {} success .", conf);
    // 加载配置文件
    CSimpleIniA ini;
    SI_Error rc = ini.LoadFile(conf.c_str());
    if(rc < 0){
        LOG_ERROR(MAIN_LOG,"load {} ini file error code: {} .", conf, rc);
        exit(0);
    }
    LOG_INFO(MAIN_LOG,"load {} file success .", conf);

    config->server.http_port = ini.GetLongValue("server","http_port", 8080);
    config->video.path = ini.GetValue("video", "path", "/data/video");
    // 递归创建文件视频文件存储目录
    createDirRecursive(config->video.path);
    LOG_INFO(MAIN_LOG,"video file save path: {} .", config->video.path);
    config->log.level = spdlog::level::from_str(ini.GetValue("log","level", "info"));
    config->log.max_count = ini.GetLongValue("log", "max_count", 15);
    config->log.max_size_mb = ini.GetLongValue("log", "max_size_mb", 50);

    // 根据配置文件重新设置log
    ilong::LogHelper::logInit();

    // 从数据库加载虚拟设备信息
    auto deviceManager = Singleton<DeviceManager>::instance();
    deviceManager->initFromDB();

    // 启动http server
    LOG_INFO(MAIN_LOG, "http server run with port: {} .", config->server.http_port);
    httpSession.init();
    bool ret = httpSession.run(config->server.http_port);
    if(!ret){
        LOG_ERROR(MAIN_LOG,"http server run error with port: {} .", config->server.http_port);
    }
}