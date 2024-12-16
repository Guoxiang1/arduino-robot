// -----------------------------------------------------
#include <ESP8266WiFi.h>
/* 依赖 PubSubClient 2.4.0 */
#include <PubSubClient.h>
/* 依赖 ArduinoJson 5.13.4 */
#include <ArduinoJson.h>
#include <SparkFun_SHTC3.h>
#include <Servo.h>
   


/* 连接您的WIFI SSID和密码 */
#define WIFI_SSID         "iQOO11Pro"
#define WIFI_PASSWD       "123456789"

/* 设备的三元组信息*/
#define PRODUCT_KEY       "k1y8ukRlIUv"
#define DEVICE_NAME       "ESP8266"
#define DEVICE_SECRET     "b589d665cb289eb777a79766949cfb8b"
#define REGION_ID         "cn-shanghai"

/* 线上环境域名和端口号，不需要改 */
#define MQTT_SERVER       PRODUCT_KEY ".iot-as-mqtt." REGION_ID ".aliyuncs.com"
#define MQTT_PORT         1883
#define MQTT_USRNAME      DEVICE_NAME "&" PRODUCT_KEY

#define CLIENT_ID         "k1y8ukRlIUv.ESP8266|securemode=2,signmethod=hmacsha256,timestamp=1730812789212|"
// password来自sha1加密
#define MQTT_PASSWD       "ad94b77b8dac44c2095aea1d6ea1765f9ff08ec301c7ce04e5a6049f245a974c"

#define ALINK_BODY_FORMAT         "{\"id\":\"ESP8266\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
#define ALINK_TOPIC_PROP_POST     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"

unsigned long lastMs = 0;
float RH,T,RH_sum,T_sum;
unsigned char count=0;
WiFiClient espClient;
PubSubClient  client(espClient);
SHTC3 mySHTC3;

const char *ssid = "esp8266_666";   //wifi名
const char *password = "12345678";  //wifi密码
WiFiServer server(8266);            //端口
//unsigned int testnumber;

const int analogInPin = A0;  // 模拟输入引脚
int BattaryValue = 0;        // 电池电压值
int Battarylow = 15;         // 电池低电量指示灯引脚

const int numberOfServos = 8; // 舵机数量
const int numberOfACE = 9; // 动作代码元素数量
int servoPos[] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // 舵机当前位置
int servoCal[] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // 舵机校准数据,根据舵机具体安装情况而定
int servoPrgPeriod = 20; // 20 ms
int ActonFlag = 0;
Servo servo[numberOfServos];  // 舵机对象

//*********动作组********
// --------------------------------------------------------------------------------
// Servo zero position
// -------------------------- P16, P05, P04, P00, P02, P14, P12, P13
int servoAct00 [] PROGMEM = { 135,  45, 135,  45,  45, 135,  45, 135 };

// Zero
int servoPrg00step = 1;
int servoPrg00 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {  135,  45, 135,  45,  45, 135,  45, 135,  500  }, // zero position
};

// Standby
int servoPrg01step = 1;
int servoPrg01 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
//  {   90,  90,  90,  90,  90,  90,  90,  90,  300  }, // servo center point
  {   70,  90,  90, 110, 110,  90,  90,  70,  500  }, // standby
};

// Forward
int servoPrg02step = 11;
int servoPrg02 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   90,  90,  90, 110, 110,  90,  45,  90,  100  }, // leg1,4 up; leg4 fw
  {   70,  90,  90, 110, 110,  90,  45,  70,  100  }, // leg1,4 dn
  {   70,  90,  90,  90,  90,  90,  45,  70,  100  }, // leg2,3 up
  {   70,  45, 135,  90,  90,  90,  90,  70,  100  }, // leg1,4 bk; leg2 fw
  {   70,  45, 135, 110, 110,  90,  90,  70,  100  }, // leg2,3 dn
  {   90,  90, 135, 110, 110,  90,  90,  90,  100  }, // leg1,4 up; leg1 fw
  {   90,  90,  90, 110, 110, 135,  90,  90,  100  }, // leg2,3 bk
  {   70,  90,  90, 110, 110, 135,  90,  70,  100  }, // leg1,4 dn
  {   70,  90,  90, 110,  90, 135,  90,  70,  100  }, // leg3 up
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg3 fw dn
};

// Backward
int servoPrg03step = 11;
int servoPrg03 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   90,  45,  90, 110, 110,  90,  90,  90,  100  }, // leg4,1 up; leg1 fw
  {   70,  45,  90, 110, 110,  90,  90,  70,  100  }, // leg4,1 dn
  {   70,  45,  90,  90,  90,  90,  90,  70,  100  }, // leg3,2 up
  {   70,  90,  90,  90,  90, 135,  45,  70,  100  }, // leg4,1 bk; leg3 fw
  {   70,  90,  90, 110, 110, 135,  45,  70,  100  }, // leg3,2 dn
  {   90,  90,  90, 110, 110, 135,  90,  90,  100  }, // leg4,1 up; leg4 fw
  {   90,  90, 135, 110, 110,  90,  90,  90,  100  }, // leg3,1 bk
  {   70,  90, 135, 110, 110,  90,  90,  70,  100  }, // leg4,1 dn
  {   70,  90, 135,  90, 110,  90,  90,  70,  100  }, // leg2 up
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg2 fw dn
};

// Move Left
int servoPrg04step = 11;
int servoPrg04 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   70,  90,  45,  90,  90,  90,  90,  70,  100  }, // leg3,2 up; leg2 fw
  {   70,  90,  45, 110, 110,  90,  90,  70,  100  }, // leg3,2 dn
  {   90,  90,  45, 110, 110,  90,  90,  90,  100  }, // leg1,4 up
  {   90, 135,  90, 110, 110,  45,  90,  90,  100  }, // leg3,2 bk; leg1 fw
  {   70, 135,  90, 110, 110,  45,  90,  70,  100  }, // leg1,4 dn
  {   70, 135,  90,  90,  90,  90,  90,  70,  100  }, // leg3,2 up; leg3 fw
  {   70,  90,  90,  90,  90,  90, 135,  70,  100  }, // leg1,4 bk
  {   70,  90,  90, 110, 110,  90, 135,  70,  100  }, // leg3,2 dn
  {   70,  90,  90, 110, 110,  90, 135,  90,  100  }, // leg4 up
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg4 fw dn
};

// Move Right
int servoPrg05step = 11;
int servoPrg05 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   70,  90,  90,  90,  90,  45,  90,  70,  100  }, // leg2,3 up; leg3 fw
  {   70,  90,  90, 110, 110,  45,  90,  70,  100  }, // leg2,3 dn
  {   90,  90,  90, 110, 110,  45,  90,  90,  100  }, // leg4,1 up
  {   90,  90,  45, 110, 110,  90, 135,  90,  100  }, // leg2,3 bk; leg4 fw
  {   70,  90,  45, 110, 110,  90, 135,  70,  100  }, // leg4,1 dn
  {   70,  90,  90,  90,  90,  90, 135,  70,  100  }, // leg2,3 up; leg2 fw
  {   70, 135,  90,  90,  90,  90,  90,  70,  100  }, // leg4,1 bk
  {   70, 135,  90, 110, 110,  90,  90,  70,  100  }, // leg2,3 dn
  {   90, 135,  90, 110, 110,  90,  90,  70,  100  }, // leg1 up
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg1 fw dn
};

// Turn left
int servoPrg06step = 8;
int servoPrg06 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   90,  90,  90, 110, 110,  90,  90,  90,  100  }, // leg1,4 up
  {   90, 135,  90, 110, 110,  90, 135,  90,  100  }, // leg1,4 turn
  {   70, 135,  90, 110, 110,  90, 135,  70,  100  }, // leg1,4 dn
  {   70, 135,  90,  90,  90,  90, 135,  70,  100  }, // leg2,3 up
  {   70, 135, 135,  90,  90, 135, 135,  70,  100  }, // leg2,3 turn
  {   70, 135, 135, 110, 110, 135, 135,  70,  100  }, // leg2,3 dn
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg1,2,3,4 turn
};

// Turn right
int servoPrg07step = 8;
int servoPrg07 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // standby
  {   70,  90,  90,  90,  90,  90,  90,  70,  100  }, // leg2,3 up
  {   70,  90,  45,  90,  90,  45,  90,  70,  100  }, // leg2,3 turn
  {   70,  90,  45, 110, 110,  45,  90,  70,  100  }, // leg2,3 dn
  {   90,  90,  45, 110, 110,  45,  90,  90,  100  }, // leg1,4 up
  {   90,  45,  45, 110, 110,  45,  45,  90,  100  }, // leg1,4 turn
  {   70,  45,  45, 110, 110,  45,  45,  70,  100  }, // leg1,4 dn
  {   70,  90,  90, 110, 110,  90,  90,  70,  100  }, // leg1,2,3,4 turn
};

// Lie
int servoPrg08step = 1;
int servoPrg08 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {  110,  90,  90,  70,  70,  90,  90, 110,  500  }, // leg1,4 up
};

// Say Hi
int servoPrg09step = 4;
int servoPrg09 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {  120,  90,  90, 110,  60,  90,  90,  70,  200  }, // leg1, 3 down
  {   70,  90,  90, 110, 110,  90,  90,  70,  200  }, // standby
  {  120,  90,  90, 110,  60,  90,  90,  70,  200  }, // leg1, 3 down
  {   70,  90,  90, 110, 110,  90,  90,  70,  200  }, // standby
};

// Fighting
int servoPrg10step = 11;
int servoPrg10 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {  120,  90,  90, 110,  60,  90,  90,  70,  200  }, // leg1, 2 down
  {  120,  70,  70, 110,  60,  70,  70,  70,  200  }, // body turn left
  {  120, 110, 110, 110,  60, 110, 110,  70,  200  }, // body turn right
  {  120,  70,  70, 110,  60,  70,  70,  70,  200  }, // body turn left
  {  120, 110, 110, 110,  60, 110, 110,  70,  200  }, // body turn right
  {   70,  90,  90,  70, 110,  90,  90, 110,  200  }, // leg1, 2 up ; leg3, 4 down
  {   70,  70,  70,  70, 110,  70,  70, 110,  200  }, // body turn left
  {   70, 110, 110,  70, 110, 110, 110, 110,  200  }, // body turn right
  {   70,  70,  70,  70, 110,  70,  70, 110,  200  }, // body turn left
  {   70, 110, 110,  70, 110, 110, 110, 110,  200  }, // body turn right
  {   70,  90,  90,  70, 110,  90,  90, 110,  200  }  // leg1, 2 up ; leg3, 4 down
};

// Push up
int servoPrg11step = 11;
int servoPrg11 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  90,  90, 110, 110,  90,  90,  70,  300  }, // start
  {  100,  90,  90,  80,  80,  90,  90, 100,  400  }, // down
  {   70,  90,  90, 110, 110,  90,  90,  70,  500  }, // up
  {  100,  90,  90,  80,  80,  90,  90, 100,  600  }, // down
  {   70,  90,  90, 110, 110,  90,  90,  70,  700  }, // up
  {  100,  90,  90,  80,  80,  90,  90, 100, 1300  }, // down
  {   70,  90,  90, 110, 110,  90,  90,  70, 1800  }, // up
  {  135,  90,  90,  45,  45,  90,  90, 135,  200  }, // fast down
  {   70,  90,  90,  45,  60,  90,  90, 135,  500  }, // leg1 up
  {   70,  90,  90,  45, 110,  90,  90, 135,  500  }, // leg2 up
  {   70,  90,  90, 110, 110,  90,  90,  70,  500  }  // leg3, leg4 up
};

// Sleep
int servoPrg12step = 2;
int servoPrg12 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   30,  90,  90, 150, 150,  90,  90,  30,  200  }, // leg1,4 dn
  {   30,  45, 135, 150, 150, 135,  45,  30,  200  }, // protect myself
};

// Dancing 1
int servoPrg13step = 10;
int servoPrg13 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   90,  90,  90,  90,  90,  90,  90,  90,  300  }, // leg1,2,3,4 up
  {   50,  90,  90,  90,  90,  90,  90,  90,  300  }, // leg1 dn
  {   90,  90,  90, 130,  90,  90,  90,  90,  300  }, // leg1 up; leg2 dn
  {   90,  90,  90,  90,  90,  90,  90,  50,  300  }, // leg2 up; leg4 dn
  {   90,  90,  90,  90, 130,  90,  90,  90,  300  }, // leg4 up; leg3 dn
  {   50,  90,  90,  90,  90,  90,  90,  90,  300  }, // leg3 up; leg1 dn
  {   90,  90,  90, 130,  90,  90,  90,  90,  300  }, // leg1 up; leg2 dn
  {   90,  90,  90,  90,  90,  90,  90,  50,  300  }, // leg2 up; leg4 dn
  {   90,  90,  90,  90, 130,  90,  90,  90,  300  }, // leg4 up; leg3 dn
  {   90,  90,  90,  90,  90,  90,  90,  90,  300  }, // leg3 up
};

// Dancing 2
int servoPrg14step = 9;
int servoPrg14 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  45, 135, 110, 110, 135,  45,  70,  300  }, // leg1,2,3,4 two sides
  {  115,  45, 135,  65, 110, 135,  45,  70,  300  }, // leg1,2 up
  {   70,  45, 135, 110,  65, 135,  45, 115,  300  }, // leg1,2 dn; leg3,4 up
  {  115,  45, 135,  65, 110, 135,  45,  70,  300  }, // leg3,4 dn; leg1,2 up
  {   70,  45, 135, 110,  65, 135,  45, 115,  300  }, // leg1,2 dn; leg3,4 up
  {  115,  45, 135,  65, 110, 135,  45,  70,  300  }, // leg3,4 dn; leg1,2 up
  {   70,  45, 135, 110,  65, 135,  45, 115,  300  }, // leg1,2 dn; leg3,4 up
  {  115,  45, 135,  65, 110, 135,  45,  70,  300  }, // leg3,4 dn; leg1,2 up
  {   75,  45, 135, 105, 110, 135,  45,  70,  300  }, // leg1,2 dn
};

// Dancing 3
int servoPrg15step = 10;
int servoPrg15 [][numberOfACE] PROGMEM = {
  // P16, P05, P04, P00, P02, P14, P12, P13,  ms
  {   70,  45,  45, 110, 110, 135, 135,  70,  300  }, // leg1,2,3,4 bk
  {  110,  45,  45,  60,  70, 135, 135,  70,  300  }, // leg1,2,3 up
  {   70,  45,  45, 110, 110, 135, 135,  70,  300  }, // leg1,2,3 dn
  {  110,  45,  45, 110,  70, 135, 135, 120,  300  }, // leg1,3,4 up
  {   70,  45,  45, 110, 110, 135, 135,  70,  300  }, // leg1,3,4 dn
  {  110,  45,  45,  60,  70, 135, 135,  70,  300  }, // leg1,2,3 up
  {   70,  45,  45, 110, 110, 135, 135,  70,  300  }, // leg1,2,3 dn
  {  110,  45,  45, 110,  70, 135, 135, 120,  300  }, // leg1,3,4 up
  {   70,  45,  45, 110, 110, 135, 135,  70,  300  }, // leg1,3,4 dn
  {   70,  90,  90, 110, 110,  90,  90,  70,  300  }, // standby
};

// --------------------------------------------------------------------------------
//温湿度检测组

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    payload[length] = '\0';
    Serial.println((char *)payload);

}

void wifiInit() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);   // 连接 WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("WiFi not Connect");
    }
    Serial.println("Connected to AP");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(MQTT_SERVER, MQTT_PORT);   // 连接 WiFi 之后，连接 MQTT 服务器
    client.setCallback(callback);
}


void mqttCheckConnect()
{
    while (!client.connected())
    {
        Serial.println("Connecting to MQTT Server ...");
        if (client.connect(CLIENT_ID, MQTT_USRNAME, MQTT_PASSWD))

        {

            Serial.println("MQTT Connected!");

        }
        else
        {
            Serial.print("MQTT Connect err:");
            Serial.println(client.state());
            delay(5000);
        }
    }
}


void mqttIntervalPost()
{
    char param[32];
    char jsonBuf[128];
    sprintf(param, "{\"CurrentTemperature\":%f}",T_sum/count);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    boolean d = client.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    if(d){
      Serial.println("publish Temperature success"); 
    }else{
      Serial.println("publish Temperature fail"); 
    }

    sprintf(param, "{\"CurrentHumidity\":%f}",RH_sum/count);
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.println(jsonBuf);
    d = client.publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    if(d){
      Serial.println("publish Humidity success"); 
    }else{
      Serial.println("publish Humidity fail"); 
    }
}

void errorDecoder(SHTC3_Status_TypeDef message) // The errorDecoder function prints "SHTC3_Status_TypeDef" resultsin a human-friendly way
{
  switch(message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}



/// 主程序
void setup() {
  Serial.begin(115200);    // 初始化串口，并开启调试信息
  Serial.println();
  Serial.print("Setting soft-AP ... ");

    // 设置 STA+AP 模式
  WiFi.mode(WIFI_AP_STA);

    // 连接到现有的 WiFi 网络

    wifiInit();
    
    Wire.begin(3,1);           //初始化Wire（IIC）库
    unsigned char i = 0;
    errorDecoder(mySHTC3.begin());

    // 设置 AP 模式的 IP 地址、网关和子网掩码
  IPAddress softLocal(192,168,1,101);     //IP地址
  IPAddress softGateway(192,168,1,101);
  IPAddress softSubnet(255,255,255,0);

  WiFi.softAPConfig(softLocal, softGateway, softSubnet); 
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();            //Tells the server to begin listening for incoming connections.
  Serial.printf("Web server started, open %s in a web browser\n", myIP.toString().c_str());
  
  pinMode(Battarylow, OUTPUT);   
 
  // Servo Pin Set
  servo[0].attach(5);
  servo[0].write(70 + servoCal[0]);
  servo[1].attach(4);
  servo[1].write(90 + servoCal[1]);
  servo[2].attach(00);
  servo[2].write(90 + servoCal[2]);
  servo[3].attach(2);
  servo[3].write(110 + servoCal[3]);
  servo[4].attach(14);
  servo[4].write(110 + servoCal[4]);
  servo[5].attach(12);
  servo[5].write(90 + servoCal[5]);
  servo[6].attach(13);
  servo[6].write(90 + servoCal[6]);
  servo[7].attach(15);
  servo[7].write(70 + servoCal[7]);

}


void loop() {
/////////////温度检测并上报
  delay(1000);                  //延时1000毫秒
  SHTC3_Status_TypeDef result = mySHTC3.update();
  if(mySHTC3.lastStatus == SHTC3_Status_Nominal)   //判断SHTC3状态是否正常
  {
    RH = mySHTC3.toPercent();   //读取湿度数据                       
    T = mySHTC3.toDegC();       //读取温度数据
    RH_sum+=RH;
    T_sum+=T;
    count+=1;                    
  }else{
    Serial.print("Update failed, error: ");
    errorDecoder(mySHTC3.lastStatus); //输出错误原因
    Serial.println();
  }
  Serial.print("Humidity:");  //向串口打印 Humidity:
  Serial.print(RH);           //向串口打印湿度数据
  Serial.print("%");
  Serial.print("  Temperature:");
  Serial.print(T);            //向串口打印温度数据
  Serial.println("C"); 
  Serial.println("https://blog.zeruns.tech");
  if (millis() - lastMs >= 30000)
  {
    lastMs = millis();
    mqttCheckConnect(); 

    /* 上报 */
    mqttIntervalPost();
    count=0;
    RH_sum=0;
    T_sum=0;
  }
  client.loop();
//*********串口，调试用********
//  Serial.print("sensor = ");
//  Serial.print(sensorValue);
//  Serial.print("\r\n");

  WiFiClient client = server.available();  //Gets a client that is connected to the server and has data available for reading. 
  if (client)
  {
    Serial.println("\n[Client connected]");
    while (client.connected())      //Whether or not the client is connected.
    {
//*********读电池电量********
//      BattaryValue = analogRead(analogInPin);   // read the analog in value
//      if (BattaryValue<900)
//        digitalWrite(Battarylow, HIGH);
//      else
//        digitalWrite(Battarylow, LOW);
        
//************接收电脑串口数据发给手机*************
//      if(Serial.available()){
//        size_t len = Serial.available();  //获取串口读取的字节数（字符数
//        uint8_t sbuf[len];
//        Serial.readBytes(sbuf, len);    
//        client.write(sbuf, len);
//        delay(1);     
//        }
    
//************串口测试打印数字，用于调试*************      
//      testnumber++;
//      Serial.print("Orientation: ");
//      Serial.print(testnumber);
//      Serial.print("\r\n");
//      delay(1000);
          
//************读取客户端发来数据并打印*************
      if (client.available()) //Returns the number of bytes available for reading 
      {
//      String line = client.readStringUntil('\r');   //直到读到括号里的内容，或超时
//      Serial.println(line);
//      Serial.println(client.read());

//          ActonFlag=client.peek();
//          ActonFlag=client.read();
//          Serial.println(ActonFlag);
//          Serial.print("\r\n");
      }
//*********判断执行动作********
        while(client.peek()!=-1)   //循环后，指保留并执行app发过来的最后一个动作
        {
          ActonFlag=client.read();
//          Serial.print(ActonFlag);
          if(ActonFlag=='s'||ActonFlag=='t')break;
          if(ActonFlag=='u'||ActonFlag=='v')break;
          if(ActonFlag=='w'||ActonFlag=='x')break;
          if(ActonFlag=='y'||ActonFlag=='z')break;
        }        
      
      
      switch (ActonFlag) {
          case 'a':    
            runServoPrg(servoPrg06, servoPrg06step); // turnLeft
            break;
          case 'b':    
            runServoPrg(servoPrg02, servoPrg02step); // forward
            break;
          case 'c':    
            runServoPrg(servoPrg07, servoPrg07step); // turnRight
            break;
          case 'd':    
            runServoPrg(servoPrg04, servoPrg04step); // moveLeft
            break;
          case 'e':    
            runServoPrg(servoPrg03, servoPrg03step); // backward
            break;
          case 'f':    
            runServoPrg(servoPrg05, servoPrg05step); // moveRight
            break;
          case 'g':    
            runServoPrg(servoPrg01, servoPrg01step); // standby
            break;
          case 'h':    
            runServoPrg(servoPrg09, servoPrg09step); // sayHi
            break;
          case 'i':    
            runServoPrg(servoPrg11, servoPrg11step); // pushUp
            break;
          case 'j':    
            runServoPrg(servoPrg08, servoPrg08step); // lie
            break;
          case 'k':    
            runServoPrg(servoPrg10, servoPrg10step); // fighting
            break;
          case 'l':    
            runServoPrg(servoPrg12, servoPrg12step); // sleep
            break;
          case 'm':    
            runServoPrg(servoPrg13, servoPrg13step); // dancing1
            break;
          case 'n':    
            runServoPrg(servoPrg14, servoPrg14step); // dancing2
            break;
          case 'o':    
            runServoPrg(servoPrg15, servoPrg15step); // dancing3
            break;  
          case 'p':    
            runServoPrg(servoPrg00, servoPrg00step); // zero position
            break;
          case 's':    
            servo[0].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 't':    
            servo[1].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;          
          case 'u':    
            servo[2].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 'v':    
            servo[3].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 'w':    
            servo[4].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 'x':    
            servo[5].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 'y':    
            servo[6].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;
          case 'z':    
            servo[7].write((client.read()-48)*100+(client.read()-48)*10+client.read()-48);ActonFlag=0; break;
            break;          
        }
    }
  delay(1);
//************客户端disonnected后退出while循环*************  
  client.stop();
//  Serial.println("[Client disonnected]");
  }
}

//*********运行动作函数********
void runServoPrg(int servoPrg[][numberOfACE], int step)
{
  for (int i = 0; i < step; i++) { // Loop for step

    int totalTime = servoPrg[i][numberOfACE - 1]; // Total time of this step

    // Get servo start position
    for (int s = 0; s < numberOfServos; s++) {
      servoPos[s] = servo[s].read() - servoCal[s];
    }

    for (int j = 0; j < totalTime / servoPrgPeriod; j++) { // Loop for time section
      for (int k = 0; k < numberOfServos; k++) { // Loop for servo
        servo[k].write((map(j, 0, totalTime / servoPrgPeriod, servoPos[k], servoPrg[i][k])) + servoCal[k]);
      }
      delay(servoPrgPeriod);
    }
  }
}
