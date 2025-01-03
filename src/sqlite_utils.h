/******************************************************************
 * Filename: sqlite_utils.h
 * Author: liwen
 * Description: sqlite database business layer encapsulation
 * Version: 1.0.0
 * Created: 2024-12-30
 * Last Modified: 2024-12-30
 ******************************************************************/
//

#ifndef GB28181_CLIENT_SQLITE_UTILS_H
#define GB28181_CLIENT_SQLITE_UTILS_H

#include "log_helper.h"

namespace SQLite{
    class Database;
}

NS_BEGIN

#define TABLE_NAME "device_info"

class DeviceConfig;

using DeviceVec = std::vector<std::shared_ptr<DeviceConfig>>;

class SQLiteUtils {
public:
    SQLiteUtils();
    ~SQLiteUtils();
    bool initDB(const std::string &tableName);
    bool addDeviceInfo(std::shared_ptr<DeviceConfig> deviceConfig);
    bool removeDeviceInfo(const std::string &deviceSipId);
    bool modifyDeviceStatus(const std::string &deviceSipId, const int deviceStatus);
    DeviceVec queryDevice(int pageSize, int pageNum);
    DeviceVec queryAllDevice();
    int getDeviceTotalCount();
    bool isExistLocalHost(int localPort);
private:

private:
    std::unique_ptr<SQLite::Database> sqlite_db_;
    std::mutex db_mutex_;
    std::string table_name_;
};

NS_END

#endif //GB28181_CLIENT_SQLLITE_UTILS_H
