#include "basic_error.h"
#include <unordered_map>
#include <string>

static std::unordered_map<int, std::string>g_error_msg =
{
	{ ERROR_OK, "success" },
	{ ERROR_UNKOWN, "Inner Error" },				// ERROR_UNKOWN
	{ ERROR_ALLOC_MEMORY, "Memory Error" },				// ERROR_ALLOC_MEMORY
	{ ERROR_INVALID_PARAM, "Invalid Param" },			// ERROR_INVALID_PARAM
	{ ERROR_MISS_PARAM, "Miss Param" },				// ERROR_MISS_PARAM
	{ ERROR_SESSION_NOT_FOUND, "Session Not Found" },		// ERROR_SESSION_NOT_FOUND
	{ ERROR_FILE_OPEN_FAILED, "File Open Failed" },			// ERROR_FILE_OPEN_FAILED
	{ ERROR_VIDEO_INFO_NOT_FOUND, "Video Info Not Found" },		// ERROR_VIDEO_INFO_NOT_FOUND
    { ERROR_DEVICE_NOT_FOUND, "Device  Not Found" },
    { ERROR_NO_SUPPORT_MUTIPLE_FILES, "Not Support Mutiple Files" },
    { ERROR_EXEC_DATA_BASE, "Insert Data Base Fail." },
    { ERROR_DEVICE_ALREADY_EXISTS, "The Device Already Exists." },
    { ERROR_LOCAL_HOST_ALREADY_EXISTS, "The Local Host Already Exists." },
    { ERROR_FILE_NOT_FOUND, "File Not Found." },
};

const char* get_error_msg(int error_code)
{
	auto it = g_error_msg.find(error_code);
	if (it != g_error_msg.end())
	{
		return it->second.c_str();
	}
	else
	{
		return "";
	}
}