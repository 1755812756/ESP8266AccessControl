#include "Common.h"

Common::Common() {
}


//将字节数组转储为串行的十六进制值
String Common::ByteArrayToHEX(byte *buffer, byte bufferSize)
{
  String value = "";
  for (int i = 0; i < bufferSize; i++)
  {
    value += String(buffer[i] < 0x10 ? "0" : "");
    value += String(buffer[i], HEX);
  }
  return value;
}

//将字节数组转储为串行的十六进制值 反序
String Common::ByteArrayToHEXFan(byte *buffer, byte bufferSize)
{
  String value = "";
  for (int i = bufferSize - 1; i >= 0; i--)
  {
    value += String(buffer[i] < 0x10 ? "0" : "");
    value += String(buffer[i], HEX);
  }
  return value;
}

//获取WG卡号
long Common::HEXToWg26CardNo(String UID)
{
  UID = UID.substring(2);

  char b[5];
  strcpy(b, UID.c_str());
  long iOut;
  iOut = strtol(b, NULL, 16);
  return iOut;
}



//将串行的十六进制值转储为字节数组
void Common::hex_str_to_byte(char *hex_str, int length, unsigned char *result)
{
  char h, l;
  for (int i = 0; i < length / 2; i++)
  {
    if (*hex_str < 58)
    {
      h = *hex_str - 48;
    }
    else if (*hex_str < 71)
    {
      h = *hex_str - 55;
    }
    else
    {
      h = *hex_str - 87;
    }
    hex_str++;
    if (*hex_str < 58)
    {
      l = *hex_str - 48;
    }
    else if (*hex_str < 71)
    {
      l = *hex_str - 55;
    }
    else
    {
      l = *hex_str - 87;
    }
    hex_str++;
    *result++ = h << 4 | l;
  }
}

//时间戳转时间
String Common::timeTodatetime(time_t t) {
  struct tm *p;
  p = gmtime(&t);
  char s[80];
  strftime(s, 80, "%Y-%m-%d %H:%M:%S", p);
  return s;
}

//时间对比大小
int Common::compare(const char* time1, const char* time2)
{
  int year1, month1, day1, hour1, min1, sec1;
  int year2, month2, day2, hour2, min2, sec2;
  sscanf(time1, "%d-%d-%d %d:%d:%d", &year1, &month1, &day1, &hour1, &min1, &sec1);
  sscanf(time2, "%d-%d-%d %d:%d:%d", &year2, &month2, &day2, &hour2, &min2, &sec2);
  int tm1 = year1 * 10000 + month1 * 100 + day1;
  int tm2 = year2 * 10000 + month2 * 100 + day2;
  if (tm1 != tm2) return (tm1 > tm2) ? 1 : 0; //如果相等，大返回1，小返回0
  tm1 = hour1 * 3600 + min1 * 60 + sec1;
  tm2 = hour2 * 3600 + min2 * 60 + sec2; //将时分秒转换为秒数
  if (tm1 != tm2) return (tm1 > tm2) ? 1 : 0; //如果相等，大返回1，小返回0
  return 2;//到这里必然是相等了
}

