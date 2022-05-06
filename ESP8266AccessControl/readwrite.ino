// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>

// #include <DNSServer.h>
// #include <ESP8266WebServer.h>
// #include <FS.h>

// ESP8266WebServer esp8266_server(80);

#include <WiFiManager.h>
String WiFilocalIP;
uint8_t MAC_array_STA[6];
char MAC_char_STA[18];

#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN 5 // 配置针脚
#define SS_PIN 4
MFRC522 mfrc522(SS_PIN, RST_PIN); // 创建新的RFID实例
MFRC522::MIFARE_Key key;

#include "Mjserver.h"
Mjserver mjserver;
int OpenTime = 0; //开门时长

String MqttUrl;//Mqtt地址
int MqttPort;//Mqtt端口
String MqttAccount;//Mqtt账号
String MqttPwd;//Mqtt密码

String mjRequest = "mj/request";
String mjMacRequest;

int MqttLoadCount = 5; //Mqtt连接次数

size_t jurisdictionCapacity;//权限名单Json长度
String jurisdictionJson;//权限名单Json

#include "Common.h"
Common common;

#include <PubSubClient.h>
WiFiClient espClient;                                                         // 定义wifiClient实例
PubSubClient client(espClient);                                         // 定义PubSubClient的实例
long lastMsg = 0;

int ioD1 = mjserver.relayio; //IO14（D1），用来控制继电器
/*  io数值对应关系：
    D0=16，D1=5，D2=4，D3=0，D4=2，D5=14，D6=12，D7=13,D8=15
    对于新手建议使用这些io，使用比较稳定，不会影响系统正常运行
*/
void setup()
{
  Serial.begin(9600); // 设置串口波特率为9600
  while (!Serial)
    ; // 如果串口没有打开，则死循环下去不进行下面的操作

  //  pinMode(LED_BUILTIN, OUTPUT);   // 初始化NodeMCU控制板载LED引脚为OUTPUT
  //  digitalWrite(LED_BUILTIN, HIGH );// 初始化LED引脚状态

  pinMode(ioD1, OUTPUT);
  digitalWrite(ioD1, 1);


  // 建立WiFiManager对象
  WiFiManager wifiManager;

  // 清除ESP8266所存储的WiFi连接信息以便测试WiFiManager工作效果
  //wifiManager.resetSettings();

  wifiManager.setConfigPortalTimeout(60);
  // 自动连接WiFi。以下语句的参数是连接ESP8266时的WiFi名称
  wifiManager.autoConnect("ZR_Cardissuer");

  // 打印IP地址
  WiFilocalIP = WiFi.localIP().toString().c_str();
  Serial.println(WiFilocalIP);


  //获取MAC
  WiFi.macAddress(MAC_array_STA);
  for (int i = 0; i < sizeof(MAC_array_STA); ++i) {
    sprintf(MAC_char_STA, "%s%02x", MAC_char_STA, MAC_array_STA[i]);
  }
  mjMacRequest = "mj/" + String(MAC_char_STA) + "/request";//mqtt主题

  mjserver.begin(); //启动门禁后台服务(ESP8266)

  //启动连接mfrc522
  SPI.begin();        // SPI开始
  mfrc522.PCD_Init(); // Init MFRC522 card

  LoadMqttFile(mjserver.fileMqtt); //读取MQTT配置
  LoadFilejurisdiction(mjserver.filejurisdiction);//读取权限名单数据

  //启动Mqtt
  client.setServer(MqttUrl.c_str(), MqttPort); //设定MQTT服务器与使用的端口
  client.setCallback(callback);//设定回调方式，当ESP8266收到订阅消息时会调用此方法

  //启动NTP时间
  timeClient.begin();
  timeClient.setTimeOffset(28800);

}

//循环执行
void loop()
{
  timeClient.update();
  //获取时间戳
  unsigned long epochTime = timeClient.getEpochTime();
  String datetime = common.timeTodatetime(epochTime);

  mjserver.handleClient(); //监听服务

  //更新Mqtt配置
  if (mjserver.isloadFileMqtt() == 1) {
    Serial.println("isloadFileMqtt");
    LoadMqttFile(mjserver.fileMqtt);
    client.setServer(MqttUrl.c_str(), MqttPort); //设定MQTT服务器与使用的端口
    client.setCallback(callback);
    reconnect();
    MqttLoadCount = 5;
  }

  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;

    if (!client.connected() && MqttLoadCount > 0) {
      MqttLoadCount--;
      reconnect();
    }

    if (client.connected()) {
      String outTopicJson = "{\"timeStamp\":" + (String)epochTime + ",\"dateTime\":\"" + datetime + "\",\"ip\":\"" + WiFilocalIP + "\",\"mac\":\"" + MAC_char_STA + "\"}";
      client.publish("mj/heartbeat", outTopicJson.c_str());
    }
  }
  client.loop();

  // 寻找新卡
  if (!mfrc522.PICC_IsNewCardPresent())
    return;

  // 选择一张卡
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  // 显示卡片的详细信息
  Serial.print(F("卡片 UID:"));
  String keyValue =  common.ByteArrayToHEX(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println(keyValue);
  Serial.println();
  String UID =  common.ByteArrayToHEXFan(mfrc522.uid.uidByte, mfrc522.uid.size); //16进制反序卡号
  long WgCardNo =  common.HEXToWg26CardNo(UID);                                  //获取Wg26卡号
  Serial.println(WgCardNo);

  Serial.print(F("卡片类型: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // 检查兼容性
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K)
  {
    Serial.println(F("仅仅适合Mifare Classic卡的读写"));
    return;
  }
  //权限名单更新则重新读取json
  if (mjserver.isloadFileJurisdiction() == 1) {
    LoadFilejurisdiction(mjserver.filejurisdiction);//读取权限名单数据
  }
  bool isjurisdiction = isjurisdictionCardNo(WgCardNo, datetime.c_str());
  //权限验证
  if (isjurisdiction)
  {
    digitalWrite(ioD1, LOW); //LED 点亮

    AddMjData(mjserver.filemjdata, WgCardNo, datetime.c_str());//写入数据

    //发送MQTT数据
    if (client.connected()) {
      String responseJson = "{\"timeStamp\":" + (String)epochTime + ",\"dateTime\":\"" + datetime + "\",\"CardNo\":\"" + WgCardNo + "\"}";
      client.publish("mj/response", responseJson.c_str());
    }

    //读卡开门时长 isloadFileSystem数据更新是触发
    if (mjserver.isloadFileSystem() == 1 || OpenTime == 0) {
      File dataFile = SPIFFS.open(mjserver.fileSystem, "r");
      String OpenTimeJson = "";
      //读取文件内容并且通过串口监视器输出文件信息
      for (int i = 0; i < dataFile.size(); i++) {
        OpenTimeJson += (char)dataFile.read();
      }
      const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, OpenTimeJson);
      OpenTime = doc["OpenTime"].as<int>();
    }
    delay(OpenTime * 1000); //延时关门
    digitalWrite(ioD1, HIGH); //LED 点亮
  }

  // 停止 PICC
  mfrc522.PICC_HaltA();
  //停止加密PCD
  mfrc522.PCD_StopCrypto1();

}

//读取MQTT配置
void LoadMqttFile(String fileMqtt) {

  File dataFile = SPIFFS.open(fileMqtt, "r");
  String MqttJson = "";
  //读取文件内容并且通过串口监视器输出文件信息
  for (int i = 0; i < dataFile.size(); i++) {
    MqttJson += (char)dataFile.read();
  }
  const size_t capacity = JSON_OBJECT_SIZE(4) + 90;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, MqttJson);
  MqttUrl = doc["MqttUrl"].as<String>();
  MqttPort = doc["MqttPort"].as<int>();
  MqttAccount = doc["MqttAccount"].as<String>();
  MqttPwd = doc["MqttPwd"].as<String>();
}

//加载人员名单
void LoadFilejurisdiction(String file) {
  File dataFile = SPIFFS.open(file, "r");

  jurisdictionJson = "";
  //读取文件内容并且通过串口监视器输出文件信息
  for (int i = 0; i < dataFile.size(); i++) {
    jurisdictionJson += (char)dataFile.read();
  }
  //Serial.println(jurisdictionJson);
  jurisdictionCapacity  = dataFile.size() + (dataFile.size() * 30);
}

//验证卡号是否有权限
bool isjurisdictionCardNo(long CardNo, const char* datetime) {
  DynamicJsonDocument doc(jurisdictionCapacity);
  DeserializationError error = deserializeJson(doc, jurisdictionJson);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }
  bool isjurisdiction = false;
  for (JsonObject item :  doc.as<JsonArray>()) {
    long _CardNo = item["CardNo"].as<long>(); // "12742218"
    const char* ValidityDate = item["ValidityDate"]; // "2021-08-12 13:45"
    int isValidityDate = common.compare(ValidityDate, datetime);
    if (_CardNo == CardNo && isValidityDate > 0) {
      isjurisdiction = true;
      return true;
    }
  }
  return isjurisdiction;
}

//MQTT监听
void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);   // 打印主题信息
  Serial.print("] ");
  String paybodyJson;
  for (int i = 0; i < length; i++) {
    paybodyJson += String((char)payload[i]);
  }
  Serial.println(paybodyJson);
  Serial.println();
  if (String(topic) == mjRequest || String(topic) == mjMacRequest) {
    size_t jurisdictionCapacity  = paybodyJson.length() * 2 + 100;
    DynamicJsonDocument doc(jurisdictionCapacity);
    DeserializationError error = deserializeJson(doc, paybodyJson);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    String cmd = doc["cmd"].as<String>();
    String cmd_id = doc["cmd_id"].as<String>();
    String client_id = doc["client_id"].as<String>();
    Serial.println(cmd);
    Serial.println(cmd_id);
    Serial.println(client_id);
    if (cmd == "restart") { //重启设备
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"restart\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":1}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
      delay(500);
      ESP.restart();
    } else if (cmd == "systemset") {//系统配置
      int code = mjserver.SystemSetUp(doc["OpenTime"].as<String>());
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"systemset\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":" + code + "}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
    } else if (cmd == "mqttset") { //MQTT配置
      int code = mjserver.MqttSetUp(doc["MqttUrl"].as<String>(), doc["MqttPort"].as<String>(), doc["MqttAccount"].as<String>(), doc["MqttPwd"].as<String>());
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"mqttset\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":" + code + "}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
    } else if (cmd == "jurisdictionsave") { //权限新增或修改
      int code = mjserver.JurisdictionSave(doc["CardNo"].as<String>(), doc["ValidityDate"].as<String>());
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"jurisdictionsave\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":" + code + "}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
    } else if (cmd == "jurisdictiondelete") { //删除权限
      int code = mjserver.JurisdictionDelete(doc["CardNos"].as<String>());
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"jurisdictiondelete\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":" + code + "}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
    } else if (cmd == "opendoor") { //远程开门
      digitalWrite(ioD1, LOW); //LED 点亮
      String responseJson = "{\"reply\":\"ACK\",\"cmd\":\"opendoor\",\"cmd_id\":\"" + cmd_id + "\",\"mac\":\"" + MAC_char_STA + "\",\"code\":1}";
      client.publish(("mj/" + client_id + "/response").c_str(), responseJson.c_str());
      
      delay(OpenTime * 1000); //延时关门
      digitalWrite(ioD1, HIGH); //LED 关闭
    }
  }
}

//MQTT连接
void reconnect() {
  Serial.print("开始连接 MQTT...");
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);

  // Attempt to connect
  if (client.connect(clientId.c_str(), MqttAccount.c_str(), MqttPwd.c_str())) {
    Serial.println("连接成功");
    // 连接成功时订阅主题
    client.subscribe(mjRequest.c_str());
    client.subscribe(mjMacRequest.c_str());
  } else {
    Serial.print("连接失败, 错误=");
    Serial.print(client.state());
    Serial.println(" 1秒后进行重连 ");
    // Wait 5 seconds before retrying
  }
}

//新增刷卡记录
void AddMjData(String file_name, long CardNo, const char* ValidityDate)
{
  //确认闪存中是否有file_name文件
  if (SPIFFS.exists(file_name)) {
    File dataFile = SPIFFS.open(file_name, "r");
    String jurisdictionJson;
    //读取文件内容并且通过串口监视器输出文件信息
    for (int i = 0; i < dataFile.size(); i++) {
      jurisdictionJson += (char)dataFile.read();
    }

    size_t jurisdictionCapacity  = jurisdictionJson.length() * 2 + 100;
    DynamicJsonDocument doc(jurisdictionCapacity);
    DeserializationError error = deserializeJson(doc, jurisdictionJson);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonArray  item = doc.as<JsonArray>();

    int i = item.size();
    if (i > 5) { //数据大于1000条则清除第一条
      item.remove(0);
      i = item.size();
    }
    item[i]["CardNo"] = CardNo;
    item[i]["CardDate"] = ValidityDate;

    String jsonList;
    serializeJson(item, jsonList);

    File dataFile2 = SPIFFS.open(file_name, "w");// 建立File对象用于向SPIFFS中的file对象（即/notes.txt）写入信息  追加
    dataFile2.println(jsonList); // 向dataFile添加字符串信息
    dataFile2.close();                           // 完成文件操作后关闭文件
    Serial.println("SPIFFS fileSystem");
  }
}

