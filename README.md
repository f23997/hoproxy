# hoproxy
HTTP Tunnel Proxy Supports arbitrary domain camouflage / disguise, and forwards traffic to any specified target IP:port over TCP.

Normal proxies only CONNECT to the real IP + port.
hoproxy can accept requests for any domain name, and then forward them to the specified backend TCP IP + port.
-
一般代理只会connect真实ip端口

hoproxy 可以收到任意请求域名，然后转发到指定的后端tcp ip端口
-

转发后面的地址需要有服务
​服务端ubuntu

部署建议

#​安装#

 apt install socat gcc unzip -y


wget https://github.com/user-attachments/files/24694287/hoproxy1.0.zip

解压#
unzip hoproxy1.0.zip

 

 
 
#编译#
gcc hoproxy.c -o hoproxy

配合支持http-proxy的客户端openvpn ssh proxytunnel混淆流量



//
openvpn只需要在客户端修改添加
remote 这里任意域名 80 
http-proxy 这里填写代理ip 80


remote Any domain name allowed here 80 
http-proxy Enter the proxy IP here
Proxy IP goes here 80
//
​启动命令（转发到目标地址）：


# -d: 后台运行
# -l: 监听 80 端口
# -r: 转发到ip的1194 端口
运行它#
./hoproxy -d -l 80 -r 127.0.0.1:1194


-




