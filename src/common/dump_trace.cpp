//
// Created by liwen on 2022/11/9.
//

#include "dump_trace.h"

#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <filesystem>

#if defined (__linux__)

#include <map>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <cxxabi.h>

#endif

NS_BEGIN

#define FORMAT_TIME_MAX_LEN         64 // 格式化时间最长长度
#define APP_NAME_MAX_LEN            128 // APP名称最大长度
#define APP_MAX_LEN                 1024 // 定义APP处理最长长度
#define RATIO_1000                  1000 // 1000的进率
#define FOLDER_MAX_LEN              512 // 文件夹长度最大值

// 信号处理的map集合
static const std::map<int, std::string> gsc_mp4Signals = {
        {SIGSEGV, "SIGSEGV"},
        {SIGABRT, "SIGABRT"},
        {SIGINT,  "SIGINT"},
        {SIGFPE,  "SIGFPE"},
        {SIGILL,  "SIGILL"},
        {SIGSYS,  "SIGSYS"},
        {SIGBUS,  "SIGBUS"}
        // 可以添加其他信号
};

/*************************************************************
 * 功能：获取应用运行目录，不包含应用名称
 * 输入参数：
 *		pPath：存放应用运行目录的内存空间
 *		uiLen：存放应用运行目录的内存空间长度，字节
 * 输出参数：
 *		pPath：存放应用运行目录的内存空间
 * ***********************************************************/
void getAppRunPath(char *pPath, unsigned int uiLen) {
    if (!pPath) {
        return;
    }

    memset(pPath, 0, uiLen);
    std::string strPath = "";

    char szAbsPath[APP_MAX_LEN] = {0};
    ssize_t iPathLen = readlink("/proc/self/exe", szAbsPath, APP_MAX_LEN); // 获取可执行程序的绝对路径
    if (0 < iPathLen && iPathLen < APP_MAX_LEN) {
        std::string strFullPath(szAbsPath);
        size_t iPos = strFullPath.find_last_of('/');
        if (iPos != std::string::npos) {
            strPath = strFullPath.substr(0, iPos); // return the directory without the file name
        }
    }
    memcpy(pPath, strPath.data(), (uiLen < strPath.length() ? uiLen : strPath.length()));
}


/*************************************************************
 * 功能：创建文件目录
 * 输入参数：
 *		pFolderPath：文件目录
 * 返回值：
 *		bool：创建结果，true -- 成功，false -- 失败
 * ***********************************************************/
bool createDirectory(const char *pFolderPath) {
    if (!pFolderPath) {
        return false;
    }

    bool bRet = true;

    // 创建目录，函数方式
    if (0 != access(pFolderPath, 0)) {
        // 返回0表示创建成功，-1表示失败
        if (0 > mkdir(pFolderPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO)) {
            bRet = false;
        }
    }

    return bRet;
}


/*************************************************************
 * 功能：获取应用名称
 * 输入参数：
 *		pName：存放应用名称的内存空间
 *		uiLen：存放应用名称的内存空间长度，字节
 * 输出参数：
 *		pName：存放应用名称的内存空间
 * ***********************************************************/
void getAppName(char *pName, unsigned int uiLen) {
    if (!pName) {
        return;
    }

    memset(pName, 0, uiLen);
    std::string strAppName = "unknown";

    char szAbsPath[APP_MAX_LEN] = {0};
    ssize_t iPathLen = readlink("/proc/self/exe", szAbsPath, APP_MAX_LEN); // 获取可执行程序的绝对路径
    if (0 < iPathLen && iPathLen < APP_MAX_LEN) {
        std::string strFullPath(szAbsPath);
        size_t iPos = strFullPath.find_last_of('/');
        if (iPos != std::string::npos) {
            strAppName = strFullPath.substr(iPos + 1);
        }
    }

    memcpy(pName, strAppName.data(), (uiLen < strAppName.length() ? uiLen : strAppName.length()));
}

/*************************************************************
 * 功能：获取格式化时间，格式为yyyyMMdd_HHmmss.zzz
 * 返回值：
 *		std::string：格式化时间
 * ***********************************************************/
static std::string getFormatTime() {
    time_t stTimeNow;
    time(&stTimeNow);
    char szTmp[FORMAT_TIME_MAX_LEN] = {0};

    strftime(szTmp, sizeof(szTmp), "%Y%m%d_%H%M%S", localtime(&stTimeNow));

    int iMillsec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    iMillsec = tv.tv_usec / RATIO_1000;

    char szTime[FORMAT_TIME_MAX_LEN] = {0};
    sprintf(szTime, "%s_%03d", szTmp, iMillsec);
    std::string strTime(szTime);

    return strTime;
}

ExceptionHandlerCB exception_handler_cb_;

/*************************************************************
 * 功能：注册异常处理，linux系统
 * 输入参数：
 *		signum：信号值
 *      info：信息
 *      ctx：上下文
 * ***********************************************************/
static void pfnExceptionHandler_linux(int signum, siginfo_t *info, void *ctx) {
    if(exception_handler_cb_){
        exception_handler_cb_(signum);
    }
    // 创建dump日志文件夹
    char szAppFolder[FOLDER_MAX_LEN] = {0};
    getAppRunPath(szAppFolder, sizeof(szAppFolder));
    std::string strFolder = std::string(szAppFolder) + "/dump/";
    if (!createDirectory(strFolder.data())) {
        return;
    }

    // 写入异常自定义头部信息
    std::ostringstream oss;
    oss << "Stack Trace: " << std::endl;
    oss << "Signal (" << signum << "), " << strsignal(signum) << std::endl;

    // 读取堆栈信息
    const size_t sFrameSize = 32; // 堆栈大小
    void *pStackBuffer[sFrameSize] = {0};
    int iSize = backtrace(pStackBuffer, sFrameSize);
    //backtrace_symbols_fd(pStackBuffer, iSize, STDOUT_FILENO);
    char **symbols = backtrace_symbols(pStackBuffer, iSize);
    if (!symbols) {
        return;
    }

    // 解析函数符号信息
    for (int i = 0; i < iSize; ++i) {
        char *mangleName = 0;
        char *offsetBegin = 0;
        char *offsetEnd = 0;

        for (char *p = symbols[i]; *p; ++p) {
            if ('(' == *p) {
                mangleName = p;
            } else if ('+' == *p) {
                offsetBegin = p;
            } else if (')' == *p) {
                offsetEnd = p;
                break;
            }
        }

        if (mangleName && offsetBegin && offsetEnd && (mangleName < offsetBegin)) {
            *mangleName++ = '\0';
            *offsetBegin++ = '\0';
            *offsetEnd++ = '\0';

            int status = -4;
            // 获取混淆解析后可读的函数符号信息
            char *retRealName = abi::__cxa_demangle(mangleName, nullptr, nullptr, &status); // 解析符号，得到真正的函数名
            if (0 == status && retRealName) {
                oss << "[Bt" << i << "] status(" << status << "), " << symbols[i] << ": " << retRealName << " + ";
            } else {
                oss << "[Bt" << i << "] status(" << status << "), " << symbols[i] << ": " << mangleName << " + ";
            }

            oss << offsetBegin << offsetEnd << std::endl;

            if (retRealName) {
                free(retRealName);
                retRealName = nullptr;
            }
        } else {
            oss << "[Bt" << i << "] status(-4), " << symbols[i] << std::endl;
        }
    }
    free(symbols);

    oss << std::endl;

    // 异常写入文件
    char szAppName[APP_NAME_MAX_LEN] = {0};
    getAppName(szAppName, sizeof(szAppName));
    std::string strAppName(szAppName);
    std::string strFilePath = strFolder + "core-" + strAppName + "-" + getFormatTime();
    std::ofstream fout(strFilePath.data());
    do {
        if (!(fout.is_open())) {
            break;
        }

        fout << oss.str().data();
        fout.close();

    } while (0);

    signal(signum, SIG_DFL); // 还原默认的信号处理
    raise(signum);
}

/*************************************************************
 * 功能：注册异常处理，linux系统
 * ***********************************************************/
void registerExceptionHandler(ExceptionHandlerCB exhCb) {
    exception_handler_cb_ = exhCb;
    // 需要配合nm，addr2line等指令使用
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = &pfnExceptionHandler_linux;
    action.sa_flags = SA_SIGINFO;

    for (const auto &signals: gsc_mp4Signals) {
        if (0 > sigaction(signals.first, &action, NULL)) {
            printf("Error: sigaction failed!\n");
        }
    }
}

NS_END