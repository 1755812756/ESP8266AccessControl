#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include <ArduinoJson.h> //  ArduinoJson库

class Mjserver
{
  public:
    Mjserver();

    int relayio; //继电器针脚

    String fileSystem; //系统配置路径

    String fileMqtt; //Mqtt配置路径

    String filejurisdiction; //权限名单

    String filemjdata; //刷卡记录

    void begin();

    //监听服务
    void handleClient();

    int isloadFileSystem();//是否更新系统配置  0.未更新 1.已更新

    int isloadFileMqtt();//是否更新Mqtt配置  0.未更新 1.已更新

    int isloadFileJurisdiction(); //是否更新权限名单  0.未更新 1.已更新

    static int SystemSetUp(String OpenTime); //系统配置  OpenTime:开门时长

    static int MqttSetUp(String MqttUrl, String MqttPort, String MqttAccount, String MqttPwd); //MQTT配置    "MqttUrl":MQTT地址  "MqttPort":MQTT端口 "MqttAccount": MQTT账号  "MqttPwd": MQTT密码

    static int JurisdictionSave(String CardNo, String ValidityDate); //新增修改权限名单  CardNo:卡号  ValidityDate:有效期

    static int JurisdictionDelete(String CardNos); //新增修改权限名单  CardNos:卡号json
  private:

    //登录
    static void handlelogin();

    //写入系统配置
    static void handleSystemSetUp();

    //写入Mqtt配置
    static void handleMqttSetUp();

    //远程开门
    static void handleOpenDoor();

    //新增权限
    static void handleJurisdictionSave();

    //删除权限
    static void handleJurisdictionDelete();

    //重启设备
    static void handleReset();

    //处理用户浏览器的HTTP访问
    static void handleUserRequest();

    //建立returnJson接口返回信息
    static String returnJson(int Tag, String Message, String Data = "");

    //处理浏览器HTTP访问
    static bool handleFileRead(String resource);

    //获取文件类型
    static String getContentType(String filename);
};
