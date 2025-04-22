#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ESP32Servo.h>
//----------------------------------------------------------------------最后更新时间2024-04-23

const char* ssid     = "N1";
const char* password = "abc12345678";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800); // 使用国内NTP服务器

// 定义引脚
const int PIR_PIN = 13;      // 人体传感器连接的GPIO引脚
const int SERVO_PIN = 16;    // 舵机连接的GPIO引脚

// 定义舵机角度常量
const int INIT_ANGLE = 92;  // 初始角度
const int DETECT_ANGLE = 30; // 检测到人时的角度
const int RESET_ANGLE = 150;   // 未检测到人时的角度

// 定义时间常量（秒）
int COUNTDOWN_DURATION = 300; // （秒）倒计 时时间 300*1000ms

Servo myServo;               // 创建舵机对象
bool lastPIRState = false;   // 上一次人体传感器状态
unsigned long countdownStart = 0; // 倒计时开始时间
bool isCountingDown = false; // 是否正在倒计时



void setup() {
  Serial.begin(115200);
  
  // 新增WiFi扫描功能
  scanAndPrintWiFiNetworks(); // [6,8](@ref)

  // 连接WiFi
  wifiConnected();


  pinMode(PIR_PIN, INPUT);   // 设置人体传感器引脚为输入
  // 允许ESP32使用所有PWM通道
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.attach(SERVO_PIN); // 将舵机连接到指定引脚
  myServo.write(INIT_ANGLE); // 初始化舵机位置
  delay(1000);              // 等待舵机到位
  myServo.detach();//停止舵机的角度维持
}

void loop() {
  

  if(hour()<7||hour()>18){
      bool currentPIRState = digitalRead(PIR_PIN); // 读取当前人体传感器状态
      if (currentPIRState) {
        Serial.println("检测到人体，开灯!");
        lampOpen();
        int flag=COUNTDOWN_DURATION;
        while(flag>0){
          Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",year(), month(), day(), hour(), minute(), second());
          Serial.printf("倒计时：%d\n", flag);
          delay(1000);
          flag-=1;
          currentPIRState = digitalRead(PIR_PIN);
          if (currentPIRState){
              Serial.println("检测到人体，倒计时重置！");
              flag=COUNTDOWN_DURATION;
          }
        }
        Serial.println("倒计时结束，关灯!");
        lampClose();
      }
  }else{
      bool currentPIRState = digitalRead(PIR_PIN); // 读取当前人体传感器状态
      if(currentPIRState){
          Serial.println("检测到人体");
      }
      Serial.print("当前时间为白天：");
      Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",year(), month(), day(), hour(), minute(), second());
      delay(100);
  }
    if(WiFi.status()==WL_DISCONNECTED){
        Serial.println("WiFi已断开,正在重新连接!");
        wifiConnected(); //重连！
    }
}
//开灯
void lampOpen(){
    myServo.attach(SERVO_PIN); // 将舵机连接到指定引脚
    myServo.write(INIT_ANGLE); // 初始化舵机位置
    delay(500);              // 等待舵机到位
    myServo.write(DETECT_ANGLE); // 检测到人时的角度
    delay(3000);              // 等待舵机到位
    myServo.write(INIT_ANGLE); // 初始化舵机位置
    delay(500);              // 等待舵机到位
    myServo.detach();//停止舵机的角度维持
  
}
//关灯
void lampClose(){
    myServo.attach(SERVO_PIN); // 将舵机连接到指定引脚
    myServo.write(INIT_ANGLE); // 初始化舵机位置
    delay(500);              // 等待舵机到位
    myServo.write(RESET_ANGLE); // 未检测到人时的角度
    delay(3000);              // 等待舵机到位
    myServo.write(INIT_ANGLE); // 初始化舵机位置
    delay(500);              // 等待舵机到位
    myServo.detach();//停止舵机的角度维持
}

//wifi连接
void wifiConnected(){
    Serial.print("[WiFi] Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
      // Will try for about 10 seconds (20x 500ms)
    int tryDelay = 500;
    int numberOfTries = 20;

    // Wait for the WiFi event
    while (true) {
        
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            return;
            break;
          case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            Serial.print(".");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            // NTP时间同步
            Serial.println("[NTP] NTP Time update");
            timeClient.begin();
            while (!timeClient.update()) delay(500);
            setTime(timeClient.getEpochTime());
            timeClient.update();//更新时间
            return;
            break;
          default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(tryDelay);
        
        if(numberOfTries <= 0){
          Serial.print("[WiFi] Failed to connect to WiFi!");
          // Use disconnect function to force stop trying to connect
          WiFi.disconnect();
          return;
        } else {
          numberOfTries--;
        }
    }


}

// 新增函数：扫描并输出WiFi网络
void scanAndPrintWiFiNetworks() {
  Serial.println("\n开始扫描附近WiFi网络...");
  
  // 配置WiFi模式为STA并断开当前连接[8](@ref)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100); // 等待射频模块稳定
  
  int networksFound = WiFi.scanNetworks();
  if (networksFound == 0) {
    Serial.println("未发现任何WiFi网络");
  } else {
    Serial.printf("发现%d个网络:\n", networksFound);
    for (int i = 0; i < networksFound; ++i) {
      // 输出SSID和信号强度[6](@ref)
      Serial.printf("%2d: %-20s (强度:%4ddBm)%s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "*");
    }
  }
  WiFi.scanDelete(); // 清除扫描结果缓存[8](@ref)
}