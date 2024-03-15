#include <iostream>
#include "ClassTest.h"


TCPSProcess::TCPSProcess()
{

}

TCPSProcess::~TCPSProcess()
{

}

int32_t TCPSProcess::Send(uint8_t *dp,uint32_t offset,uint32_t dl)
{
     if(TCPSC != nullptr)
     {
          return TCPSC->FUN_TCPSCommunicat_Send(dp,offset,dl);
     }
     return 0;
}

void TCPSProcess::Disconnect(void)
{
     if(TCPSC != nullptr)
     {
          TCPSC->FUN_TCPSCommunicat_Disconnect();
     }
}

void TCPSProcess::ReceDataCallback(SocketType st,std::string rips,uint16_t rport,uint8_t *data,uint16_t len)
{
     std::cout<<"Rece Byte num = "<<len<<" from ["<<rips<<":"<<rport<<"]"<<std::endl;
     if(TCPSC != nullptr)
     {//以下是将数据回传
          TCPSC->FUN_TCPSCommunicat_Send(data,0,len);
     }
}

void TCPSProcess::StatusCallback(SocketType st,BoostStatus state,std::string rips,uint16_t rport)
{
     switch(state)
     {
          case BoostStatus::Connected:      
               std::cout<<"Remote Dev ["<<rips<<":"<<rport<<"] Connected!!!"<<std::endl;         
               AutoDisconnect_thread_ptr = std::make_unique<std::thread>(&TCPSProcess::AutoDisconnect_ThreadMethod,this);
               AutoDisconnect_thread_ptr->detach();
               break;
          case BoostStatus::Disconnected:
               std::cout<<"Remote Dev ["<<rips<<":"<<rport<<"] DisConnected!!!"<<std::endl;
               TCPSC = nullptr;
               break;
          default:
               break;
     }
}


void TCPSProcess::AutoDisconnect_ThreadMethod(void)
{
     std::this_thread::sleep_for(std::chrono::seconds(5));
     if(TCPSC != nullptr)
     {
          TCPSC->FUN_TCPSCommunicat_Disconnect();
     }
     TCPSC = nullptr;
}

