//
// Created by liwenlong on 2025-01-05.
//

#ifndef GB28181_CLIENT_MP4_TO_PS_RTP_TEST_H
#define GB28181_CLIENT_MP4_TO_PS_RTP_TEST_H

#include <string>
#include "log_helper.h"

class Mp4ToPsRtpTest {
public:
    Mp4ToPsRtpTest();
    ~Mp4ToPsRtpTest();
    void exec(const std::string &videoPath);
private:
    std::atomic_bool is_runing_{false};
};


#endif //GB28181_CLIENT_MP4_TO_PS_RTP_TEST_H
