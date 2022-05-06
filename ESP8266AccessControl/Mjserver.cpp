#include "Mjserver.h"

ESP8266WebServer esp8266_server(80);
int _relayio = 15; //IO14（D1），用来控制继电器

String _fileSystem = "/configure/System.txt"; //系统配置文件路径

String _fileMqtt = "/configure/Mqtt.txt"; //Mqtt配置文件路径

String _filejurisdiction = "/jurisdiction/jurisdiction.json"; //权限名单文件路径

String _filemjdata = "/jurisdiction/mjdata.json"; //刷卡记录文件路径

int _isloadFileSystem = 0; //是否更新系统配置  0.未更新 1.已更新

int _isloadFileMqtt = 0; //是否更新Mqtt配置  0.未更新 1.已更新

int _isloadFileJurisdiction = 0; //是否更新权限名单  0.未更新 1.已更新

Mjserver::Mjserver() {
  relayio = _relayio;
}

void Mjserver::begin()
{
  fileSystem = _fileSystem;
  fileMqtt = _fileMqtt;
  filejurisdiction = _filejurisdiction;
  filemjdata = _filemjdata;

  esp8266_server.on("/login", handlelogin);//登录

  esp8266_server.on("/SystemSetUp", handleSystemSetUp);//写入系统配置

  esp8266_server.on("/MqttSetUp", handleMqttSetUp);//Mqtt系统配置

  esp8266_server.on("/OpenDoor", handleOpenDoor); //远程开门

  esp8266_server.on("/JurisdictionSave", handleJurisdictionSave); //新增权限

  esp8266_server.on("/JurisdictionDelete", handleJurisdictionDelete); //删除权限

  esp8266_server.on("/Reset", handleReset); //重启设备

  esp8266_server.onNotFound(handleUserRequest); // 处理其它网络请求

  // 启动网站服务
  esp8266_server.begin();
  Serial.println("HTTP server started");

  // 开启服务
  if (SPIFFS.begin())
  { // 启动闪存文件系统
    Serial.println("SPIFFS Started.");
  }
  else
  {
    Serial.println("SPIFFS Failed to Start.");
  }
}

//监听服务
void Mjserver::handleClient()
{
  esp8266_server.handleClient();
}

int Mjserver::isloadFileSystem()
{
  int value = _isloadFileSystem;
  _isloadFileSystem = 0;
  return value;
}

int Mjserver::isloadFileMqtt()
{
  int value = _isloadFileMqtt;
  _isloadFileMqtt = 0;
  return value;
}

int Mjserver::isloadFileJurisdiction()
{
  int value = _isloadFileJurisdiction;
  _isloadFileJurisdiction = 0;
  return value;
}


//登录
void Mjserver::handlelogin()
{
  String username = esp8266_server.arg("username");
  Serial.println(username);
  String password = esp8266_server.arg("password");
  Serial.println(password);
  int Tag = 0;
  String Message = "";
  if (username == "admin" && password == "123456")
  {
    Tag = 1;
  }
  else
  {
    Message = "账号或密码错误!";
  }
  esp8266_server.send(200, "application/json", returnJson(Tag, Message)); //发送网页
}

//写入系统配置
int Mjserver::SystemSetUp(String OpenTime) {
  int Tag = 0;
  if (SPIFFS.exists(_fileSystem)) {
    File dataFile = SPIFFS.open(_fileSystem, "w");// 建立File对象用于向SPIFFS中的file对象（即/notes.txt）写入信息
    dataFile.println("{\"OpenTime\":" + OpenTime + "}"); // 向dataFile添加字符串信息
    dataFile.close();                           // 完成文件操作后关闭文件
    Serial.println("SPIFFS fileSystem");
    _isloadFileSystem = 1;
    Tag = 1;
  }
  return Tag;
}

void Mjserver::handleSystemSetUp()
{
  String OpenTime = esp8266_server.arg("OpenTime");
  Serial.println(OpenTime);
  int Tag =  SystemSetUp(OpenTime);
  esp8266_server.send(200, "application/json", returnJson(Tag, "")); //发送网页
}

//写入Mqtt配置
int Mjserver::MqttSetUp(String MqttUrl, String MqttPort, String MqttAccount, String MqttPwd) {
  int Tag = 0;
  if (SPIFFS.exists(_fileMqtt)) {
    File dataFile = SPIFFS.open(_fileMqtt, "w");// 建立File对象用于向SPIFFS中的file对象（即/notes.txt）写入信息
    dataFile.println("{\"MqttUrl\":\"" + MqttUrl + "\",\"MqttPort\":" + MqttPort + ",\"MqttAccount\":\"" + MqttAccount + "\",\"MqttPwd\":\"" + MqttPwd + "\"}"); // 向dataFile添加字符串信息
    dataFile.close();                           // 完成文件操作后关闭文件
    Serial.println("SPIFFS fileSystem");
    _isloadFileMqtt = 1;
    Tag = 1;
  }
  return Tag;
}
void Mjserver::handleMqttSetUp()
{
  String MqttUrl = esp8266_server.arg("MqttUrl");
  String MqttPort = esp8266_server.arg("MqttPort");
  String MqttAccount = esp8266_server.arg("MqttAccount");
  String MqttPwd = esp8266_server.arg("MqttPwd");
  int Tag = MqttSetUp(MqttUrl, MqttPort, MqttAccount, MqttPwd);
  esp8266_server.send(200, "application/json", returnJson(Tag, "")); //发送网页
}

//远程开门
void Mjserver::handleOpenDoor()
{
  digitalWrite(_relayio, LOW); //LED 点亮
  File dataFile = SPIFFS.open(_fileSystem, "r");
  String OpenTimeJson = "";
  //读取文件内容并且通过串口监视器输出文件信息
  for (int i = 0; i < dataFile.size(); i++) {
    OpenTimeJson += (char)dataFile.read();
  }
  const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, OpenTimeJson);
  int OpenTime = doc["OpenTime"].as<int>();
  delay(OpenTime * 1000); //延时关门
  digitalWrite(_relayio, HIGH); //LED 点亮

  esp8266_server.send(200, "application/json", returnJson(1, "")); //发送网页
}

//新增权限
int Mjserver::JurisdictionSave(String CardNo, String ValidityDate) {
  int Tag = 0;
  if (SPIFFS.exists(_filejurisdiction)) {
    File dataFile = SPIFFS.open(_filejurisdiction, "r");
    String jurisdictionJson;
    //读取文件内容并且通过串口监视器输出文件信息
    for (int i = 0; i < dataFile.size(); i++) {
      jurisdictionJson += (char)dataFile.read();
    }
    //Serial.println(jurisdictionJson);
    size_t jurisdictionCapacity  = jurisdictionJson.length() * 2 + 100;
    DynamicJsonDocument doc(jurisdictionCapacity);
    DeserializationError error = deserializeJson(doc, jurisdictionJson);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      //Message = "序列化Json失败";
      //esp8266_server.send(200, "application/json", returnJson(Tag, Message)); //发送网页
      return Tag;
    }
    bool isjurisdiction = false;
    JsonArray  item = doc.as<JsonArray>();
    for (int i = 0; i < item.size(); i++) {
      long _CardNo = item[i]["CardNo"].as<long>();
      if (_CardNo == atoi(CardNo.c_str())) { //存在卡号则更新有效期
        isjurisdiction = true;
        item[i]["ValidityDate"] = ValidityDate;
        break;
      }
    }
    //不存在卡号则追加
    if (!isjurisdiction) {
      int i = item.size();
      item[i]["CardNo"] = CardNo;
      item[i]["ValidityDate"] = ValidityDate;
    }

    String jsonList;
    serializeJson(item, jsonList);
    //Serial.println(jsonList);

    File dataFile2 = SPIFFS.open(_filejurisdiction, "w");// 建立File对象用于向SPIFFS中的file对象（即/notes.txt）写入信息  追加
    dataFile2.println(jsonList); // 向dataFile添加字符串信息
    dataFile2.close();                           // 完成文件操作后关闭文件
    Serial.println("SPIFFS fileSystem");
    Tag = 1;
    _isloadFileJurisdiction = 1; //状态改为更新
  }
  return Tag;
}
void Mjserver::handleJurisdictionSave()
{
  String CardNo = esp8266_server.arg("CardNo");
  String ValidityDate = esp8266_server.arg("ValidityDate");
  int Tag = JurisdictionSave(CardNo, ValidityDate);
  //确认闪存中是否有file_name文件
  esp8266_server.send(200, "application/json", returnJson(Tag, "")); //发送网页
}



//删除权限
int Mjserver::JurisdictionDelete(String CardNos)
{
  int Tag = 0;
  size_t deleteCapacity  = CardNos.length() + (CardNos.length() * 30);
  DynamicJsonDocument docdelete(deleteCapacity);
  DeserializationError error = deserializeJson(docdelete, CardNos);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
//    Message = "序列化Json失败";
//    esp8266_server.send(200, "application/json", returnJson(Tag, Message)); //发送网页
      return Tag;
  }
  JsonArray  deleteitem = docdelete.as<JsonArray>();

  //确认闪存中是否有file_name文件
  if (SPIFFS.exists(_filejurisdiction)) {
    File dataFile = SPIFFS.open(_filejurisdiction, "r");
    String jurisdictionJson;
    //读取文件内容并且通过串口监视器输出文件信息
    for (int i = 0; i < dataFile.size(); i++) {
      jurisdictionJson += (char)dataFile.read();
    }
    Serial.println(jurisdictionJson);
    size_t jurisdictionCapacity  = jurisdictionJson.length() * 2 + 100;
    DynamicJsonDocument doc(jurisdictionCapacity);
    DeserializationError error = deserializeJson(doc, jurisdictionJson);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      //Message = "序列化Json失败";
      //esp8266_server.send(200, "application/json", returnJson(Tag, Message)); //发送网页
      return Tag;
    }
    bool isjurisdiction = false;
    JsonArray  item = doc.as<JsonArray>();
    for (int j = 0; j < deleteitem.size(); j++) {
      long _CardNoDel = deleteitem[j]["CardNo"].as<long>();
      for (int i = 0; i < item.size(); i++) {
        long _CardNo = item[i]["CardNo"].as<long>();
        if (_CardNo == _CardNoDel) { //存在卡号则更新有效期
          item.remove(i);
          break;
        }
      }
    }

    String jsonList;
    serializeJson(item, jsonList);
    Serial.println(jsonList);

    File dataFile2 = SPIFFS.open(_filejurisdiction, "w");// 建立File对象用于向SPIFFS中的file对象（即/notes.txt）写入信息  追加
    dataFile2.println(jsonList); // 向dataFile添加字符串信息
    dataFile2.close();                           // 完成文件操作后关闭文件
    Serial.println("SPIFFS fileSystem");
    Tag = 1;
    _isloadFileJurisdiction = 1; //状态改为更新
  }
  return Tag;
}
void Mjserver::handleJurisdictionDelete()
{
  String CardNos = esp8266_server.arg("CardNos");
  Serial.println(CardNos);
  int Tag = JurisdictionDelete(CardNos);
  esp8266_server.send(200, "application/json", returnJson(Tag, "")); //发送网页
}

//重启设备
void Mjserver::handleReset()
{
  esp8266_server.send(200, "application/json", returnJson(1, "设备正在重启")); //发送网页
  delay(500);
  ESP.restart();
}

//建立returnJson接口返回信息
String Mjserver::returnJson(int Tag, String Message, String Data)
{
  // 开始ArduinoJson Assistant的serialize代码
  const size_t capacity = JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(3) + 140;
  DynamicJsonDocument doc(capacity);

  doc["Tag"] = Tag;
  doc["Message"] = Message;
  doc["Data"] = Data;

  String jsonCode;
  serializeJson(doc, jsonCode);
  Serial.print("info Json Code: ");
  Serial.println(jsonCode);

  return jsonCode;
}

// 处理用户浏览器的HTTP访问
void Mjserver::handleUserRequest()
{

  // 获取用户请求资源(Request Resource）
  String reqResource = esp8266_server.uri();
  Serial.print("reqResource: ");
  Serial.println(reqResource);

  // 通过handleFileRead函数处处理用户请求资源
  bool fileReadOK = handleFileRead(reqResource);

  // 如果在SPIFFS无法找到用户访问的资源，则回复404 (Not Found)
  if (!fileReadOK)
  {
    esp8266_server.send(404, "text/plain", "404 Not Found");
  }
}

//处理浏览器HTTP访问
bool Mjserver::handleFileRead(String resource)
{

  if (resource.endsWith("/"))
  { // 如果访问地址以"/"为结尾
    resource = "/login.html"; // 则将访问地址修改为/index.html便于SPIFFS访问
  }

  String contentType = getContentType(resource); // 获取文件类型

  if (SPIFFS.exists(resource))
  { // 如果访问的文件可以在SPIFFS中找到
    File file = SPIFFS.open(resource, "r");       // 则尝试打开该文件
    esp8266_server.streamFile(file, contentType); // 并且将该文件返回给浏览器
    file.close();                                 // 并且关闭文件
    return true;                                  // 返回true
  }
  return false; // 如果文件未找到，则返回false
}

// 获取文件类型
String Mjserver::getContentType(String filename)
{
  if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}
