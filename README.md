# hoproxy
HTTP Tunnel Proxy Supports arbitrary domain camouflage / disguise, and forwards traffic to any specified target IP:port over TCP.


​服务端

部署建议

​安装
 apt install socat gcc
编译
gcc hoproxy.c -o hoproxy
gcc hoproxy.c -o hoproxy





​启动命令（转发到目标地址）：


# -d: 后台运行
# -l: 监听 8080 端口
# -r: 转发到ip的1194 端口
./hoproxy -d -l 8080 -r 127.0.0.1:1194




