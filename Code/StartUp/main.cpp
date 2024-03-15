#include <iostream>
#include "FUN_Boost.h"
#include "FUN_StringConv.h"
#include "ClassTest.h"
#include <chrono>

//-----------------------以下是作用服务端的测试代码--------------------
#define UserNum     2//允许同时2个用户端接入
std::unique_ptr<Boost_TCPS> tcps;//实例化一个Boost_TCPS的智能指针
int32_t ListenIntex = -1;
TCPSProcess Tcpsp[UserNum];//用于存放TCPS已连接通道实例的数组
uint16_t ListenPort = 12348;//侦听端口号
int8_t CUn = 5;//累积接入5次后停止侦听

int32_t GetNextListenIndex(NetReceDataEvent *nrdf,NetStatusEvent *nsf)
{
     int32_t i;
     for(i = 0 ; i < UserNum ; i++)
     {
          if(Tcpsp[i].TCPSC == nullptr)
          {
               break;
          }
     }

     if(i < UserNum)
     {
          // 使用lambda表达式创建二个回调函数指针rdf 和 sf
          //使用值捕获的方式(这个值就是指向Tcpsp[i]的指针)，创建了一个lambda虚函数表达式
          TCPSProcess *tcpspp = &Tcpsp[i];
          auto rdf = [tcpspp](SocketType st,std::string rips, uint16_t rports, uint8_t *dp, uint16_t dl) 
          {  
               tcpspp->ReceDataCallback(st,rips,rports,dp,dl);
          };
          auto sf = [tcpspp](SocketType st,BoostStatus state,std::string rips,uint16_t rport) 
          {  
               tcpspp->StatusCallback(st,state,rips,rport);
          };

          *nrdf = rdf;
          *nsf = sf;
          return i;
     }
     return -1;
}

void tcps_StatusCallback(SocketType st,BoostStatus state,std::string rips,uint16_t rport)
{
     switch(state)
     {
          case Listening:
               std::cout<<"Local ["<<rips<<":"<<rport<<"] Start Listen...."<<std::endl;
               break;
          case ListenFail:
               std::cout<<"Local ["<<rips<<":"<<rport<<"] Listen Fail!!!"<<std::endl;
               break;
          case NoListen:
               std::cout<<"Stop Listen!!!"<<std::endl;
               break;
          case CheckListen:
               {
                    NetReceDataEvent rdf;
                    NetStatusEvent sf;
                    ListenIntex = GetNextListenIndex(&rdf,&sf);
                    if(ListenIntex >= 0)
                    {
                         tcps->FUN_TCPS_ContinueListen(rdf,sf);
                    }          
               }   
               break;
          default:
               std::cout<<"Unknow State"<<std::endl;
               break;
     }
}

void tcps_UserLinedCallback(std::unique_ptr<TCPSCommunicat> tcpsc,NetReceDataEvent *nrdf,NetStatusEvent *nsf)
{
     CUn--;
     //std::cout<<"tcps_UserLinedCallback"<<std::endl;
     if(ListenIntex >= 0)
     {
          Tcpsp[ListenIntex].TCPSC = std::move(tcpsc);//转存
     }    

     //查找是否有侦听的空间
     ListenIntex = GetNextListenIndex(nrdf,nsf);
     //
     //std::cout<<"ListenIntex = "<<ListenIntex<<" nrdf = "<<nrdf<<" nsf = "<<nsf<<std::endl;

     if(CUn == 0)
     {
          tcps->FUN_TCPS_StopListen();
          CUn--;
     }
}

int main(int argc, char *argv[]) 
{
     //定义一个TCPS
     tcps = std::make_unique<Boost_TCPS>(tcps_StatusCallback,tcps_UserLinedCallback);
     //
     NetReceDataEvent rdf;
     NetStatusEvent sf;
     ListenIntex = GetNextListenIndex(&rdf,&sf);
     if(ListenIntex >= 0)
     {//
          tcps->FUN_TCPS_StartListen(IPv4,ListenPort,rdf,sf);
     } 
    

    while(CUn >= 0);

    return 0;
}










//-----------------------以下是作用客户端的测试代码--------------------
// std::unique_ptr<Boost_TCPC> tcpc;//tcp客户端
// std::unique_ptr<std::thread> AutoDisconnect_thread_ptr;//自动断开线程
// std::unique_ptr<std::thread> AutoConnect_thread_ptr;//自动连接线程
// std::string Rips = "192.168.1.2";
// uint16_t Rport = 12346;

// void AutoDisconnect_ThreadMethod(void)
// {
//      std::this_thread::sleep_for(std::chrono::seconds(5));
//      if(tcpc != nullptr)
//      {
//           tcpc->FUN_TCPC_Disconnect();
//      }
//      //tcpc = nullptr;
// }

// void AutoConnect_ThreadMethod(void)
// {
//      std::this_thread::sleep_for(std::chrono::seconds(5));
//      if(tcpc != nullptr)
//      {
//           tcpc->FUN_TCPC_Connect(Rips,Rport);
//      }
//      //tcpc = nullptr;
// }

// void tcpc_ReceCallback(SocketType st,std::string rips, uint16_t rport, uint8_t *dp, uint16_t dl)
// {
//      std::cout<<"Rece Byte num = "<<dl<<" from ["<<rips<<":"<<rport<<"]"<<std::endl;
//      if(tcpc != nullptr)
//      {//以下是将数据回传
//           tcpc->FUN_TCPC_Send(dp,0,dl);
//      }
// }

// void tcpc_StatusCallback(SocketType st,BoostStatus state,std::string rips,uint16_t rport)
// {
//      switch(state)
//      {
//           case Connected:
//                std::cout<<"Remote ["<< rips <<":"<<rport<<"] Connected Success!!"<<std::endl;
//                AutoDisconnect_thread_ptr = std::make_unique<std::thread>(&AutoDisconnect_ThreadMethod);
//                AutoDisconnect_thread_ptr->detach();
//                break;
//           case Connecting:
//                std::cout<<"Remote ["<< rips <<":"<<rport<<"] Connecting..."<<std::endl;
//                break;
//           case ConnectFail:
//                std::cout<<"Remote ["<< rips <<":"<<rport<<"] Connect Fail!!!"<<std::endl;
//                AutoConnect_thread_ptr = std::make_unique<std::thread>(&AutoConnect_ThreadMethod);
//                AutoConnect_thread_ptr->detach();
//                break;
//           case Disconnected:
//                std::cout<<"Remote ["<< rips <<":"<<rport<<"] Disconnected!!"<<std::endl;
//                AutoConnect_thread_ptr = std::make_unique<std::thread>(&AutoConnect_ThreadMethod);
//                AutoConnect_thread_ptr->detach();
//                break;
//           case Disconnecting:
//                std::cout<<"Disconnecting..."<<std::endl;
//                break;
//           case DisconnectFail:
//                std::cout<<"Disconnect Fail"<<std::endl;
//                break;                
//           default:
//                std::cout<<"Unknow State"<<std::endl;
//                break;
//      }
// }

// int main(int argc, char *argv[]) 
// {
//      //定义一个TCPC
//      tcpc = std::make_unique<Boost_TCPC>(tcpc_ReceCallback,tcpc_StatusCallback);
//      // //连接服务器
//      // tcpc->FUN_TCPC_Connect(Rips,Rport);
//      std::cout<<"Start run..."<<std::endl;
//      AutoConnect_thread_ptr = std::make_unique<std::thread>(&AutoConnect_ThreadMethod);
//      AutoConnect_thread_ptr->detach();

//      while(1);

//      return 0;
// }


















//-----------------------以下是作为UDP的测试代码--------------------
// std::unique_ptr<Boost_UDP> udpt;//udp

// void udpt_ReceCallback(SocketType st,std::string rips, uint16_t rport, uint8_t *dp, uint16_t dl)
// {
//      std::cout<<"Rece Byte num = "<<dl<<" from ["<<rips<<":"<<rport<<"]"<<std::endl;
//      if(udpt != nullptr)
//      {//以下是将数据回传
//           // udpt->FUN_UDP_Send(rips,rport,dp,0,dl);
//      }
// }

// void udpt_StatusCallback(SocketType st,BoostStatus state,std::string rips,uint16_t rport)
// {
//      switch(state)
//      {
//           case StopReceData:
//                std::cout<<"StopReceData"<<std::endl;
//                break;                
//           default:
//                std::cout<<"Unknow State"<<std::endl;
//                break;
//      }
// }

// int main(int argc, char *argv[]) 
// {
//      udpt = std::make_unique<Boost_UDP>(IPv4,12347,udpt_ReceCallback,udpt_StatusCallback);
//      //
//      while(1)
//      {
//           // // 本地广播
//           // udpt->FUN_UDP_Send("255.255.255.255",12347,(uint8_t *)"hello_255.255.255.255",0,21);
//           // std::this_thread::sleep_for(std::chrono::seconds(5));
//           // //网段广播
//           // udpt->FUN_UDP_Send("192.168.10.255",12347,(uint8_t *)"hello_192.168.10.255",0,19);
//           // std::this_thread::sleep_for(std::chrono::seconds(5));
//           // //单播
//           // udpt->FUN_UDP_Send("192.168.10.78",12347,(uint8_t *)"hello_192.168.10.78",0,19);
//           // std::this_thread::sleep_for(std::chrono::seconds(5));
//      }
//      //
//      return 0;
// }











