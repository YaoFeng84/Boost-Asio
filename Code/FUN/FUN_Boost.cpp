#include "FUN_Boost.h"

#define ReceBufferSize   (50 * 1024)    //接收缓存字节数
/*
当设备存在2张 或 以上网卡时，且都有有效的IP地址，或处于不同网段时
由于Boost_TCPS 和 Boost_UDP不存在设置侦听本机IP，而是侦听全部有效IP的网卡的对应端口，
故在此情况下，不管哪张网卡，对应的侦听端口有数据送来时，都会触发收到数据事件。
*/


////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief TCPS已连接socket实例的构造函数
 * 
 * @param sock TCPS已连接的socket实例的智能指针
 * @param st socket类型
 * @param rdFun TCPS已连接socket实例收到数据事件的回调函数
 * @param staFun TCPS已连接socket实例状态事件的回调函数
 */
TCPSCommunicat::TCPSCommunicat(std::unique_ptr<boost::asio::ip::tcp::socket> sock,SocketType st,NetReceDataEvent rdFun,NetStatusEvent staFun):
     ConnectState(Connected),//连接成功
     tcpsock_ptr(std::move(sock)),                  //套接字指针
     ReceDataFun(rdFun),     //收到数据回调函数
     StatusFun(staFun),  //状态信息回调函数
     SocketType_(st)
{
     // //创建接收线程t1，并运行
     Rece_thread_ptr = std::make_unique<std::thread>(&TCPSCommunicat::TCPSCommunicat_ReceThreadMethod, this);
     Rece_thread_ptr->detach();//将线程与主线程分离，主线程结束，线程随之结束。
}

/**
 * @brief TCPS已连接socket实例的析构函数
 * 
 */
TCPSCommunicat::~TCPSCommunicat()
{
     //std::cout<<"~TCPCommunicat()"<<std::endl;
}

/**
 * @brief TCPS已连接socket实例断开连接的函数
 * 
 * @return int8_t 小于0表示断开失败 0表示断开成功
 */
int8_t TCPSCommunicat::FUN_TCPSCommunicat_Disconnect(void)
{    
     // TCPC_Socket.close();//无法产生FIN挥手，而是直接复位
     if(tcpsock_ptr != nullptr)
     {
          // try
          // {
          //      tcpsock_ptr->cancel();//用于取消所有挂起的异步操作。如果 read_some 或其他异步操作正在阻塞等待数据，调用 cancel() 会导致这些操作立即返回，并带有特定的错误代码（通常是 boost::asio::error::operation_aborted）。
          // }
          // catch(...){}  

          try
          {//执行下面语句后，接收线程中阻塞的tcpsock_ptr->read_some函数会退
           //并跳到(ignored_error == boost::asio::error::eof)执行
               tcpsock_ptr->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
          }
          catch(...){} 
     }
     //此处延时是为了接收线程退出前产生的事件中IP地址要正确(再次接入异常好像也是此引起的)
     //因为，外部如果调用完FUN_TCPSCommunicat_Disconnect之后，马上将对像释放，可能会引起接收线程的异常的问题
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
     return 0;
}

/**
 * @brief 阻塞型发送数据函数
 * 
 * @param data 待发送的首地址
 * @param offset 数据的偏移字节量
 * @param len 待发送数据的字节数
 * @return int32_t 实际发送的字节量
 */
int32_t TCPSCommunicat::FUN_TCPSCommunicat_Send(uint8_t *data, uint32_t offset, uint32_t len)
{
     if(ConnectState == Connected)
     {
          try
          {
               //boost::asio::write(TCPC_Socket, boost::asio::buffer(data + offset,len), ignored_error);
               return tcpsock_ptr->send(boost::asio::buffer(data + offset,len));
          }
          catch (std::exception& e)
          {
               //std::cerr << e.what() << std::endl;  
               //return 0;
          }
     }
     return 0;
}

/**
 * @brief 内部 TCPS已连接socket实例 的 接收线程方法
 * 
 */
void TCPSCommunicat::TCPSCommunicat_ReceThreadMethod(void)
{
     size_t length;
     boost::system::error_code ignored_error;  
     uint8_t sbuffer[ReceBufferSize];

     //std::this_thread::sleep_for(std::chrono::milliseconds(1));//本线程休眠stime秒

     if(tcpsock_ptr == nullptr)
     {          
          return;
     }

     // 获取对端的 endpoint  
     boost::asio::ip::tcp::endpoint rendpoint = tcpsock_ptr->remote_endpoint();    
     RemoteIP = rendpoint.address().to_string();  
     RemotePort = rendpoint.port();  
     
     // 获取本地的 endpoint
     // boost::asio::ip::tcp::endpoint lendpoint = tcpsock_ptr->local_endpoint();
     // std::string lip = lendpoint.address().to_string();  
     // unsigned short lport = lendpoint.port();

     //
     StatusFun(B_TCPS,Connected,RemoteIP,RemotePort);

     while(ConnectState == Connected)
     {
          //std::cout<<"Start Rece!!!"<<std::endl;
          //如果没有 ignored_error 参数，当对端断开时，将无法执行到length=0 --> TCPC_CloseCallBack
          length = tcpsock_ptr->read_some(boost::asio::buffer(sbuffer,(size_t)sizeof(sbuffer)), ignored_error);  
          if(!ignored_error)
          {//成功读取到数据
               ReceDataFun(SocketType_,RemoteIP,RemotePort,sbuffer, length);
               //tcpsock_ptr->send(boost::asio::buffer(sbuffer + 0,length));
          }
          else if(ignored_error == boost::asio::error::eof)
          {//对端关闭了
               // std::cout<<"disconnect_1"<<std::endl;
               break;//退出循环
          }
          else
          {//其他错误
               //std::cout<<"TCPC_Socket = "<<ignored_error.message()<<std::endl;//显示错误信息
               // std::cout<<"disconnect_2"<<std::endl;
               break;//退出循环
          }
     }
     


     ConnectState = Disconnected;
     try
     {
          if(tcpsock_ptr != nullptr)
          {
               // std::cout<<"disconnect_0"<<std::endl;
               // TCPC_Socket.close();//无法产生FIN挥手，而是直接复位
               tcpsock_ptr->shutdown(boost::asio::ip::tcp::socket::shutdown_both); 
          }                   
     }catch(...){}
     
     // std::cout<<"!!!Stop Rece"<<std::endl;

     StatusFun(B_TCPS,Disconnected,RemoteIP,RemotePort);

     // std::cout<<"!!!Stop Rece!!!"<<std::endl;
}









////////////////////////////////////////////////////////////////////////////////////
/*
Boost_TCPS类只提供 启动侦听服务、继续侦听服务、停止侦听服务 及配合事件实现的暂停侦听服务。

一但有客户端接入，则会通过TCPSCommunicat类智能指针，将接入的客户端通过【用户接入事件回调】转移出去，由用户代码自行保管
用户可以通过TCPSCommunicat类的智能指针，对已接入的客户端进行 发送数据、断开连接等操作，而收到的数据，会产生相关的回调函数产生

关于TCPSCommunicat回调函数的实现机制：
1、当Boost_TCPS实例启动侦听时，会要求用户代码提供一个 NetReceDataEvent的收到数据事件回调函数DF1 和 NetStatusEvent的状态事件回调函数SF1
   而用户提供的这两个回调函数DF1 和 SF1，将会提供给接下来接入的客户端来使用。
   故，当启动侦听后，首个客户端接进来时，产生UserLinkedEvent事件，会有两件事：
   1、提供TCPSCommunicat类已连接的智能指针，此智能指针是给用户代码操作的，此智能指针提供 发送数据、断开连接，而收到的数据，则会回调DF1函数，而产生的状态事件则会回调SF1函数
   2、要求用户提供接下来可用于再次接入的客户端所要使用的 NetReceDataEvent回调函数 和 NetStatusEvent回调函数
----->当希望继续侦听时，则提供，如DF2 和 SF2，Boost_TCPS实例会继续侦听，并等待新用户的接入
----->当希望暂停侦听时，则不修改UserLinkedEvent事件的返回的回调数据，Boost_TCPS实例会停止侦听，
             并且，每隔1秒，Boost_TCPS实例的 状态事件回调函数 会产生一个名为CheckListen的判断侦听事件
             用户可以通过该事件，在需要继续侦听时，调用FUN_TCPS_ContinueListen，让Boost_TCPS实例继续侦听
             当然，FUN_TCPS_ContinueListen这个函数的前提是提供继续侦听为新接入客户端提供所需要的 NetReceDataEvent回调函数 和 NetStatusEvent回调函数
      通过上面的操作实际上就可以控制同时接入的客户端的数量了！！！！

2、当要结束Boost_TCPS实例侦听时，可以调用FUN_TCPS_StopListen来解束侦听
   调用FUN_TCPS_StopListen结束侦听后，无法通过调用FUN_TCPS_ContinueListen来恢复
   当需要重新侦听时，需要调用FUN_TCPS_StartListen来重新启动一次侦听。

*/

/**
 * @brief TCPS实例构造函数
 * 
 * @param scfun TCPS状态事件回调函数
 * @param ulefun 用户接入事件回调函数
 */
Boost_TCPS::Boost_TCPS(NetStatusEvent scfun,UserLinkedEvent ulefun):
     Listen_thread_ptr(nullptr),
     Acceptor_ptr(nullptr),
     UserLinkedFun(ulefun),
     StatusFun(scfun),
     Statusflag(NoListen)
{
     
}

/**
 * @brief TCPS实例析构函数
 * 
 */
Boost_TCPS::~Boost_TCPS()
{
     
}

/**
 * @brief TCPS启动侦听函数
 * 
 * @param ipv 使用IP地址版本
 * @param ports 侦听的端口号
 * @param nrdf socket收到数据事件回调函数
 * @param nsf socket状态事件回调函数
 * @return int8_t 0:启动侦听成功 非0:启动侦听失败
 */
int8_t Boost_TCPS::FUN_TCPS_StartListen(IPVersion ipv, std::string ports, NetReceDataEvent nrdf, NetStatusEvent nsf)
{     
     Statusflag = Listening;
     //
     Listen_thread_ptr = std::make_unique<std::thread>(&Boost_TCPS::TCPS_ListenThreadMethod,this,ipv,std::stoi(ports),nrdf,nsf);
     Listen_thread_ptr->detach();
     
     return 0;
}

/**
 * @brief TCPS启动侦听函数
 * 
 * @param ipv 使用IP地址版本
 * @param ports 侦听的端口号
 * @param nrdf socket收到数据事件回调函数
 * @param nsf socket状态事件回调函数
 * @return int8_t int8_t 0:启动侦听成功 非0:启动侦听失败
 */
int8_t Boost_TCPS::FUN_TCPS_StartListen(IPVersion ipv,uint16_t lport, NetReceDataEvent nrdf, NetStatusEvent nsf)
{
     Statusflag = Listening;
     //
     Listen_thread_ptr = std::make_unique<std::thread>(&Boost_TCPS::TCPS_ListenThreadMethod,this,ipv,lport,nrdf,nsf);
     Listen_thread_ptr->detach();
     return 0;
}

/**
 * @brief TCPS继续侦听函数
 * 当Boost_TCPS实例暂停侦听时，实例的状态回调函数会每秒产生一次CheckListen事件的回调
 * 如果想恢复继续侦听，则需要调用此函数
 * @param nrdfun 收到数据事件回调函数
 * @param nsfun 状态事件回调函数
 */
void Boost_TCPS::FUN_TCPS_ContinueListen(NetReceDataEvent nrdfun, NetStatusEvent nsfun)
{
     if(Statusflag == CheckListen)
     {
          NrdFun = nrdfun;
          NsFun = nsfun;
          Statusflag = Listening;
     }
}

/**
 * @brief TCPS停止侦听函数
 * 
 */
void Boost_TCPS::FUN_TCPS_StopListen(void)
{
     Statusflag = NoListen;     
     if(Acceptor_ptr != nullptr)
     {
          try
          {
               Acceptor_ptr->close();
          }
          catch (...){}
          Acceptor_ptr.reset();
          Acceptor_ptr = nullptr;
     }
     Listen_thread_ptr = nullptr;
}

/**
 * @brief 内部TCPS侦听线程
 * 
 * @param ipv IP地址版本
 * @param lport 侦听端口
 * @param nrdfun Socket收到数据回调函数
 * @param nsfun 连接状态回调函数
 */
void Boost_TCPS::TCPS_ListenThreadMethod(IPVersion ipv, uint16_t lport, NetReceDataEvent nrdfun, NetStatusEvent nsfun)
{
     int8_t acceptokflag = 0;   
     int8_t listenbackcallflag = 1;//侦听回调标志

     // std::cout <<"start listen 0!!!"<<std::endl;
     NrdFun = nrdfun;
     NsFun = nsfun;

     while((Statusflag == Listening) || (Statusflag == CheckListen))
     {
          if(Statusflag == Listening)
          {
               if(Acceptor_ptr == nullptr)
               {
                    try
                    {
                         if(ipv == IPv6)
                         {
                              Acceptor_ptr = std::make_unique<boost::asio::ip::tcp::acceptor>(tcp_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v6::any(), lport));
                         }
                         else
                         {
                              Acceptor_ptr = std::make_unique<boost::asio::ip::tcp::acceptor>(tcp_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::any(), lport));
                         }                         
                    }
                    catch(...)
                    {//Bind异常---->IP地址错误，非本机IP，端口被占用等
                         Acceptor_ptr = nullptr;                         
                         Statusflag = ListenFail;//启动侦听失败--退出侦听
                    }                    
               }

               if(listenbackcallflag)
               {//一次侦听状态回调
                    listenbackcallflag = 0;
                    if(ipv == IPv6)
                    {
                         StatusFun(B_TCPS,Statusflag,boost::asio::ip::address_v6::any().to_string(),lport);//执行回调函数
                    }
                    else
                    {
                         StatusFun(B_TCPS,Statusflag,boost::asio::ip::address_v4::any().to_string(),lport);//执行回调函数
                    }                    
               }
                    

               if(Acceptor_ptr != nullptr)
               {
                    //实例化一个新的socket对象的智能指针
                    std::unique_ptr<boost::asio::ip::tcp::socket> socket_ptr(new boost::asio::ip::tcp::socket(tcp_context));//创建socket
                    acceptokflag = 0;
                    try
                    {//此处异常主要是为了应付在等待用户接入时，调用停止侦听时，会抛出异常
                         Acceptor_ptr->accept(*socket_ptr);//等待客户端接入
                         acceptokflag = 1;    
                    }
                    catch(...){}
                    

                    if(acceptokflag)
                    {//正常允许用户接入
                         //实例化一个新的TCPSCommunicat类的对象
                         std::unique_ptr<TCPSCommunicat> communicat_ptr = std::make_unique<TCPSCommunicat>(std::move(socket_ptr),B_TCPS,NrdFun,NsFun);
                         NrdFun = 0;
                         NsFun = 0;                         
                         //触发用户接入事件                         
                         UserLinkedFun(std::move(communicat_ptr),&NrdFun,&NsFun);
                         //
                         if((NrdFun) && (NsFun))
                         {//允许继续侦听

                         }
                         else
                         {//允许判断侦听
                              if(Acceptor_ptr != nullptr)
                              {
                                   Acceptor_ptr->close();//关闭，防止用户接入
                                   Acceptor_ptr.reset();//释放智能指针
                                   Acceptor_ptr = nullptr;//置空
                              } 
                              Statusflag = CheckListen;
                         }
                    }
                    else
                    {
                         Statusflag = ListenFail;
                    }
               }
          }
          else if(Statusflag == CheckListen)      
          {
               std::this_thread::sleep_for(std::chrono::seconds(1));
               StatusFun(B_TCPS,Statusflag,"",0);//执行回调函数               
          }
     }

     // std::cout<<"Stop Listen Thread 0"<<std::endl;
     if(Acceptor_ptr != nullptr)
     {
          Acceptor_ptr->close();//关闭，防止用户接入
          Acceptor_ptr.reset();//释放智能指针
          Acceptor_ptr = nullptr;//置空
     }     
     StatusFun(B_TCPS,Statusflag,"",0);//执行回调函数
     // std::cout<<"Stop Listen Thread 1"<<std::endl;
     //std::cout <<"stop listen!!!"<<std::endl;
}







////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief TCPC实例构造函数
 * 
 * @param rdcfun TCPC收到数据时的回调函数
 * @param scfun TCPC状态回调函数
 */
Boost_TCPC::Boost_TCPC(NetReceDataEvent rdcfun, NetStatusEvent scfun):
     TcpcSocket_Ptr(nullptr),
     Rece_thread_ptr(nullptr),
     ReceDataFun(rdcfun),
     StatusFun(scfun),
     ConnectState(Disconnected)
{
     
}

/**
 * @brief TCPC实例析构函数
 * 
 */
Boost_TCPC::~Boost_TCPC()
{
     //std::cout<<"~TCPC()"<<std::endl;
}

/**
 * @brief TCPC阻塞型连接函数
 * 
 * @param rips 对端的IPv4、IPv6、主机名、域名
 * @param rport 对端的端口号
 * @return int8_t 0:表示连接成功 小于0:表示连接失败
 */
int8_t Boost_TCPC::FUN_TCPC_Connect(std::string rips, uint16_t rport)
{
/*
此处使用解析器原因：
在Boost.Asio中，使用boost::asio::ip::tcp::endpoint来直接创建一个网络端点确实是一种可能的方式，
但这通常在你已经确切知道目标主机的IP地址和端口号时才会这么做。
然而，在大多数实际的应用场景中，你通常只知道服务端的域名（比如www.example.com）或主机名（比如myserver），而不是它的具体IP地址。
此外，端口号也可能是以服务名（比如http或https）的形式给出，而不是具体的数字。

因此，boost::asio::ip::tcp::resolver的作用就是将这些更高级别的标识符（如域名或服务名）转换为实际的IP地址和端口号，
从而生成可以用来建立连接的endpoint对象。
resolver通过查询DNS（域名系统）服务或相应的服务名到端口号的映射来完成这个转换。
*/
     try
     { //resolver.resolve({(char *)ips, (char *)ports});
          //创建解析序列
          boost::asio::ip::tcp::resolver::query query(rips, std::to_string(rport));          
          //创建错误代码
          boost::system::error_code ec;
          //创建解析器
          boost::asio::ip::tcp::resolver resolver(tcp_context);  
          //解析出目标设备的IP地址和端口号                 
          auto destination_endpoint = resolver.resolve(query);
          //执行回调事件
          ConnectState = Connecting;
          StatusFun(B_TCPC,Connecting,rips,rport);//执行回调
          //创建一个新的socket的智能指针 给 TcpcSocket_Ptr，原来TcpcSocket_Ptr上的自动释放
          TcpcSocket_Ptr = std::make_unique<boost::asio::ip::tcp::socket>(tcp_context);
          //连接到第一个解析得到的端点          
          //boost::asio::connect(*TcpcSocket_Ptr, destination_endpoint, ec);
          TcpcSocket_Ptr->connect(*destination_endpoint,ec);
          // 检查是否连接成功  
          if(ec){}//连接失败
          else
          {//已连接上
               // //创建接收线程t1，并运行
               Rece_thread_ptr = std::make_unique<std::thread>(&Boost_TCPC::TCPC_ReceThreadMethod, this);
               Rece_thread_ptr->detach();//将线程与主线程分离，主线程结束，线程随之结束。
               ConnectState = Connected;     
               return 0;               
          }
     }
     catch(...){}//可能IP地址 或 端口 参数非法导致的异常    
     ConnectState = ConnectFail;
     StatusFun(B_TCPC,ConnectFail,rips,rport);
     return -1;
}

/**
 * @brief TCPC阻塞型发送数据函数接口
 * 
 * @param data 待发字节数组的首地址
 * @param offset 距首地址的偏移字节量
 * @param len 待发的字节数
 * @return int32_t 实际发送的字节数
 */
int32_t Boost_TCPC::FUN_TCPC_Send(uint8_t *data, uint32_t offset, uint32_t len)
{
     if(ConnectState == Connected)
     {
          if(TcpcSocket_Ptr != nullptr)
          {
               try
               {
                    //boost::asio::write(TCPC_Socket, boost::asio::buffer(data + offset,len), ignored_error);
                    return TcpcSocket_Ptr->send(boost::asio::buffer(data + offset,len));
               }
               catch (...){}
          }          
     }
     return 0;
}

/**
 * @brief TCPC断开连接的函数
 * 
 */
void Boost_TCPC::FUN_TCPC_Disconnect(void)
{
     if(ConnectState == Connected)
     {//已连接
          if(TcpcSocket_Ptr != nullptr)
          {
               // try
               // {
               //      TcpcSocket_Ptr->cancel();//用于取消所有挂起的异步操作。如果 read_some 或其他异步操作正在阻塞等待数据，调用 cancel() 会导致这些操作立即返回，并带有特定的错误代码（通常是 boost::asio::error::operation_aborted）。
               // }
               // catch(...){}
               
               try
               {
                    TcpcSocket_Ptr->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
               }
               catch(...){} 
          }
          // ConnectState = Disconnected;
     }
     else if(ConnectState == Connecting)
     {//未连接---如果正在连接，则停止正在连接的动作
          ConnectState = Disconnecting;

     }
}

/**
 * @brief TCPC 内部接收线程方法
 * 
 */
void Boost_TCPC::TCPC_ReceThreadMethod(void)
{
     
     size_t length;
     boost::system::error_code ignored_error;  
     uint8_t sbuffer[ReceBufferSize];

     //std::this_thread::sleep_for(std::chrono::milliseconds(1));//本线程休眠stime秒

     if(TcpcSocket_Ptr == nullptr)
     {          
          return;
     }
     // std::cout<<"Tcpc Start Rece 0"<<std::endl;
     // 获取对端的 endpoint  
     boost::asio::ip::tcp::endpoint rendpoint = TcpcSocket_Ptr->remote_endpoint();      
     std::string RemoteIP = rendpoint.address().to_string();//远程IP
     uint16_t RemotePort = rendpoint.port();//远程端口
     // std::cout<<"Tcpc Start Rece 1"<<std::endl;
     
     // 获取本地的 endpoint
     // boost::asio::ip::tcp::endpoint lendpoint = tcpsock_ptr->local_endpoint();
     // std::string lip = lendpoint.address().to_string();  
     // unsigned short lport = lendpoint.port();

     //
     ConnectState = Connected;
     StatusFun(B_TCPC,Connected,RemoteIP,RemotePort);
     // std::cout<<"Tcpc Start Rece 2"<<std::endl;
     while(ConnectState == Connected)
     {
          //std::cout<<"Start Rece!!!"<<std::endl;
          //如果没有 ignored_error 参数，当对端断开时，将无法执行到length=0 --> TCPC_CloseCallBack
          length = TcpcSocket_Ptr->read_some(boost::asio::buffer(sbuffer,(size_t)sizeof(sbuffer)), ignored_error);  
          if(!ignored_error)
          {//成功读取到数据
               ReceDataFun(B_TCPC,RemoteIP,RemotePort,sbuffer, length);
               //tcpsock_ptr->send(boost::asio::buffer(sbuffer + 0,length));
          }
          else if((ignored_error == boost::asio::error::eof))// && (ConnectState == Connected))
          {//对端关闭了(增加TCPC_ConnectState判断，主要是为了防止在Linux下，已方主动关闭，也会跳到这里，产生回调)
               // StatusCallBack(Disconnected,rip,rport);
               // std::cout<<"Disconnect_1"<<std::endl;
               break;//退出循环
          }
          else
          {//其他错误
               //std::cout<<"TCPC_Socket = "<<ignored_error.message()<<std::endl;//显示错误信息
               break;//退出循环
          }
     }
     


     ConnectState = Disconnected;
     try
     {
          if(TcpcSocket_Ptr != nullptr)
          {
               // TCPC_Socket.close();//无法产生FIN挥手，而是直接复位
               TcpcSocket_Ptr->shutdown(boost::asio::ip::tcp::socket::shutdown_both); 
          }                   
     }catch(...){}

     StatusFun(B_TCPC,Disconnected,RemoteIP,RemotePort);
}








////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief UDP实例构造函数
 * 支持在多个网卡都有有效IP地址的情况下，哪个网卡从lport收到 广播 或 单播数据时，都会被此代码接收到
 * 
 * @param ipv 侦听的IP版本号,支持IPv4 和 IPv6
 * @param lport 侦听的端口号
 * @param rdfun 收到数据的回调函数
 * @param sfun 实例状态的回调函数
 */
Boost_UDP::Boost_UDP(IPVersion ipv, uint16_t lport, NetReceDataEvent rdfun, NetStatusEvent sfun):
     socket_ptr(nullptr),
     Rece_thread_ptr(nullptr),
     Recv_flag(0),
     UDPReceDataFun(rdfun),
     UDPStatusFun(sfun)
     // socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{  
     //申请一个udp的socket智能指针，初始化并绑定到指定的IP和端口上
     //Socket使用的IP地址 和 端口号---->发送数据时，会使用该IP和端口号来发送
     //这样，对方UDP收到的本机IP和本机端口号就是此参数
     //对方UDP回复数据时，也是向本机IP和本机端口号回复数据。
     //故本机也只有侦听或从该端口号接收数据
     //lips地址为0.0.0.0时，即可以接收对端的广播，也可以接收对端的单播，也可以接收自身的广播
     //lips地址为自己设备的IP值时，在Linux下好像只可以对端的单播，而对端的广播 和 自身的广播，无法收到
     //socket_ptr = std::make_unique<boost::asio::ip::udp::socket>(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(lips), lport));
     //故此处统一使用IPv4的自身地址，如果想改为IPv6的，
     // socket_ptr = std::make_unique<boost::asio::ip::udp::socket>(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"), lport));
     if(ipv == IPv6)
     {
          socket_ptr = std::make_unique<boost::asio::ip::udp::socket>(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::any(), lport));
     }
     else
     {
          socket_ptr = std::make_unique<boost::asio::ip::udp::socket>(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), lport));
     }
     

     

     if(socket_ptr != nullptr)
     {
          /*具体来说，
          boost::asio::ip::udp::socket::reuse_address选项的作用是控制当socket被绑定到一个本地地址和端口时，
          是否允许该地址和端口在socket关闭后立即被其他socket重新使用。
          默认情况下，当UDP socket关闭后，
          操作系统会保留该socket绑定的地址和端口一段时间（这通常被称为TIME_WAIT状态），
          以防止由于网络中的延迟数据包导致的混乱。然而，在某些情况下，
          你可能希望立即重用该地址和端口，以避免资源浪费或简化程序逻辑。*/
          socket_ptr->set_option(boost::asio::ip::udp::socket::reuse_address(true)); //设置socket选项，允许该socket的地址在关闭后立即被重新使用
          //设置socket选项，使能收发广播
          socket_ptr->set_option(boost::asio::ip::udp::socket::broadcast(true));//使能收发广播
                  
          
          //绑定到指定IP和端口（因为创建时，就在初始化时已绑定，故此处不用再次绑定）
          // boost::asio::ip::udp::endpoint udpendpoint(boost::asio::ip::address::from_string(lips), lport);
          // socket_ptr->bind(udpendpoint);

          //如果配置udp的socket时，socket_ptr = std::make_unique<boost::asio::ip::udp::socket>(io_service, boost::asio::ip::udp::v4());
          //上面配置未指定IP及端口号，此时，需要再次绑定IP 和 端口号
          //如果绑定采用以下方式时，会收到本设备发出的广播，但却无法收到其他设备发来的广播 或 单播数据
          // boost::asio::ip::udp::endpoint udpendpoint(boost::asio::ip::udp::v4(),lport);
          // socket_ptr->bind(udpendpoint);


          // 开始接收数据  
          Rece_thread_ptr = std::make_unique<std::thread>(&Boost_UDP::UDP_StartReceive,this);
          Rece_thread_ptr->detach();
     }     
} 

/**
 * @brief UDP实例的析构函数
 * 
 */
Boost_UDP::~Boost_UDP()
{
     Recv_flag = 0;
     if(socket_ptr != nullptr)
     {
          try
          {
               socket_ptr->cancel();//用于取消所有挂起的异步操作。如果 receive_from 或其他异步操作正在阻塞等待数据，调用 cancel() 会导致这些操作立即返回，并带有特定的错误代码（通常是 boost::asio::error::operation_aborted）。
          }
          catch(...){}
     }     
     socket_ptr = nullptr;
     Rece_thread_ptr = nullptr;
}

/**
 * @brief UDP发送函数
 * 
 * @param rips 对端IP地址，无缝支持IPv4 或 IPv6
 * @param rport 对端端口号
 * @param message 所要发送的信息字符串
 * @return int32_t 返回实际的发送字节数
 */
int32_t Boost_UDP::FUN_UDP_Send(std::string rips,uint16_t rport,std::string message)
{
     //此处判断 socket_ptr是否合法
     //本机使用的发送端点的信息，则是由实例化时，指定的
     std::size_t sn = 0;
     if(socket_ptr != nullptr)
     {
          boost::system::error_code ignored_error;  
          //          
          try
          {
               // uint16_t rport = socket_ptr->local_endpoint().port();//对端端口号
               //当 rips = "255.255.255.255" 或 某网段广播IP时，是广播
               //当 rips = 某个指定的IP时，如"192.168.1.6"时，是单播
               //定义一个目标端点
               boost::asio::ip::udp::endpoint destination_endpoint(boost::asio::ip::address::from_string(rips), rport);
               sn = socket_ptr->send_to(boost::asio::buffer(message), destination_endpoint, 0, ignored_error);
          }
          catch(...){sn = 0;}
          
          if (ignored_error) 
          {  //发送失败
               // std::cerr << "Send failed: " << ignored_error.message() << std::endl; 
          } 
          else 
          {  //发送成功
               // std::cout << "Sent " << message << " to " << destination_endpoint << std::endl;  
               // return sn;
          }  
     }
     return sn;
}

/**
 * @brief UDP发送函数
 * 
 * @param rips 对端IP地址，无缝支持IPv4 或 IPv6
 * @param rport 对端端口号
 * @param data 所要发送数据首地址
 * @param offset 距离首地址的偏移字节数
 * @param len 所要发送的字节数
 * @return int32_t 实际发送的字节数
 */
int32_t Boost_UDP::FUN_UDP_Send(std::string rips,uint16_t rport,uint8_t *data, uint32_t offset, uint32_t len)
{
     std::size_t sn = 0;
     if(socket_ptr != nullptr)
     {
          boost::system::error_code ignored_error; 

          try
          {
               boost::asio::ip::udp::endpoint destination_endpoint(boost::asio::ip::address::from_string(rips), rport);
               //boost::asio::ip::udp::endpoint destination_endpoint(boost::asio::ip::address_v4::broadcast(), rport);//广播地址
               sn = socket_ptr->send_to(boost::asio::buffer(data + offset,len), destination_endpoint, 0, ignored_error);
          }
          catch (...){sn = 0;}

          if (ignored_error) 
          {  //发送失败
               // std::cerr << "Send failed: " << ignored_error.message() << std::endl; 
          } 
          else 
          {  //发送成功
               // std::cout << "Sent " << message << " to " << destination_endpoint << std::endl;  
               // return sn;
          } 
     }
     return sn;
}

/**
 * @brief 内部UDP启动接收的方法
 * 
 */
void Boost_UDP::UDP_StartReceive(void) 
{  
     //接收缓存
     std::array<char, ReceBufferSize> recv_buffer;  
     //申明一个对端网络端点，用于接收对端端点的信息
     boost::asio::ip::udp::endpoint remote_endpoint;  
     //
     size_t bytes_received = 0; // 用来存储实际接收到的字节数
     boost::system::error_code error;
     //
     Recv_flag = 1;

     // 查看本地侦听的端口号
     //    unsigned short port = socket_ptr->local_endpoint().port();  
     //    std::cout << "Listening on port " << port << std::endl;  
     
     while(Recv_flag && (socket_ptr != nullptr))
     {
          bytes_received = socket_ptr->receive_from(boost::asio::buffer(recv_buffer), remote_endpoint, 0, error);  

          // 检查是否有错误发生  
          if (error) 
          {  
               // 处理错误  
               Recv_flag = 0;//退出接收
               // std::cerr << "Receive failed: " << error.message() << std::endl;  
          } 
          else 
          {  
               // 处理接收到的数据  
               UDPReceDataFun(B_UDP,remote_endpoint.address().to_string(),remote_endpoint.port(),(uint8_t *)recv_buffer.data(),bytes_received);
               // std::cout << "Received Data :" << bytes_received << " bytes from " << remote_endpoint << std::endl;  
               // 可以在这里使用 recv_buffer 的前 bytes_received 个字节  
          }
     }

     UDPStatusFun(B_UDP,StopReceData,"",0);

}

