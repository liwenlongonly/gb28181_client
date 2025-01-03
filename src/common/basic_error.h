#ifndef _BASIC_ERROR_H_
#define _BASIC_ERROR_H_


#define ERROR_OK                           0       // 正确执行
#define ERROR_UNKOWN				       -1		// 内部错误
#define ERROR_ALLOC_MEMORY			       -2		// 分配内存失败
#define ERROR_INVALID_PARAM			       -3		// 参数格式无效
#define ERROR_MISS_PARAM			       -4		// 缺少参数
#define ERROR_SESSION_NOT_FOUND		       -5		// 会话不存在
#define ERROR_FILE_OPEN_FAILED		       -6		// 文件打开失败
#define ERROR_VIDEO_INFO_NOT_FOUND	       -7		// 视频信息未找到
#define ERROR_DEVICE_NOT_FOUND    	       -8		// 设备未找到
#define ERROR_NO_SUPPORT_MUTIPLE_FILES     -9		// 不支持多文件上传
#define ERROR_EXEC_DATA_BASE               -10		// 操作数据库设备失败
#define ERROR_DEVICE_ALREADY_EXISTS        -11		// 设备已存在
#define ERROR_LOCAL_HOST_ALREADY_EXISTS    -12		// 端口已存在
#define ERROR_FILE_NOT_FOUND	           -13		// 文件未找到

const char* get_error_msg(int error_code);

#endif //!_BASIC_ERROR_H_
