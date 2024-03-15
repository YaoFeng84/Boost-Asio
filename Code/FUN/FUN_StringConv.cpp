#include "FUN_StringConv.h"

// #if SystemFlag == SystemFlag_Linux
// #include <iconv.h> //Linux下的转换库，需要sudo apt-get install libiconv-hook-dev
// #endif

#if SystemFlag == SystemFlag_Linux
// 函数：将UTF-8字符串转换为CP-936编码的字节数组  
std::vector<uint8_t> FUN_StringConv_UTF8StrTo936ByteArray(const std::string &utf8Str) 
{  
    std::vector<uint8_t> cp936Bytes;    

    // 设置iconv转换描述符  
    iconv_t cd = iconv_open("CP936", "UTF-8");  
    if (cd == (iconv_t)-1) 
    {  
        // perror("iconv_open");  
        return {};  
    }   

    // 计算输入字符串的长度（字节）  
    size_t utf8Len = utf8Str.length();  
    size_t cp936Len = utf8Len * 4; // 假设最大膨胀为4倍（通常足够）  
    cp936Bytes.resize(cp936Len);    

    // 准备输入和输出缓冲区  
    char* inbuf = const_cast<char*>(utf8Str.c_str());  
    char* outbuf = reinterpret_cast<char*>(cp936Bytes.data());    

    // 执行转换  
    if (iconv(cd, &inbuf, &utf8Len, &outbuf, &cp936Len) == (size_t)-1) 
    {  
        // perror("iconv");  
        iconv_close(cd);  
        return {};  
    }    

    // 关闭转换描述符  
    iconv_close(cd);  

     //调整输出字节数组的大小以去除未使用的空间
     int i,osize = cp936Bytes.size();
     for(i = 0; i < osize; i++)
     {
         if(cp936Bytes[i] == 0)
         {
             cp936Bytes.resize(i);
             break;
         }
     }

    return cp936Bytes;  
}  
#elif SystemFlag == SystemFlag_Windows
std::vector<uint8_t> FUN_StringConv_UTF8StrTo936ByteArray(const std::string &utf8Str)
{
     // 首先，将UTF-8字符串转换为宽字符串（UTF-16）  
    int wcharCount = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);  
    std::vector<wchar_t> wcharBuffer(wcharCount);  
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wcharBuffer[0], wcharCount);  
  
    // 然后，将宽字符串转换为CP-936编码的字节数组
    //请注意，CP_ACP通常指的是当前系统区域设置的ANSI代码页，对于中文Windows系统，这通常是CP_936。
    //如果你的系统区域设置不是中文，那么CP_ACP可能不是CP_936，你可能需要直接指定CP_936。
    int mbcharCount = WideCharToMultiByte(CP_ACP, 0, &wcharBuffer[0], -1, nullptr, 0, nullptr, nullptr);  
    std::vector<uint8_t> mbcharBuffer(mbcharCount);  
    WideCharToMultiByte(CP_ACP, 0, &wcharBuffer[0], -1, reinterpret_cast<LPSTR>(&mbcharBuffer[0]), mbcharCount, nullptr, nullptr);  
    
    //调整输出字节数组的大小以去除未使用的空间
     int i,osize = mbcharBuffer.size();
     for(i = 0; i < osize; i++)
     {
         if(mbcharBuffer[i] == 0)
         {
             mbcharBuffer.resize(i);
             break;
         }
     }

    return mbcharBuffer; 
}
#endif


#if SystemFlag == SystemFlag_Linux
// 函数：将 CP-936编码的字节数组 转换为 UTF-8字符串
std::string FUN_StringConv_936ByteArrayToUTF8Str(char *cp936_bytes, uint32_t size)
{   
    // 输入和输出缓冲区  

    std::string utf8_string;  
 
    char outbuf[4 * size] = {}; // UTF-8编码可能占用更多空间，通常是原编码的4倍  
    char *outptr = outbuf;   

    // 设置输入和输出缓冲区的大小  
    size_t inbytesleft = size;
    size_t outbytesleft = sizeof(outbuf);  

  
    // 设置iconv转换描述符  
    iconv_t cd = iconv_open("UTF-8", "GB2312"); // 注意：GB2312通常用于简体中文，与CP-936相似但并非完全相同  

    if (cd == (iconv_t)-1) {  

        // perror("iconv_open");  

        return "";  
    }  

    // 执行转换  
    if (iconv(cd, &cp936_bytes, &inbytesleft, &outptr, &outbytesleft) == (size_t)-1) 
    {  
        // perror("iconv");  
        iconv_close(cd);  
        return "";  
    }    

    // 转换成功，将输出缓冲区的内容复制到utf8_string中  
    utf8_string.assign(outbuf, outptr - outbuf); 

    // 关闭转换描述符  
    iconv_close(cd);  

  

    return utf8_string;
}
#elif SystemFlag == SystemFlag_Windows
std::string FUN_StringConv_936ByteArrayToUTF8Str(char *cp936_bytes, uint32_t size)
{
     int bufferSize1 = MultiByteToWideChar(CP_ACP, 0, &cp936_bytes[0], size, nullptr, 0);  
    std::vector<wchar_t> buffer1(bufferSize1);  
    MultiByteToWideChar(CP_ACP, 0, &cp936_bytes[0], size, &buffer1[0], bufferSize1); 

    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, &buffer1[0], bufferSize1, nullptr, 0, nullptr, nullptr);  
    std::vector<char> buffer(bufferSize);  
    WideCharToMultiByte(CP_UTF8, 0, &buffer1[0], bufferSize1, &buffer[0], bufferSize, nullptr, nullptr);  

    return std::string(&buffer[0], bufferSize);
}
#endif


