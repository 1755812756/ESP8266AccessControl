#### 介绍
ESP8266门禁
通过ESP8266接入RC522读卡器实现刷卡触发继电器开门操作
![输入图片说明](image.png)
#### 软件架构
1.增加API功能 使用LayUI前端界面 管理设备配置功能
![开门时间](https://images.gitee.com/uploads/images/2022/0506/123011_431e66f1_5546353.png "屏幕截图.png")
2.接入物联网MQTT通讯，实现微耕卡号权限下发，也是通过后端页面进行配置
![输入图片说明](https://images.gitee.com/uploads/images/2022/0506/123132_efe196b6_5546353.png "屏幕截图.png")
3.权限下发成功，会在设备记录提供了查看页面(txt存储)
![输入图片说明](https://images.gitee.com/uploads/images/2022/0506/123143_0a90f910_5546353.png "屏幕截图.png")
4.记录刷卡记录(txt存储)
![输入图片说明](https://images.gitee.com/uploads/images/2022/0506/123151_ac0ebe0f_5546353.png "屏幕截图.png")

#### 备注
 由于ESP8266闪存有限制大小现在、存储慢了以后就不会存储了
 ![接线图](https://images.gitee.com/uploads/images/2022/0506/123514_0095f8aa_5546353.png "屏幕截图.png")
 [读卡功能参考](https://blog.csdn.net/qq_31878883/article/details/88971935) 