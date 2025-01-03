/******************************************************************
 * Filename: virtual_device_i.h
 * Author: ilong
 * Description: Abstract interface class for virtual device
 * Version: 1.0.0
 * Created: 2024-12-26
 * Last Modified: 2024-12-26
 ******************************************************************/
//

#ifndef GB28181_CLIENT_VIRTUAL_DEVICE_I_H
#define GB28181_CLIENT_VIRTUAL_DEVICE_I_H

#include <memory>
#include "macro_definition.h"

NS_BEGIN

class DeviceConfig;

class VirtualDeviceI {
public:
    VirtualDeviceI(){}
    virtual ~VirtualDeviceI(){}
    virtual void init(std::shared_ptr<DeviceConfig> config) = 0;
    virtual std::shared_ptr<DeviceConfig> getConfig() = 0;
    virtual int start() = 0;
    virtual void close() = 0;
};

NS_END
#endif //GB28181_CLIENT_VIRTUAL_DEVICE_I_H
