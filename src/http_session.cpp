#include "rapidjson/Rjson.hpp"
#include "http_session.h"
#include "httplib.h"
#include "device_manager.h"
#include "global_config.h"

using namespace rapidjson;

NS_BEGIN

    struct FileItem {
        FILE *filep;
        std::string file_name;
        std::string key;
        std::string file_path;
    };

    uint64_t inline GetTimestampMs()
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        return ms.count();
    }


HttpSession::HttpSession() {

}

HttpSession::~HttpSession() {

}

void HttpSession::init() {
    http_server_ = std::make_shared<httplib::Server>();
    auto deviceManager = Singleton<DeviceManager>::instance();
    auto config = Singleton<GlobalConfig>::instance();
    http_server_->set_logger([](const httplib::Request &req, const httplib::Response &res) {
        std::string params;
        for (auto &item: req.params) {
            params += "{" + item.first + "," + item.second + "} ";
        }
        if(params.empty()){
            params = "{}";
        }
        std::string reqBody;
        if(!req.body.empty()){
            reqBody = req.body;
        }else{
            reqBody = "{}";
        }
        LOG_INFO(HTTP_LOG,"http request path: {}  param: {} body: {} resp:{} .end", req.path, params, reqBody, res.body);
    });

    http_server_->set_exception_handler([](const httplib::Request &req, httplib::Response &res, std::exception_ptr ep) {
        std::string buf;
        try {
            std::rethrow_exception(ep);
        } catch (std::exception &e) {
            reply_error(buf, -1, e.what());
        } catch (...) {  // See the following NOTE
            reply_error(buf, -1, "Unknown Exception");
        }
        res.set_content(buf, "application/json");
        res.status = httplib::StatusCode::InternalServerError_500;
    });

    // 创建GB28181设备接口
    http_server_->Post("/gb28181/v1/device/create", [deviceManager](const httplib::Request &req, httplib::Response &res) {
        std::string reply;
        deviceManager->deviceCreate(req.body, reply);
        res.set_content(reply, "application/json");
    });

    // 启动GB28181设备接口
    http_server_->Post("/gb28181/v1/device/start", [deviceManager](const httplib::Request &req, httplib::Response &res) {
        std::string reply;
        deviceManager->deviceStart(req.body, reply);
        res.set_content(reply, "application/json");
    });

    // 关闭GB28181设备接口
    http_server_->Post("/gb28181/v1/device/stop", [deviceManager](const httplib::Request &req, httplib::Response &res) {
        std::string reply;
        deviceManager->deviceStop(req.body, reply);
        res.set_content(reply, "application/json");
    });

    // 删除GB28181设备接口
    http_server_->Post("/gb28181/v1/device/delete", [deviceManager](const httplib::Request &req, httplib::Response &res) {
        std::string reply;
        deviceManager->deviceDelete(req.body, reply);
        res.set_content(reply, "application/json");
    });

    // 视频文件上传接口
    http_server_->Post("/gb28181/v1/file/upload", [config](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
        if (req.is_multipart_form_data()) {
            // NOTE: `content_reader` is blocking until every form data field is read
            std::vector<FileItem> files;
            content_reader(
                    [&](const httplib::MultipartFormData &file) {
                        std::string filePath = config->video.path + "/" + std::to_string(GetTimestampMs())  +"_"+ file.filename;
                        FILE *fileP = fopen(filePath.c_str(), "wb");
                        if (fileP != nullptr) {
                            files.emplace_back(FileItem{fileP, file.filename, file.name, filePath});
                            return true;
                        }else{
                            return false;
                        }
                    },
                    [&](const char *data, size_t data_length) {
                        fwrite(data, data_length, 1, files.back().filep);
                        return true;
                    });
            for (auto & item: files) {
                if(item.filep){
                    fclose(item.filep);
                    item.filep = nullptr;
                }
            }
            std::string reply;
            if(files.size() != 1){
                reply_error(reply, ERROR_NO_SUPPORT_MUTIPLE_FILES, "Does not support multiple files");
                res.set_content(reply, "application/json");
                return ;
            }else{
                Document wdc = Rjson::rWriteDC();
                Rjson::rAdd(wdc, "code", ERROR_OK);
                Rjson::rAdd(wdc, "message", StringRef("success"));
                Rjson::rAdd(wdc, "data", files.back().file_path);
                reply = Rjson::ToString(wdc);
                res.set_content(reply, "application/json");
            }
        }else{
            res.status = httplib::StatusCode::UnavailableForLegalReasons_451;
            return ;
        }
    });

    // 获取虚拟设备列表 分页
    http_server_->Post("/gb28181/v1/device/list", [deviceManager](const httplib::Request &req, httplib::Response &res) {
        std::string reply;
        deviceManager->deviceList(req.body, reply);
        res.set_content(reply, "application/json");
    });
}

bool HttpSession::run(int port) {
    if(http_server_){
        return http_server_->listen("0.0.0.0", port);
    }
    return false;
}

void HttpSession::stop() {
    if(http_server_){
        return http_server_->stop();
    }
}

NS_END