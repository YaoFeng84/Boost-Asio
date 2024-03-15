#ifndef FUN_StringConv_h
#define FUN_StringConv_h 

#define SystemFlag_Windows    0
#define SystemFlag_Linux      1
#define SystemFlag            SystemFlag_Windows

#if SystemFlag == SystemFlag_Linux
#include <iconv.h>//Linux下的转换库，需要-liconv_hook 安装命令sudo apt-get install libiconv-hook-dev 安装完成后库文件在/usr/lib下
#elif SystemFlag == SystemFlag_Windows
#include <windows.h>  
#endif

#include <vector>  
#include <string>
#include <array>

// 
extern std::vector<uint8_t> FUN_StringConv_UTF8StrTo936ByteArray(const std::string &utf8Str);
extern std::string FUN_StringConv_936ByteArrayToUTF8Str(char *cp936_bytes, uint32_t size);

#endif
