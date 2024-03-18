#ifndef FUN_Boost_h
#define FUN_Boost_h

#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>

typedef enum 
{
//以下是TCPC的
     ConnectFail = 0,//连接服务器失败     
     DisconnectFail, //服务器断开失败     
     Connecting,     //连接中
     Disconnecting,  //断开中
//以下是TCPS的
     Listening,      //监听中
     ListenFail,     //监听失败
     CheckListen,    //判断侦听
     NoListen,       //没有监听
//以下是TCPC和TCPS共有的
     Connected,      //已连接
     Disconnected,   //已断开
//以下是UDP的
     StopReceData   //停止接收数据
}BoostStatus;

typedef enum
{
     B_TCPC = 0,
     B_TCPS,
     B_UDP
}SocketType;

typedef enum
{
     IPv4 = 0,
     IPv6
}IPVersion;

//定义回调函数的类型，使用std::function类型会比传统的函数指针会更灵活，使用会更方便
typedef std::function<void(SocketType,std::string, uint16_t, uint8_t*, uint16_t)> NetReceDataEvent;
typedef std::function<void(SocketType,BoostStatus, std::string, uint16_t)> NetStatusEvent;

//TCPS的通信类
class TCPSCommunicat
{
public:
     explicit TCPSCommunicat(std::unique_ptr<boost::asio::ip::tcp::socket>,\
                   SocketType,\
                   NetReceDataEvent,\
                   NetStatusEvent);
     ~TCPSCommunicat();

   
     //发送数据函数申明
     int32_t FUN_TCPSCommunicat_Send(uint8_t *, uint32_t, uint32_t);
     //断开函数申明
     int8_t FUN_TCPSCommunicat_Disconnect(void);


protected://仅派生类 和 类自身看到
     
     

public:
     BoostStatus ConnectState;//0表示未连接 非0表示连接
     std::string RemoteIP;//远程IP
     uint16_t RemotePort;//远程端口

private:     
     // 智能指针管理的 套接字指针
     std::unique_ptr<boost::asio::ip::tcp::socket> tcpsock_ptr;
     NetReceDataEvent ReceDataFun;
     NetStatusEvent StatusFun;
     // 智能指针管理的 线程对象指针
     std::unique_ptr<std::thread> Rece_thread_ptr;
     //
     SocketType SocketType_;
     

private:
     void TCPSCommunicat_ReceThreadMethod(void);
};

//此重定义必须放在TCPSCommunicat类之后，因为里面有用到TCPSCommunicat类!!!
typedef std::function<void(std::unique_ptr<TCPSCommunicat>,NetReceDataEvent*,NetStatusEvent*)> UserLinkedEvent;//用户接入事件

//TCPS类
class Boost_TCPS
{
public:
     explicit Boost_TCPS(NetStatusEvent,UserLinkedEvent);
     ~Boost_TCPS();
     //启动侦听函数的申明
     int8_t FUN_TCPS_StartListen(IPVersion, std::string lport, NetReceDataEvent, NetStatusEvent);
     int8_t FUN_TCPS_StartListen(IPVersion, uint16_t lport, NetReceDataEvent, NetStatusEvent);
     //继续侦听函数的申明
     void FUN_TCPS_ContinueListen(NetReceDataEvent, NetStatusEvent);
     //停止侦听函数的申明
     void FUN_TCPS_StopListen(void);
     


protected:
     void TCPS_ListenThreadMethod(IPVersion, uint16_t, NetReceDataEvent, NetStatusEvent);

private:
     boost::asio::io_context tcp_context;
     //智能指针管理 侦听线程对象
     std::unique_ptr<std::thread> Listen_thread_ptr;
     //
     std::unique_ptr<boost::asio::ip::tcp::acceptor> Acceptor_ptr;
     //
     NetReceDataEvent NrdFun;
     NetStatusEvent NsFun;
     //
     UserLinkedEvent UserLinkedFun;
     NetStatusEvent StatusFun;
     BoostStatus Statusflag;//
};

//TCPC类
class Boost_TCPC
{
public:
     explicit Boost_TCPC(NetReceDataEvent,NetStatusEvent);
     ~Boost_TCPC();
     //开始连接 函数申明
     int8_t FUN_TCPC_Connect(std::string rips, uint16_t rport);
     //停止连接 函数申明
     void FUN_TCPC_Disconnect(void);
     //发送数据函数申明
     int32_t FUN_TCPC_Send(uint8_t *, uint32_t, uint32_t);

private:
     boost::asio::io_context tcp_context;
     std::unique_ptr<boost::asio::ip::tcp::socket> TcpcSocket_Ptr;
     // 智能指针管理的 线程对象指针
     std::unique_ptr<std::thread> Rece_thread_ptr;   
     NetReceDataEvent ReceDataFun;
     NetStatusEvent StatusFun;
     BoostStatus ConnectState;

     void TCPC_ReceThreadMethod(void);
};

//UDP类
class Boost_UDP
{
public:
     explicit Boost_UDP(IPVersion, uint16_t, NetReceDataEvent, NetStatusEvent);
     ~Boost_UDP();
     //申明阻塞型发送数据方法
     int32_t FUN_UDP_Send(std::string rips,uint16_t rport,std::string message);
     int32_t FUN_UDP_Send(std::string rips,uint16_t rport,uint8_t *data, uint32_t offset, uint32_t len);


private:  
     //
     void UDP_StartReceive(void);

     boost::asio::io_service io_service;
     //智能指针管理 udp的socket
     std::unique_ptr<boost::asio::ip::udp::socket> socket_ptr;
     //智能指针管理 侦听线程对象
     std::unique_ptr<std::thread> Rece_thread_ptr;
     //接收标志
     uint8_t Recv_flag;
     //
     NetReceDataEvent UDPReceDataFun;
     NetStatusEvent UDPStatusFun;
};  

#endif
