/******************************************************************
 * Filename: file_utils.h
 * Author: liwen
 * Description: common file operation functions.
 * Version: 1.0.0
 * Created: 2025-01-02
 * Last Modified: 2025-01-02
 ******************************************************************/
//

#ifndef GB28181_CLIENT_FILE_UTILS_H
#define GB28181_CLIENT_FILE_UTILS_H

#include <string>

// 删除指定文件
extern void deleteFile(const std::string& filePath);

extern bool isFileExist(const std::string& filePath);

extern bool createDirRecursive(const std::string& path);

#endif //GB28181_CLIENT_FILE_UTILS_H
