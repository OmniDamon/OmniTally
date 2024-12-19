
//建议WiFi设置空白，这样就可以使用AirKiss配网(AirKiss配网，手机连接2.4G网络，关注公众号“易艾思”，菜单点击更多->嵌入设备配网，靠近设备进行配网)
const char *WiFi_ssid = "";  //WiFi名称Damon Ivy Guest 2.4G
const char *WiFi_key = "";   //WiFi密码1357924680


//以下根据需要设置
uint8_t Tally_ID = 1;            //TallyID台历灯号
uint8_t Tally_Style = 1;         //Tally显示风格  //1=全屏,2=小方块,3=空方块,3=实心圆,5=空心圆,6=爱心,7=数字亮字,8=数字底亮
uint8_t Tally_Brightnes = 64;    //Tally灯亮灯亮度
uint8_t Tally_Darkness = 32;     //Tally灯灭灯亮度
uint8_t Tally_Background = 4;    //Tally灯背景亮度
uint8_t Switcher_Type = 1;       //切换台品牌 //1=OSEE,2=vMix
String Switcher_IP = "0.0.0.0";  //切换台IP
int Web_Port = 80;
int Socket_Port = 81;


//型号及版本
String SYSModel = "OmniTally";   //型号
String SYSVer = "1.0.24121810";  //版本
uint8_t SYSDebug = 1;            //调试

//以下根据硬件设置
uint8_t Pin_LED = 2;     //LED指示灯针脚GPIO2 D4
uint8_t Pin_RGB = 4;     //RGB灯组针脚GPIO4 D2
uint8_t Pin_KEY = 0;     //RGB灯组针脚GPIO0 D3
uint8_t Light_Type = 0;  //灯组类型

//5*5矩阵
uint8_t RGB_Pixel = 25;            //RGB灯组灯珠数量
uint8_t RGB_Matrix_PixelX = 5;     //RGB灯组矩阵水平灯珠数量
uint8_t RGB_Matrix_PixelY = 5;     //RGB灯组矩阵垂直灯珠数量
uint8_t RGB_Matrix_OffsetX = 0;    //RGB灯组矩阵水平偏移量
uint8_t RGB_Matrix_OffsetY = 0;    //RGB灯组矩阵水平偏移量
uint8_t RGB_Matrix_Direction = 2;  //RGB灯组矩阵排列方向 //1=水平左右,2=反Z字排列
/*
//8*8矩阵
uint8_t RGB_Pixel=64; //RGB灯组灯珠数量
uint8_t RGB_Matrix_PixelX=8;  //RGB灯组矩阵水平灯珠数量
uint8_t RGB_Matrix_PixelY=8;  //RGB灯组矩阵垂直灯珠数量
uint8_t RGB_Matrix_OffsetX=2;  //RGB灯组矩阵水平偏移量
uint8_t RGB_Matrix_OffsetY=2;  //RGB灯组矩阵水平偏移量
uint8_t RGB_Matrix_Direction=1;  //RGB灯组矩阵排列方向 //1=水平左右,2=反Z字排列
*/


//以下为内部变量不需要修改
uint8_t SYSChk = 0;               //随机数
String Hardware = ARDUINO_BOARD;  //硬件类型
String WiFiMAC = "";              //设备MAC
String WiFiIP = "";               //设备IP
uint8_t WiFiCount = 0;            //WiFi连接计数器
uint8_t WiFiAirKiss = 0;          //WiFiAirKiss计数器
uint8_t LoopCount = 0;            //Loop计数器
uint8_t TickerCount = 0;          //Tick计数器
uint8_t FlickerCount = 0;         //Flicker计数器
uint8_t ConnectCountdown = 0;     //Connect计数器
uint8_t UpdateCount = 0;          //Update计数器
uint8_t WebSocketStatus = 0;      //Socket状态
uint8_t LastTcping = 0;           //上次Tcping
uint8_t SwitcherPVW = 0;          //当前PVW
uint8_t SwitcherPGM = 0;          //当前PGM
uint8_t SwitcherPVWlast = -1;     //上次PVW
uint8_t SwitcherPGMlast = -1;     //上次PGM



/*
0 Config
1 Pin_LED
2 Pin_RGB
3 Pin_KEY
4 Light_Type
5 RGB_Pixel
6 RGB_Matrix_PixelX
7 RGB_Matrix_PixelY
8 RGB_Matrix_OffsetX
9 RGB_Matrix_OffsetY
10 RGB_Matrix_Direction
11 Tally_ID
12 Tally_Style
13 Tally_Brightnes
14 Tally_Darkness
15 Tally_Background
16 Switcher_Type
17 Switcher_IP1
18 Switcher_IP2
19 Switcher_IP3
20 Switcher_IP4
*/


#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

#include <ESP8266WebServer.h>
ESP8266WebServer server(Web_Port);

#include <WebSocketsServer.h>
WebSocketsServer WebSocket = WebSocketsServer(Socket_Port);


#include "OneButton.h"
OneButton Abutton(Pin_KEY);

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(RGB_Pixel, Pin_RGB, NEO_GRB + NEO_KHZ800);


#include <ArduinoJson.h>
StaticJsonDocument<256> doc;
StaticJsonDocument<256> json;
StaticJsonDocument<256> jsons;


#include <WiFiClient.h>
WiFiClient client;
WiFiClient tcping;


#include <Ticker.h>
Ticker ticker;


const byte digit5x5[10][8] = {
  { 0x70, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00, 0x00 },  // 0
  { 0x20, 0x60, 0x20, 0x20, 0x70, 0x00, 0x00, 0x00 },  // 1
  { 0x70, 0x08, 0x30, 0x40, 0x78, 0x00, 0x00, 0x00 },  // 2
  { 0x70, 0x08, 0x30, 0x08, 0x70, 0x00, 0x00, 0x00 },  // 3
  { 0x88, 0x88, 0x78, 0x08, 0x08, 0x00, 0x00, 0x00 },  // 4
  { 0x78, 0x40, 0x70, 0x08, 0x70, 0x00, 0x00, 0x00 },  // 5
  { 0x30, 0x40, 0x70, 0x48, 0x30, 0x00, 0x00, 0x00 },  // 6
  { 0x78, 0x08, 0x10, 0x20, 0x20, 0x00, 0x00, 0x00 },  // 7
  { 0x70, 0x88, 0x70, 0x88, 0x70, 0x00, 0x00, 0x00 },  // 8
  { 0x70, 0x88, 0x78, 0x08, 0x30, 0x00, 0x00, 0x00 }   // 9
};

const byte matrix[5][8] = {
  { 0x00, 0x70, 0x70, 0x70, 0x00, 0x00, 0x00, 0x00 },  // 方块
  { 0xf8, 0x88, 0x88, 0x88, 0xf8, 0x00, 0x00, 0x00 },  // 空方块
  { 0x70, 0xF8, 0xF8, 0xF8, 0x70, 0x00, 0x00, 0x00 },  // 实心圆
  { 0x70, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00, 0x00 },  // 空心圆
  { 0x50, 0xF8, 0xF8, 0x70, 0x20, 0x00, 0x00, 0x00 }   // 爱心
};

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println(SYSModel + " Ver:" + SYSVer);
  SYSChk = random(1, 254);

  EEPROM.begin(256);
  Config_Load();

  pinMode(Pin_LED, OUTPUT);
  pinMode(Pin_KEY, INPUT_PULLUP);

  Abutton = OneButton(Pin_KEY);

  Abutton.reset();
  Abutton.attachClick(AClick);
  Abutton.attachDoubleClick(AdoubleClick);
  Abutton.attachLongPressStart(ALongPressStart, &Abutton);
  Abutton.attachDuringLongPress(ADuringLongPress, &Abutton);
  Abutton.attachLongPressStop(ALongPressStop, &Abutton);

  ticker.attach(1, tickerFun);

  if (RGB_Pixel > 0) {
    strip.begin();
    strip.setPin(Pin_RGB);
    strip.updateLength(RGB_Pixel);
    strip.clear();
    strip.fill(strip.Color(16, 16, 16), 0, RGB_Pixel);
    strip.setBrightness(255);
    strip.show();
  }

  uint8_t Config = EEPROM.read(0);
  if (Config != 1) {
    WiFi_SmartConfig();
  }
  WiFi_Connect();

  Web_Server();

  client.setTimeout(3000);
  tcping.setTimeout(3000);

  WebSocket.begin();

  WebSocket.onEvent(WebSocket_Event);
}


void loop() {
  LoopCount++;
  switch (Switcher_Type) {
    case 1:
      Switcher_OSEE();

      break;

    case 2:
      Switcher_vMix();

      break;
  }

  Tally_Light(0);


  if (FlickerCount > 0) {
    FlickerCount--;
    if (FlickerCount % 80 == 0) {
      digitalWrite(Pin_LED, !digitalRead(Pin_LED));
      delay(30);
    }
  }

  Abutton.tick();
  MDNS.update();
  WebSocket.loop();
  server.handleClient();
}


bool tickerFun() {
  //Serial.println(String(SwitcherPGM) + " - " + String(SwitcherPVW) + " - " + WiFiMAC + " - " + WiFiIP);

  TickerCount++;

  if (ConnectCountdown > 0) { ConnectCountdown--; }


  if (TickerCount % 5 == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiIP = WiFi.localIP().toString();
    } else {
      WiFiIP = "0.0.0.0";
    }

    WebSocket_Send();
  }


  if (UpdateCount > 0) {
    UpdateCount++;
    if (UpdateCount > 60) {
      ESP.restart();
    }
  }

  digitalWrite(Pin_LED, !digitalRead(Pin_LED));

  return true;
}


String OSEE_data;
String OSEE_BlockJson[32];
uint8_t OSEE_BlockCount = 0;
void Switcher_OSEE() {
  if (client.connected()) {
    if ((TickerCount % 5 == 0) && (LastTcping != TickerCount)) {
      debug("Switcher_OSEE TCPing");
      LastTcping = TickerCount;
      if (!TCPing(Switcher_IP, 19010)) {
        client.stop();
      }
    }
    if (client.available()) {
      debug("Switcher_OSEE Receive");
      do {
        char response = client.read();
        OSEE_data = OSEE_data + response;
      } while (client.available());
      Switcher_OSEE_PreJson(OSEE_data);
      if (OSEE_BlockCount > 0) {
        //DynamicJsonDocument json(512);
        for (uint8_t i = 1; i <= OSEE_BlockCount; i++) {
          debug("Switcher_OSEE JSON:" + OSEE_BlockJson[i]);
          //Serial.println(OSEE_BlockJson[i]);
          deserializeJson(json, OSEE_BlockJson[i]);
          if (json["id"] == "pgmIndex") {
            SwitcherPGM = uint8_t(json["value"][0]) + 1;
          }
          if (json["id"] == "pvwIndex") {
            SwitcherPVW = uint8_t(json["value"][0]) + 1;
          }
        }
        LED_Flicker();
        OSEE_BlockCount = 0;
      }
    }

  } else {
    if (Switcher_IP != "0.0.0.0") {
      if (ConnectCountdown == 0) {
        ConnectCountdown = 5;
        debug("Switcher_OSEE Connecting");
        if (!client.connect(Switcher_IP, 19010)) {
          debug("Switcher_OSEE Connection failed");
        }
      }
    }
  }
}

void Switcher_OSEE_PreJson(String &input) {
  int start = 0;
  int end = 0;
  String output;
  for (int i = 0; i < input.length(); i++) {
    char c = input[i];
    if (isPrintable(c) || c == '{' || c == '}' || c == '[' || c == ']') {
      output += c;
    }
  }
  while ((start = output.indexOf('{', start)) >= 0) {
    end = output.indexOf('}', start) + 1;
    if (end > start) {
      OSEE_BlockJson[++OSEE_BlockCount] = output.substring(start, end);
      start = end;
    } else {
      break;
    }
  }
  if (start > 0 && start < output.length()) {
    input = output.substring(start);
  } else {
    input = "";
  }
}



void Switcher_vMix() {
  delay(10);
  if (LoopCount % 10 == 0) {
    

    if ((Switcher_IP != "0.0.0.0") && (WiFi.status() == WL_CONNECTED)) {
      HTTPClient http;
      debug("http://" + Switcher_IP + ":8088/api/");
      http.setTimeout(3000);
      http.begin(client, "http://" + Switcher_IP + ":8088/api/");
      http.addHeader("Content-Type", "text/xml");

      int httpCode = http.GET();
      if (httpCode > 0) {
        LoopCount = 0;
        String payload = http.getString();

        String previewValue = extractTagValue(payload, "<preview>", "</preview>");
        String activeValue = extractTagValue(payload, "<active>", "</active>");

        if (previewValue != "" && activeValue != "") {
          SwitcherPVW = previewValue.toInt();
          SwitcherPGM = activeValue.toInt();
        }
      }
      http.end();
    }
  }
}


void WebSocket_Event(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      WebSocketStatus = 0;
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = WebSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        //WebSocket.sendTXT(num, "Hell OmniTally !");
        WebSocketStatus = 1;
      }
      break;
    case WStype_TEXT:
      //Serial.printf("[%u] Text message: %s\n", num, payload);
      break;
    case WStype_PONG:
      Serial.printf("[%u] PONG received!\n", num);
      break;
    case WStype_ERROR:
      Serial.println("WebSocket error!");
      break;
  }
}

void WebSocket_Send() {
  if (WebSocketStatus == 1) {
    WebSocketStatus = 2;
    String data = String(SwitcherPVW) + String(SwitcherPGM);
    debug("WebSocket_Send:" + data);
    //debug(WebSocket.connectedClients());
    if (!WebSocket.broadcastTXT(data)) {
      debug("WebSocket Reconnect");
      WebSocket.disconnect();
      WebSocket.begin();
    }
    WebSocketStatus = 1;
  }
}


void handleRoot() {
  LED_Flicker();
  server.send(200, "text/html", "<html><head><title>OmniTally™</title><meta http-equiv='Content-Type' content='text/html; charset=utf-8' /></head><b>Hello OmniTally !</b><br><br>TallyID: " + String(Tally_ID) + "<br><br>\n<small>作者: 无所不能的大孟(OmniDamon)<br>\nMAC: " + WiFiMAC + "<br>\nIP: " + WiFiIP + "<br>\nVER: " + SYSVer + "</small><br>\n<script src='http://omni.aidns.net/OmniTally/appcall.js'></script>\n</html>");
}


void handleApi() {
  LED_Flicker();
  //DynamicJsonDocument doc(512);
  JsonObject json = doc.to<JsonObject>();

  json["Tally_ID"] = Tally_ID;
  json["WiFi_MAC"] = WiFiMAC;
  json["WiFi_IP"] = WiFiIP;
  json["Ver"] = SYSVer;
  json["PVW"] = SwitcherPVW;
  json["PGM"] = SwitcherPGM;
  String jsonString;
  serializeJson(doc, jsonString);

  debug("API_Send:" + jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", jsonString);
}


void handleSetup() {
  LED_Flicker();
  //替换" \"
  String html = "<!DOCTYPE html><html><head><title>OmniTally™ Setup</title><meta http-equiv='Content-Type' content='text/html; charset=utf-8' /><meta http-equiv='Pragma' content='no-cache' /><meta http-equiv='Cache-Control' content='no-cache' /><meta http-equiv='Expires' content='0' /><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=2, user-scalable=1'><style>table, th, td {border: 1px solid #999999;border-collapse: collapse;}input[type='number'] {text-align: right;} </style></head><body style='color:#333333;background:#f0f0f0;'><left><b>OmniTally™ Setup</b><br><table><input id='i0' type='hidden'><tr><td colspan='2'><b>基本设置</b>&ensp;&ensp;<small><input type='checkbox' id='autosave' value='1'>实时更新显示</small></td></tr><tr><td>TallyID</td><td><select id='i11' size='1' oninput='change()'><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option><option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option><option value='8'>8</option></select></td></tr><tr><td>Tally显示风格</td><td><select id='i12' size='1' oninput='change()'><option value='1'>全亮</option><option value='2'>小方块</option><option value='3'>空方块</option><option value='4'>实心圆</option><option value='5'>空心圆</option><option value='6'>爱心</option><option value='7'>数字正显</option><option value='8'>数字反显</option></select></td></tr><tr><td>激活时亮度</td><td><input id='i13' type='range' min='0' max='255' step='1' style='width:255px;' onmouseup='change()' onchange='change()'></td></tr><tr><td>未激活亮度</td><td><input id='i14' type='range' min='0' max='255' step='1' style='width:255px;' onmouseup='change()' onchange='change()'></td></tr><tr><td>背景亮度</td><td><input id='i15' type='range' min='0' max='255' step='1' style='width:255px;' onmouseup='change()' onchange='change()'></td></tr><tr><td>切换台品牌</td><td><select id='i16' size='1'><option value='0'>(未选择)</option><option value='1'>OSEE 时代奥视</option><option value='2'>vMix 软导播</option><option value='3' disabled>BMD ATEM</option></select></td></tr><tr><td>切换台IP</td><td><input id='i17' type='number' min='0' max='255' step='1'>.<input id='i18' type='number' min='0' max='255' step='1'>.<input id='i19' type='number' min='0' max='255' step='1'>.<input id='i20' type='number' min='0' max='255' step='1'></td></tr><tr><td colspan='2'><b>硬件设置</b>&ensp;&ensp;<small>(建议修改后重启)</small></td></tr><tr><td>LED针脚</td><td><select id='i1' size='1'><option value='0'>GPIO0 D3</option><option value='2'>GPIO2 D4</option><option value='4'>GPIO4 D2</option><option value='5'>GPIO5 D1</option></select> (默认GPIO2 D4) </td></tr><tr><td>RGB针脚</td><td><select id='i2' size='1'><option value='0'>GPIO0 D3</option><option value='2'>GPIO2 D4</option><option value='4'>GPIO4 D2</option><option value='5'>GPIO5 D1</option></select> (默认GPIO4 D2) </td></tr><tr><td>按键针脚</td><td><select id='i3' size='1'><option value='0'>GPIO0 D3</option><option value='2'>GPIO2 D4</option><option value='4'>GPIO4 D2</option><option value='5'>GPIO5 D1</option></select> (默认GPIO0 D3) </td></tr><tr><td>灯珠类型</td><td><select id='i4' size='1'><option value='0'>默认</option></select></td></tr><tr><td>像素灯珠数量</td><td><input id='i5' type='number' min='0' max='255' step='1'></td></tr><tr><td>矩阵水平灯珠数量</td><td><input id='i6' type='number' min='0' max='255' step='1'></td></tr><tr><td>矩阵垂直灯珠数量</td><td><input id='i7' type='number' min='0' max='255' step='1'></td></tr><tr><td>矩阵水平偏移量</td><td><input id='i8' type='number' min='0' max='255' step='1'>(>5像素可调整)</td></tr><tr><td>矩阵垂直偏移量</td><td><input id='i9' type='number' min='0' max='255' step='1'>(>5像素可调整)</td></tr><tr><td>矩阵灯珠排列</td><td><select id='i10' size='1'><option value='1'>每行自左往右(8*8)</option><option value='2'>反Z字(5*5)</option></select></td></tr><tr><td>固件更新URL</td><td><input id='url' type='text' size='32'><button onclick='SyncData(\"upgrade\")'>升级</button></td></tr><tr><td align='center'><button onclick='SyncData(\"default\")'>重置</button>&nbsp;&nbsp;<button onclick='SyncData(\"restart\")'>重启</button></td><td><button onclick='SyncData()'>读取</button>&ensp;<button onclick='SyncData(\"save\")'>保存</button></td></tr></table><br><small>作者: 无所不能的大孟(OmniDamon)<br>MAC: {WiFi_MAC}<br>IP: {WiFi_IP}<br>VER: {SYS_VER}</small><br><script src='http://omni.aidns.net/OmniTally/appcall.js'></script><br><br><script>function change(){if (document.getElementById('autosave').checked){SyncData('save');}}function AjaxRequest(data) { var xhr = new XMLHttpRequest(); xhr.open('POST', '/config', true); xhr.setRequestHeader('Content-Type', 'application/json'); xhr.send(data); xhr.onreadystatechange = function () { if (xhr.readyState === 4) { if (xhr.status === 200) { var j=JSON.parse(xhr.responseText);for (var i in j) { if (j.hasOwnProperty(i)) { if (o=document.getElementById('i'+i)){ o.value=j[i]; } }}if (j['error']){alert(j['error']);} } } };}function SyncData(cmd) {var o;var d={};d['i0']=document.getElementById('i0').value;if (!cmd){AjaxRequest();return;}else if (cmd=='default'){if (!confirm('是否确认重置为默认值？')){return false;}}else if (cmd=='upgrade'){if (!confirm('是否确认更新固件？')){return false;}else{d['url']=document.getElementById('url').value;}}else if (cmd){for (var i=1;i<=20;i++){if (o=document.getElementById('i'+i)){d['i'+i]=o.value;}}}d['cmd']=cmd;d=JSON.stringify(d);AjaxRequest(d);}AjaxRequest(); </script></body></html>";
  html = replaceSubstring(html, "{WiFi_MAC}", WiFiMAC);
  html = replaceSubstring(html, "{WiFi_IP}", WiFiIP);
  html = replaceSubstring(html, "{SYS_VER}", SYSVer);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "text/html", html);
}


void handleConfig() {
  LED_Flicker();
  //DynamicJsonDocument doc(512);
  JsonObject json = doc.to<JsonObject>();

  if (server.hasArg("plain")) {
    //DynamicJsonDocument jsons(512);
    String body = server.arg("plain");
    Serial.println("Config_Receive");
    //Serial.println(body);
    deserializeJson(jsons, body);
    if (uint8_t(jsons["i0"]) != SYSChk) {
      json["error"] = "校验失败!";
    } else if (jsons["cmd"] == "save") {
      if (jsons["i1"]) { Pin_LED = String(jsons["i1"]).toInt(); }
      if (jsons["i2"]) { Pin_RGB = String(jsons["i2"]).toInt(); }
      if (jsons["i3"]) { Pin_KEY = String(jsons["i3"]).toInt(); }
      if (jsons["i4"]) { Light_Type = String(jsons["i4"]).toInt(); }
      if (jsons["i5"]) { RGB_Pixel = uint8_t(jsons["i5"]); }
      if (jsons["i6"]) { RGB_Matrix_PixelX = String(jsons["i6"]).toInt(); }
      if (jsons["i7"]) { RGB_Matrix_PixelY = String(jsons["i7"]).toInt(); }
      if (jsons["i8"]) { RGB_Matrix_OffsetX = String(jsons["i8"]).toInt(); }
      if (jsons["i9"]) { RGB_Matrix_OffsetY = String(jsons["i9"]).toInt(); }
      if (jsons["i10"]) { RGB_Matrix_Direction = String(jsons["i10"]).toInt(); }
      if (jsons["i11"]) { Tally_ID = String(jsons["i11"]).toInt(); }
      if (jsons["i12"]) { Tally_Style = String(jsons["i12"]).toInt(); }
      if (jsons["i13"]) { Tally_Brightnes = String(jsons["i13"]).toInt(); }
      if (jsons["i14"]) { Tally_Darkness = String(jsons["i14"]).toInt(); }
      if (jsons["i15"]) { Tally_Background = String(jsons["i15"]).toInt(); }
      if (jsons["i16"]) { Switcher_Type = String(jsons["i16"]).toInt(); }
      uint8_t Switcher_IP1;
      uint8_t Switcher_IP2;
      uint8_t Switcher_IP3;
      uint8_t Switcher_IP4;
      if (jsons["i17"]) { Switcher_IP1 = uint8_t(jsons["i17"]); }
      if (jsons["i18"]) { Switcher_IP2 = uint8_t(jsons["i18"]); }
      if (jsons["i19"]) { Switcher_IP3 = uint8_t(jsons["i19"]); }
      if (jsons["i20"]) { Switcher_IP4 = uint8_t(jsons["i20"]); }
      if ((jsons["i17"]) && (jsons["i18"]) && (jsons["i19"]) && (jsons["i20"])) {
        Switcher_IP = String(Switcher_IP1) + "." + String(Switcher_IP2) + "." + String(Switcher_IP3) + "." + String(Switcher_IP4);
      }
      Config_Save(0);
    } else if (jsons["cmd"] == "default") {
      Config_Save(1);
    } else if (jsons["cmd"] == "restart") {
      ESP.restart();
    } else if ((jsons["cmd"] == "upgrade") && (jsons["url"])) {
      Update_Firmware(jsons["url"]);
    }

    Tally_Light(1);
  }

  json["1"] = Pin_LED;
  json["2"] = Pin_RGB;
  json["3"] = Pin_KEY;
  json["4"] = Light_Type;
  json["5"] = RGB_Pixel;
  json["6"] = RGB_Matrix_PixelX;
  json["7"] = RGB_Matrix_PixelY;
  json["8"] = RGB_Matrix_OffsetX;
  json["9"] = RGB_Matrix_OffsetY;
  json["10"] = RGB_Matrix_Direction;
  json["11"] = Tally_ID;
  json["12"] = Tally_Style;
  json["13"] = Tally_Brightnes;
  json["14"] = Tally_Darkness;
  json["15"] = Tally_Background;
  json["16"] = Switcher_Type;
  uint8_t IPint[4];
  IP2int(Switcher_IP, IPint);
  json["17"] = IPint[0];
  json["18"] = IPint[1];
  json["19"] = IPint[2];
  json["20"] = IPint[3];
  json["0"] = SYSChk;
  String jsonString;
  serializeJson(doc, jsonString);

  Serial.println("Config_Send");
  //Serial.println(jsonString);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", jsonString);
}


void handleNA() {
  SwitcherPVW = 0;
  SwitcherPGM = 0;
  Tally_Light(1);
  server.send(200, "text/plain", "N/A " + String(Tally_ID) + " OK");
}

void handlePVW() {
  SwitcherPVW = Tally_ID;
  SwitcherPGM = 0;
  Tally_Light(1);
  server.send(200, "text/plain", "PVW " + String(Tally_ID) + " OK");
}

void handlePGM() {
  SwitcherPVW = 0;
  SwitcherPGM = Tally_ID;
  Tally_Light(1);
  server.send(200, "text/plain", "PGM " + String(Tally_ID) + " OK");
}

void handleWildcard() {
  String path = server.uri();
  if (path.startsWith("/id/")) {
    String idStr = path.substring(4);
    uint8_t Id = idStr.toInt();
    if (Id > 0) {
      Tally_ID = Id;
      Tally_Light(1);
      Config_Save(0);
      server.send(200, "text/plain", "Tally ID " + String(Tally_ID));
      return;
    }

  } else if (path.startsWith("/tally/")) {
    String idStr = path.substring(7);
    uint8_t Id = idStr.toInt();
    if (Id > 0) {
      String html = "<!DOCTYPE html><html><head><title>OmniTally™ WebTally</title><meta http-equiv='Content-Type' content='text/html; charset=utf-8' /><meta http-equiv='Pragma' content='no-cache' /><meta http-equiv='Cache-Control' content='no-cache' /><meta http-equiv='Expires' content='0' /><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>html, body {height: 100%;margin: 0;padding: 0;overflow: hidden;}#display {font-size: 50vw;line-height: 50vh;text-align: center;display: flex;justify-content: center;align-items: center;height: 100vh;width: 100vw;font-weight:bold;}</style></head><body style='background:#666666;'><left><div id='display'>-</div><div style='position:fixed;bottom:0px;width:100%;font-size:11px;text-align:center;'>OmniTally™ {SYS_VER} &ensp; 作者: 无所不能的大孟(OmniDamon)</div><script>var Tally_ID='1';var Tally_Server='ws://192.168.2.152:81';document.getElementById('display').innerText=Tally_ID;function connectWebSocket() { socket = new WebSocket(Tally_Server); socket.onopen = function(event) { console.log(\"Connected to WebSocket server.\"); }; socket.onmessage = function(event) { console.log(\"Message from server:\", event.data); var PVW= event.data.charAt(0); var PGM = event.data.charAt(1); if (Tally_ID==PGM ){ document.body.style.backgroundColor = '#ff0000'; }else if (Tally_ID==PVW){ document.body.style.backgroundColor = '#00ff00'; }else{ document.body.style.backgroundColor = '#999999'; } }; socket.onclose = function(event) { console.log(\"Disconnected from WebSocket server.\"); reconnectWebSocket(); }; socket.onerror = function(error) { console.error(\"WebSocket error:\", error); };}function reconnectWebSocket() { setTimeout(function() { console.log(\"Attempting to reconnect...\"); connectWebSocket(); }, 1000);}function refresh() { if (socket.readyState === WebSocket.OPEN) { console.log(\"Sending message:\", Tally_ID); socket.send(Tally_ID); } else { console.log(\"WebSocket is not open. Cannot send message.\"); } setTimeout(refresh, 3000);}connectWebSocket();refresh();</script></body></html>";
      html = replaceSubstring(html, "Tally_ID='1'", "Tally_ID='" + String(Id) + "'");
      html = replaceSubstring(html, "Tally_Server='ws://192.168.2.152:81'", "Tally_Server='ws://" + String(WiFiIP) + ":" + String(Socket_Port) + "'");
      html = replaceSubstring(html, "{SYS_VER}", SYSVer);
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(200, "text/html", html);
    }

  } else {
    server.send(404, "text/plain", "Page Not found !");
  }
}


void Web_Server() {
  debug("WebServer");
  server.on("/", handleRoot);
  server.on("/na", handleNA);
  server.on("/pvw", handlePVW);
  server.on("/pgm", handlePGM);
  server.on("/api", handleApi);
  server.on("/setup", handleSetup);
  server.on("/config", handleConfig);
  server.onNotFound(handleWildcard);
  server.begin();
}


bool WiFi_Connect() {
  Serial.println("WiFi_Connect");
  digitalWrite(Pin_LED, LOW);
  WiFi.mode(WIFI_OFF);
  WiFi.setAutoConnect(true);
  if (WiFi_ssid) {
    Serial.println("WiFi_Connect:" + String(WiFi_ssid));
    WiFi.begin(WiFi_ssid, WiFi_key);
  }
  WiFi.begin();

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  WiFiMAC = macStr;

  uint8_t Config = EEPROM.read(0);
  if (Config == 1) {
    WiFi.hostname(SYSModel + "-" + WiFiMAC);
    MDNS.begin(SYSModel + "-" + WiFiMAC);
  }else{
    WiFi.hostname(SYSModel);
    MDNS.begin(SYSModel);
  }

  while (WiFi.status() != WL_CONNECTED) {
    if (WiFiCount % 5 == 0) { Serial.print("."); }
    LED_Flash(100, 100);
    if (WiFiCount++ > 5 * 15) { break; }
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(Pin_LED, LOW);
    WiFiIP = WiFi.localIP().toString();
    Serial.println("WiFi_Connect:OK");

  } else {
    digitalWrite(Pin_LED, HIGH);
    WiFiIP = "0.0.0.0";
    Serial.println("WiFi_Connect:NK");
    WiFi_SmartConfig();
  }

  Serial.println("WiFiMAC:" + WiFiMAC);
  Serial.println("WiFiIP:" + WiFiIP);

  return true;
}


void WiFi_SmartConfig() {
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi_SmartConfig");
  WiFi.beginSmartConfig();
  WiFiAirKiss = 0;
  while (1) {
    LED_Flash(400, 100);
    Serial.println("WiFi_SmartConfig...");
    if (WiFi.smartConfigDone()) {
      Serial.println("WiFi_SmartConfig:OK");
      Serial.println("SSID:" + String(WiFi.SSID().c_str()));
      Serial.println("KEY:" + String(WiFi.psk().c_str()));
      break;
    }
    WiFiAirKiss++;
    if (WiFiAirKiss > 2 * 180) {
      Serial.println("WiFi_SmartConfig:NK");
      LED_Error();
      delay(1000);
      ESP.restart();
      break;
    }
  }
}



void Tally_Light(uint8_t force) {
  if ((!force) && ((SwitcherPVWlast == SwitcherPVW) && (SwitcherPGMlast == SwitcherPGM))) { return; }
  debug("Tally_Light");
  WebSocket_Send();
  LED_Flicker();
  SwitcherPVWlast = SwitcherPVW;
  SwitcherPGMlast = SwitcherPGM;
  uint8_t yml = Tally_ID;
  uint32_t color;
  switch (Tally_Style) {
    case 1:
      if (Tally_ID == SwitcherPGM) {
        strip.fill(strip.Color(Tally_Brightnes, 0, 0), 0, RGB_Pixel);
        strip.show();
      } else if (Tally_ID == SwitcherPVW) {
        strip.fill(strip.Color(0, Tally_Brightnes, 0), 0, RGB_Pixel);
        strip.show();
      } else {
        strip.fill(strip.Color(Tally_Darkness, Tally_Darkness, Tally_Darkness), 0, RGB_Pixel);
        strip.show();
      }
      break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      yml = Tally_Style - 2;
      if (Tally_ID == SwitcherPGM) {
        color = strip.Color(Tally_Brightnes, 0, 0);
      } else if (Tally_ID == SwitcherPVW) {
        color = strip.Color(0, Tally_Brightnes, 0);
      } else {
        color = strip.Color(Tally_Darkness, Tally_Darkness, Tally_Darkness);
      }
      for (uint8_t y = 1; y <= RGB_Matrix_PixelY; y++) {
        byte bit = matrix[yml][y - 1 - RGB_Matrix_OffsetY];
        for (uint8_t x = 1; x <= RGB_Matrix_PixelX; x++) {
          //Serial.print(bitRead(bit, 8-x) ? '*' : ' ');
          //Serial.print("("+String(y-1-RGB_Matrix_OffsetY)+","+String(8-x+RGB_Matrix_OffsetX)+")");
          if ((8 - x + RGB_Matrix_OffsetX >= 0) && (y - 1 - RGB_Matrix_OffsetY >= 0) && (bitRead(bit, 8 - x + RGB_Matrix_OffsetX))) {
            strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, color);
          } else {
            strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, strip.Color(0, Tally_Background, Tally_Background));
          }
        }
        strip.show();
      }

      break;


    case 7:
      if (Tally_ID == SwitcherPGM) {
        color = strip.Color(Tally_Brightnes, 0, 0);
      } else if (Tally_ID == SwitcherPVW) {
        color = strip.Color(0, Tally_Brightnes, 0);
      } else {
        color = strip.Color(Tally_Darkness, Tally_Darkness, Tally_Darkness);
      }
      for (int y = 1; y <= RGB_Matrix_PixelY; y++) {
        byte bit = digit5x5[yml][y - 1 - RGB_Matrix_OffsetY];
        for (int x = 1; x <= RGB_Matrix_PixelX; x++) {
          //Serial.print(bitRead(bit, 8-x) ? '*' : ' ');
          if ((8 - x + RGB_Matrix_OffsetX >= 0) && (y - 1 - RGB_Matrix_OffsetY >= 0) && (bitRead(bit, 8 - x + RGB_Matrix_OffsetX))) {
            if (RGB_Matrix_Direction == 2) {
              if (y % 2 == 1) {  //正向行
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + RGB_Matrix_PixelX - x, color);
              } else {
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, color);
              }
            } else {
              strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, color);
            }
          } else {
            if (RGB_Matrix_Direction == 2) {
              if (y % 2 == 1) {  //正向行
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + RGB_Matrix_PixelX - x, strip.Color(0, Tally_Background, Tally_Background));
              } else {
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, strip.Color(0, Tally_Background, Tally_Background));
              }
            } else {
              strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, strip.Color(0, Tally_Background, Tally_Background));
            }
          }
        }
        strip.show();
      }

      break;


    case 8:
      if (Tally_ID == SwitcherPGM) {
        color = strip.Color(Tally_Brightnes, 0, 0);
      } else if (Tally_ID == SwitcherPVW) {
        color = strip.Color(0, Tally_Brightnes, 0);
      } else {
        color = strip.Color(Tally_Darkness, Tally_Darkness, Tally_Darkness);
      }
      for (int y = 1; y <= RGB_Matrix_PixelY; y++) {
        byte bit = digit5x5[yml][y - 1 - RGB_Matrix_OffsetY];
        for (int x = 1; x <= RGB_Matrix_PixelX; x++) {
          //Serial.print(bitRead(bit, 8-x) ? '*' : ' ');
          if ((8 - x + RGB_Matrix_OffsetX >= 0) && (y - 1 - RGB_Matrix_OffsetY >= 0) && (bitRead(bit, 8 - x + RGB_Matrix_OffsetX))) {
            if (RGB_Matrix_Direction == 2) {
              if (y % 2 == 1) {  //正向行
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + RGB_Matrix_PixelX - x, strip.Color(0, Tally_Background, Tally_Background));
              } else {
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, strip.Color(0, Tally_Background, Tally_Background));
              }
            } else {
              strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, strip.Color(0, Tally_Background, Tally_Background));
            }
          } else {
            if (RGB_Matrix_Direction == 2) {
              if (y % 2 == 1) {  //正向行
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + RGB_Matrix_PixelX - x, color);
              } else {
                strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, color);
              }
            } else {
              strip.setPixelColor((y - 1) * RGB_Matrix_PixelX + x - 1, color);
            }
          }
        }
        strip.show();
      }

      break;


    default:
      Tally_Style = 1;
      Tally_Light(1);
      break;
  }
}

void LED_Flicker() {
  FlickerCount = 255;
}

void LED_Flash(int on, int off) {
  digitalWrite(Pin_LED, LOW);
  if (RGB_Pixel > 0) {
    strip.setPixelColor(0, strip.Color(0, 128, 128));
    strip.show();
  }
  delay(on);
  digitalWrite(Pin_LED, HIGH);
  if (RGB_Pixel > 0) {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  }
  delay(off);
}

void LED_Error() {
  digitalWrite(Pin_LED, LOW);
  if (RGB_Pixel > 0) {
    strip.setPixelColor(0, strip.Color(128, 0, 0));
    strip.show();
  }
}



void AClick() {
  Tally_Style++;
  Tally_Light(1);
  debug("Aclick,Tally_Style:" + String(Tally_Style));
}
void AdoubleClick() {
  Tally_ID++;
  if (Tally_ID > 8) { Tally_ID = 1; }
  strip.clear();
  strip.fill(strip.Color(128, 128, 128), 0, Tally_ID);
  strip.show();
  debug("AdoubleClick,Tally_ID:" + String(Tally_ID));
}
void ALongPressStart(void *oneButton) {
  int longtime = ((OneButton *)oneButton)->getPressedMs();
  debug("ADuringLongPressStart");
}
void ADuringLongPress(void *oneButton) {
  int longtime = ((OneButton *)oneButton)->getPressedMs();
  Tally_Brightnes += 1;
  if (Tally_Brightnes > 255) { Tally_Brightnes = 1; }
  debug("ADuringLongPress,Brightnes:" + String(Tally_Brightnes));
  Tally_Light(1);
}
void ALongPressStop(void *oneButton) {
  int longtime = ((OneButton *)oneButton)->getPressedMs();
  debug("ADuringLongStop");
  debug(longtime);
  if (longtime > 30000) {
    Config_Save(1);
  }
}


void Config_Load() {
  debug("Config_Load");
  uint8_t Config = EEPROM.read(0);
  if (Config == 1) {
    Pin_LED = EEPROM.read(1);
    Pin_RGB = EEPROM.read(2);
    Pin_KEY = EEPROM.read(3);
    Light_Type = EEPROM.read(4);
    RGB_Pixel = EEPROM.read(5);
    RGB_Matrix_PixelX = EEPROM.read(6);
    RGB_Matrix_PixelY = EEPROM.read(7);
    RGB_Matrix_OffsetX = EEPROM.read(8);
    RGB_Matrix_OffsetY = EEPROM.read(9);
    RGB_Matrix_Direction = EEPROM.read(10);
    Tally_ID = EEPROM.read(11);
    Tally_Style = EEPROM.read(12);
    Tally_Brightnes = EEPROM.read(13);
    Tally_Darkness = EEPROM.read(14);
    Tally_Background = EEPROM.read(15);
    Switcher_Type = EEPROM.read(16);
    uint8_t Switcher_IP1 = EEPROM.read(17);
    uint8_t Switcher_IP2 = EEPROM.read(18);
    uint8_t Switcher_IP3 = EEPROM.read(19);
    uint8_t Switcher_IP4 = EEPROM.read(20);
    Switcher_IP = String(Switcher_IP1) + "." + String(Switcher_IP2) + "." + String(Switcher_IP3) + "." + String(Switcher_IP4);
  }
}


void Config_Save(uint8_t rest) {
  debug("Config_Save");
  if (rest == 1) {
    EEPROM.write(0, 0);
    EEPROM.commit();
    ESP.restart();
  }
  EEPROM.write(0, 1);
  EEPROM.write(1, Pin_LED);
  EEPROM.write(2, Pin_RGB);
  EEPROM.write(3, Pin_KEY);
  EEPROM.write(4, Light_Type);
  EEPROM.write(5, RGB_Pixel);
  EEPROM.write(6, RGB_Matrix_PixelX);
  EEPROM.write(7, RGB_Matrix_PixelY);
  EEPROM.write(8, RGB_Matrix_OffsetX);
  EEPROM.write(9, RGB_Matrix_OffsetY);
  EEPROM.write(10, RGB_Matrix_Direction);
  EEPROM.write(11, Tally_ID);
  EEPROM.write(12, Tally_Style);
  EEPROM.write(13, Tally_Brightnes);
  EEPROM.write(14, Tally_Darkness);
  EEPROM.write(15, Tally_Background);
  EEPROM.write(16, Switcher_Type);
  uint8_t IPint[4];
  IP2int(Switcher_IP, IPint);
  EEPROM.write(17, IPint[0]);
  EEPROM.write(18, IPint[1]);
  EEPROM.write(19, IPint[2]);
  EEPROM.write(20, IPint[3]);
  EEPROM.commit();
}



bool TCPing(String ip, int port) {
  if (tcping.connect(ip, port)) {
    debug("TCPing OK");
    tcping.stop();
    return true;
  } else {
    debug("TCPing NK");
    return false;
  }
}


void Update_Firmware(String url) {
  debug("Update Firmware " + url);
  UpdateCount = 1;
  ESPhttpUpdate.setClientTimeout(6000);
  ESPhttpUpdate.setLedPin(Pin_LED, LOW);
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
  switch (ret) {
    case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
    case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;
    case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
  }
}
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}
void update_error(int err) {
  digitalWrite(Pin_LED, LOW);
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}


void debug(int msg) {
  if (SYSDebug == 0) { return; }
  Serial.print("[DEBUG]");
  Serial.println(msg);
}
void debug(String msg) {
  if (SYSDebug == 0) { return; }
  Serial.print("[DEBUG]");
  Serial.println(msg);
}


void IP2int(String IP, uint8_t IPint[4]) {
  uint8_t startIndex = 0;
  uint8_t endIndex = 0;
  for (uint8_t i = 0; i < 4; i++) {
    endIndex = IP.indexOf('.', startIndex);
    if (endIndex == -1) {
      endIndex = IP.length();
    }
    String tmp = IP.substring(startIndex, endIndex);
    IPint[i] = tmp.toInt();
    startIndex = endIndex + 1;
  }
}


String replaceSubstring(String original, const String &target, const String &replacement) {
  int startPos = original.indexOf(target);
  if (startPos == -1) {
    return original;
  }
  String result = "";
  result += original.substring(0, startPos);
  result += replacement;
  result += original.substring(startPos + target.length());
  return result;
}


String extractTagValue(const String &data, const String &tagStart, const String &tagEnd) {
  int startIdx = data.indexOf(tagStart);
  int endIdx = data.indexOf(tagEnd);

  if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
    startIdx += tagStart.length();
    return data.substring(startIdx, endIdx);
  }
  return "";
}
