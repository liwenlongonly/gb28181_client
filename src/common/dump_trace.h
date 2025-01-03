//
// Created by liwen on 2022/11/9.
//

#ifndef HTTPSERVER_DUMP_TRACE_H
#define HTTPSERVER_DUMP_TRACE_H

#include "macro_definition.h"
#include "functional"
#include <signal.h>

NS_BEGIN

using ExceptionHandlerCB = std::function<void(int signum)>;

/*************************************************************
 * 功能：注册异常处理，linux系统
 * ***********************************************************/
extern void registerExceptionHandler(ExceptionHandlerCB exhCb);

NS_END

#endif //HTTPSERVER_DUMP_TRACE_H
