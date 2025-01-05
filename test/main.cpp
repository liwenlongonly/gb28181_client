//
// Created by liwenlong on 2025-01-05.
//

#include "mp4_to_ps_rtp_test.h"

int main(int argc, const char* argv[]) {
    // 根据配置文件重新设置log
    ilong::LogHelper::logInit();
    Mp4ToPsRtpTest test;
    test.exec("./video/file/output.mp4");
}