# gb28181_client

国标 gb28181 模拟客户端平台支持多设备, web端使用Vue + element-plus

- 支持 Linux

## 编译

```
cd gb28281_client

mkdir build && cd build
cmake ..
make -j8 && make install
```

## 运行
```
cd ./install/gb28281_client
./process_manager.sh start all
```
## 查看运行状态
```
./process_manager.sh status all
```
## 结束进程
```
./process_manager.sh stop all
```

## web端使用打包
```
cd ./web_src
npm install 
npm run build
```

## web端部署nginx使用做代理
```
location /api {
        rewrite ^.+api/?(.*)$ /gb28181/v1/$1 break;
        include uwsgi_params;
		proxy_pass  http://192.168.110.130:9080;
}
```