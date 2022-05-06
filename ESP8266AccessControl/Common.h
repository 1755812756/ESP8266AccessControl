#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <stdio.h>
#include <time.h>

class Common
{
  public:
    Common();

    //将字节数组转储为串行的十六进制值
    static String ByteArrayToHEX(byte *buffer, byte bufferSize);

    //将字节数组转储为串行的十六进制值 反序
    static String ByteArrayToHEXFan(byte *buffer, byte bufferSize);

    //获取WG卡号
    static long HEXToWg26CardNo(String UID);

    //将串行的十六进制值转储为字节数组
    static void hex_str_to_byte(char *hex_str, int length, unsigned char *result);
    
    //时间戳转换成时间
    static String timeTodatetime(time_t t);

    //时间对比大小
    static int compare(const char* time1, const char* time2);
  private:
};
#endif
