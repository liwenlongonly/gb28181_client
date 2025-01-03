//
// Created by jqq on 2022/10/25.
//

#ifndef HTTPSERVER_MACRO_DEFINITION_H
#define HTTPSERVER_MACRO_DEFINITION_H

#ifdef __cplusplus
#define NS_BEGIN                              namespace ilong {
#define NS_END                                }
#define USING_NS                              using namespace ilong;
#define NS                                    ::ilong
#else
#define NS_BEGIN
#define NS_END
#define USING_NS
#define NS
#endif

#endif //HTTPSERVER_MACRO_DEFINITION_H
