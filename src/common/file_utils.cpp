#include "file_utils.h"
#include <filesystem>
#include "log_helper.h"

namespace fs = std::filesystem;

void deleteFile(const std::string& filePath) {
    try {
        // 检查文件是否存在
        if (fs::exists(filePath)) {
            // 检查是否是文件而不是目录
            if (fs::is_regular_file(filePath)) {
                fs::remove(filePath); // 删除文件
                LOG_INFO(MSG_LOG,"文件删除成功: {}", filePath);
            } else {
                LOG_ERROR(MSG_LOG,"指定路径不是一个普通文件: {}", filePath);
            }
        } else {
            LOG_ERROR(MSG_LOG,"文件不存在: {}", filePath);
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR(MSG_LOG,"文件删除失败: {}", e.what());
    }
}

bool isFileExist(const std::string& filePath){
    try {
        // 检查文件是否存在
        if (fs::exists(filePath)) {
            return true;
        } else {
            return false;
        }
    } catch (const fs::filesystem_error& e) {
        return false;
    }
}

bool createDirRecursive(const std::string& path) {
    try {
        // 使用 std::filesystem::create_directories 创建文件夹及其父目录
        if (fs::create_directories(path)) {
            return true;
        }
    } catch (const fs::filesystem_error& e) {
        return false;
    }
    return true;
}
