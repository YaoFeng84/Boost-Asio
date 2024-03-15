#ifndef ClassTest_h
#define ClassTest_h

#include "FUN_Boost.h"

class TCPSProcess
{
public:
     //
     TCPSProcess();
     ~TCPSProcess();
     //
     int32_t Send(uint8_t*,uint32_t,uint32_t);
     void Disconnect(void);
     //
     void ReceDataCallback(SocketType st,std::string rips,uint16_t rport,uint8_t *data,uint16_t len);
     void StatusCallback(SocketType st,BoostStatus state,std::string rips,uint16_t rport);
     //
     std::unique_ptr<TCPSCommunicat> TCPSC = nullptr;

private:
     void AutoDisconnect_ThreadMethod(void);
     //
     std::unique_ptr<std::thread> AutoDisconnect_thread_ptr;
};



#endif
