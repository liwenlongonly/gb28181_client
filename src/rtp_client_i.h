/******************************************************************
 * Filename: rtp_client_i.h
 * Author: liwen
 * Description:
 * Version: 1.0.0
 * Created: 2025-01-02
 * Last Modified: 2025-01-02
 ******************************************************************/
//

#ifndef GB28181_CLIENT_RTP_CLIENT_I_H

#include "macro_definition.h"
#include <string>

class RtpConnectDelegate{
public:
    RtpConnectDelegate(){}
    virtual ~RtpConnectDelegate(){}
    virtual void onNetConnectError(const std::string &callID) = 0;
};

#define GB28181_CLIENT_RTP_CLIENT_I_H

#endif //GB28181_CLIENT_RTP_CLIENT_I_H
