/******************************************************************
 * Filename: http_session.h
 * Author: ilong
 * Description: http 业务类.
 * Version: 1.0.0
 * Created: 2024-12-26
 * Last Modified: 2024-12-26
 ******************************************************************/
//

#ifndef GB28181_CLIENT_HTTP_SESSION_H
#define GB28181_CLIENT_HTTP_SESSION_H

#include "log_helper.h"

namespace httplib{
    class Server;
};

NS_BEGIN

class HttpSession final {
public:
    HttpSession();
    ~HttpSession();
    void init();
    bool run(int port);
    void stop();
private:
    std::shared_ptr<httplib::Server> http_server_;
};

NS_END

#endif //GB28181_CLIENT_HTTP_SESSION_H
