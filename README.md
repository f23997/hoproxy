# hoproxy
HTTP Tunnel Proxy Supports arbitrary domain camouflage / disguise, and forwards traffic to any specified target IP:port over TCP.


​服务端ubuntu

部署建议

#​安装
 apt install socat gcc
 
#编译
gcc hoproxy.c -o hoproxy

配合支持http-proxy的客户端openvpn ssh proxytunnel混淆流量

client
nobind
dev tun
remote-cert-tls server
remote 这里任意域名 80 tcp
http-proxy 这里填写代理ip 80




client
nobind
dev tun
remote-cert-tls server
remote Any domain name allowed here 80 tcp
http-proxy Enter the proxy IP here
Proxy IP goes here 80

​启动命令（转发到目标地址）：


# -d: 后台运行
# -l: 监听 8080 端口
# -r: 转发到ip的1194 端口
./hoproxy -d -l 80 -r 127.0.0.1:1194




