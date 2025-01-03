#include "sqlite_utils.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <string>
#include "device_cfg.h"
#include "sqlite3.h"

NS_BEGIN

std::string initSQLStr = R"(
CREATE TABLE IF NOT EXISTS {} (
   id INTEGER PRIMARY KEY AUTOINCREMENT, -- 自增主键
   server_sip_id VARCHAR(128) NOT NULL, -- 信令网关 sip id
   server_ip VARCHAR(128) NOT NULL, -- 信令网关 ip
   server_port INTEGER NOT NULL, -- 信令网关端口
   device_sip_id VARCHAR(128) NOT NULL, -- 设备 sip id
   local_port VARCHAR(255) NOT NULL, -- 本地信令端口
   username VARCHAR(255) NOT NULL, -- 用户名
   password VARCHAR(255) NOT NULL, -- 密码
   manufacture VARCHAR(255) NOT NULL, -- 生产厂商
   device_name VARCHAR(255) NOT NULL, -- 设备名称
   file_path VARCHAR(255) NOT NULL, -- 文件路径
   device_status INTEGER DEFAULT 0, -- 设备状态，0-未启动，1-启动
   created_at DATETIME DEFAULT (datetime('now'))
);
)";

std::string getSqliteMode(int value){
    static std::map<int, std::string> mode = {
            {0,"单线程模式"},
            {1,"多线程模式"},
            {2,"串行化模式"}
    };
    if(mode.find(value)!= mode.end()){
        return mode[value];
    }
    return {};
}

SQLiteUtils::SQLiteUtils() {

}

SQLiteUtils::~SQLiteUtils() {

}

bool SQLiteUtils::initDB(const std::string &tableName) {
    table_name_ = tableName;
    try {
        LOG_INFO(SQL_LOG,"SQLite threadsafe mode: {}", getSqliteMode(sqlite3_threadsafe()));
        // 初始化 SQLite 数据库
        sqlite_db_ = std::make_unique<SQLite::Database>("device_info.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        std::string sqlStr = fmt::format(initSQLStr, tableName);
        // 按分号分割 SQL 语句
        size_t pos = 0;
        while ((pos = sqlStr.find(';')) != std::string::npos) {
            std::string sqlStatement = sqlStr.substr(0, pos);
            sqlStr.erase(0, pos + 1);
            // 跳过空白语句
            if (sqlStatement.find_first_not_of(" \t\n\r") != std::string::npos) {
                sqlite_db_->exec(sqlStatement);
            }
        }
        LOG_INFO(SQL_LOG, "SQL file executed successfully!");
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return false;
    }
    return true;
}

bool SQLiteUtils::addDeviceInfo(std::shared_ptr<DeviceConfig> deviceCfg) {
    std::string insertStr = "INSERT INTO {} (server_sip_id, server_ip, server_port,"
                            "device_sip_id, local_port, username, password, manufacture,"
                            "device_name ,file_path) VALUES ";
    std::string sqlStr = fmt::format(insertStr, table_name_);
    sqlStr += fmt::format("('{}', '{}', {}, '{}', {}, '{}', '{}', '{}', '{}', '{}');", deviceCfg->serverSipId,
                          deviceCfg->serverIp, deviceCfg->serverPort, deviceCfg->deviceSipId, deviceCfg->localPort,
                          deviceCfg->username, deviceCfg->password, deviceCfg->manufacture, deviceCfg->deviceName,
                          deviceCfg->filePath);
    LOG_INFO(SQL_LOG, "insert sql: {}", sqlStr);
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        sqlite_db_->exec(sqlStr);
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return false;
    }
    return true;
}

bool SQLiteUtils::removeDeviceInfo(const std::string &deviceSipId) {
    std::string delStr = "DELETE FROM {} WHERE device_sip_id = '{}';";
    std::string sqlStr = fmt::format(delStr, table_name_, deviceSipId);
    LOG_INFO(SQL_LOG, "delete sql: {}", sqlStr);
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        sqlite_db_->exec(sqlStr);
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return false;
    }
    return true;
}

bool SQLiteUtils::modifyDeviceStatus(const std::string &deviceSipId, const int deviceStatus) {
    std::string exeStr = "UPDATE {} "
                         "SET device_status = {} "
                         "WHERE device_sip_id = '{}';";
    std::string sqlStr = fmt::format(exeStr, table_name_, deviceStatus, deviceSipId);
    LOG_INFO(SQL_LOG,"modify sql: {}", sqlStr);
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        sqlite_db_->exec(sqlStr);
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return false;
    }
    return true;
}

DeviceVec SQLiteUtils::queryDevice(int pageSize, int pageNum) {
    std::string exeStr{
        "SELECT * FROM {} ORDER BY created_at DESC LIMIT {} OFFSET {};"
    };
    std::string sqlStr = fmt::format(exeStr, table_name_, pageSize, (pageNum -1)* pageSize);
    LOG_INFO(SQL_LOG,"query sql: {}", sqlStr);
    DeviceVec deviceVec;
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        SQLite::Statement query(*sqlite_db_, sqlStr);
        while(query.executeStep()) {  // 执行查询
            auto deviceCfg = std::make_shared<DeviceConfig>();
            deviceCfg->serverSipId = query.getColumn(1).getString();
            deviceCfg->serverIp = query.getColumn(2).getString();
            deviceCfg->serverPort = query.getColumn(3).getInt();
            deviceCfg->deviceSipId= query.getColumn(4).getString();
            deviceCfg->localPort = query.getColumn(5).getInt();
            deviceCfg->username = query.getColumn(6).getString();
            deviceCfg->username = query.getColumn(7).getString();
            deviceCfg->manufacture = query.getColumn(8).getString();
            deviceCfg->deviceName = query.getColumn(9).getString();
            deviceCfg->filePath = query.getColumn(10).getString();
            deviceCfg->deviceStatus = query.getColumn(11).getInt();
            deviceCfg->createdAt = query.getColumn(12).getString();
            deviceVec.push_back(deviceCfg);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return {};
    }
    return std::move(deviceVec);
}

DeviceVec SQLiteUtils::queryAllDevice(){
    std::string exeStr{
            "SELECT * FROM {}"
    };
    std::string sqlStr = fmt::format(exeStr, table_name_);
    LOG_INFO(SQL_LOG,"query sql: {}", sqlStr);
    DeviceVec deviceVec;
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        SQLite::Statement query(*sqlite_db_, sqlStr);
        while(query.executeStep()) {  // 执行查询
            auto deviceCfg = std::make_shared<DeviceConfig>();
            deviceCfg->serverSipId = query.getColumn(1).getString();
            deviceCfg->serverIp = query.getColumn(2).getString();
            deviceCfg->serverPort = query.getColumn(3).getInt();
            deviceCfg->deviceSipId= query.getColumn(4).getString();
            deviceCfg->localPort = query.getColumn(5).getInt();
            deviceCfg->username = query.getColumn(6).getString();
            deviceCfg->username = query.getColumn(7).getString();
            deviceCfg->manufacture = query.getColumn(8).getString();
            deviceCfg->deviceName = query.getColumn(9).getString();
            deviceCfg->filePath = query.getColumn(10).getString();
            deviceCfg->deviceStatus = query.getColumn(11).getInt();
            deviceCfg->createdAt = query.getColumn(12).getString();
            deviceVec.push_back(deviceCfg);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return {};
    }
    return std::move(deviceVec);
}

int SQLiteUtils::getDeviceTotalCount() {
    std::string exeStr = "SELECT COUNT(*) FROM {};";
    std::string sqlStr = fmt::format(exeStr, table_name_);
    int pageCount{};
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        SQLite::Statement query(*sqlite_db_, sqlStr);
        if (query.executeStep()) {  // 执行查询
            pageCount = query.getColumn(0).getInt();  // 获取第一列的值
        }
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return pageCount;
    }
    return pageCount;
}

bool SQLiteUtils::isExistLocalHost(int localPort){
    std::string exeStr = "SELECT COUNT(*) FROM {} WHERE local_port = {};";
    std::string sqlStr = fmt::format(exeStr, table_name_, localPort);
    int count{0};
    try {
        std::lock_guard<std::mutex> lock(db_mutex_);
        SQLite::Statement query(*sqlite_db_, sqlStr);
        if (query.executeStep()) {  // 执行查询
            count = query.getColumn(0).getInt();  // 获取第一列的值
        }
    } catch (const std::exception& e) {
        LOG_ERROR(SQL_LOG,"SQLite exception: {}", e.what());
        return false;
    }
    return count == 0? false: true;
}

NS_END
