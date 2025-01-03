// 设备相关请求

//导入request.js请求工具
import request from '@/utils/request.js'

// 创建设备接口的函数
export const deviceCreateService = (params)=>{
    const jsonParams = {
        "server_sip_id":params.serverSipId,
        "register_domain":params.registerDomain,
        "server_ip":params.serverIp,
        "server_port":parseInt(params.serverPort),
        "device_sip_id":params.deviceSipId,
        "local_port": parseInt(params.localPort),
        "username":params.username,
        "password":params.password,
        "manufacture":'',
        "device_name":params.deviceName,
        "file_path":params.filePath
    };
    return request.post('/device/create', jsonParams);
}

// 设备启动的接口的函数
export const deviceStartService = (sip)=>{
    const jsonParams = {
        "device_sip_id":sip,
    };
    return request.post('/device/start', jsonParams);
}

// 设备关闭的接口的函数
export const deviceStopService = (sip)=>{
    const jsonParams = {
        "device_sip_id":sip,
    };
    return request.post('/device/stop', jsonParams);
}

// 设备删除的接口的函数
export const deviceDeleteService = (sip)=>{
    const jsonParams = {
        "device_sip_id":sip,
    };
    return request.post('/device/delete', jsonParams);
}

// 设备列表的接口的函数
export const deviceListService = (params)=>{
    const jsonParams = {
        "page_size":params.pageSize,
        "page_num":params.pageNum
    };
    return request.post('/device/list', jsonParams);
}