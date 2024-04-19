#include <ESP8266WiFi.h>       //ESP8266WIFI 載入Wi-Fi資料庫
#include <ESP8266WebServer.h>  //ESP8266WEBSERVER
#include <ESP8266mDNS.h>       //ESP8266DNS
#include <FS.h>                //SPIFFS 包含檔案系統庫
#include <TimeLib.h>   


      
 //包含時間庫
// 公用變數宣告
const char* ssid = "WeiMingYu 256Pro";  //無線網路SSID
const char* password = "yum@@8022";     //無線網路密碼
const char* ap_ssid = "CHELIC-2356F";   //AP無線網路SSID
const char* ap_password = "12345678";   //AP無線網路密碼
// 設定AP模式的IP地址、閘道和子網掩碼
IPAddress local_IP(192, 168, 4, 1);     //AP模式IP位址 
IPAddress gateway(192,168,4,1);         //AP模式
IPAddress subnet(255, 255, 255, 0);     //AP模式
IPAddress primaryDNS(8, 8, 8, 8);       //主DNS服務器（例如Google的DNS）
IPAddress secondaryDNS(8, 8, 4, 4);     //次DNS服務器
IPAddress staticIP(192, 168, 4, 2);     //STA模式ESP8266的靜態IP地址
WiFiServer server(80);                  //設定web server端口為80 port
String header;                          //設定變數header為HTTP請求值
String output2State = "off";            //輔助變數存儲當前輸出狀態
String message = "待測值";               // 設輸出字串值
const int output2 = 2;                  //GPIO PIN 2 設定輸出PIN
const int analogInPin = A0;             // ESP8266 Analog Pin ADC0 = A0
int sensorValue = 0;                    // ADC數值
unsigned long currentTime = millis();   // 當前時間
unsigned long previousTime = 0;         // 前次時間
const long timeoutTime = 2000;          // 定義暫時時間以毫秒為單位 (example: 2000ms = 2s)


//初始設定值
void setup() {
  Serial.begin(115200);                 // 設定傳輸率
  pinMode(output2, OUTPUT);             // 將輸出變數初始化為輸出PIN引腳模式
  digitalWrite(output2, LOW);           // 將輸出設定為LOW低電位
  setTime(8,0,0,1,1,2024);              // 設定初始時間，例如2024年1月1日 08:00:00
  SPIFFS.begin();                       // 啟動檔案系統
  
  File f = SPIFFS.open("/data.csv", "w"); // 創建並打開一個名為"data.csv"的檔案   
  if (!f) {
    Serial.println("檔案創建失敗");
    return;
  }
  f.println("日期時間,流量數值,累計流量,碳排數值,累計碳排,耗能統計");   // 寫入檔案標題
  f.close();                                                        // 關閉檔案

// Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");      
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);              // 設定ESP8266為第三模式 
 // WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS); //STA模式靜能設定
  WiFi.begin(ssid, password);          //STA 連線設定
  //判斷STA模式有沒有連上無線網路 如沒有連接每秒判斷一次
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("  no link >.< ");
  }                  
  //列出得到的區域端IP及啟動web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP()); // 打印閘道地址  
  // 配置AP模式的IP設置  
  WiFi.softAP(ap_ssid, ap_password);    //AP模式的SSID 及密碼
  WiFi.softAPConfig(local_IP, gateway, subnet );
  Serial.print("AP CHELIC-2356F IP address: ");
  Serial.println(WiFi.softAPIP());          // 打印出AP模式下的IP地址
  
  server.begin();            //啟動WEB SERVER
}

void loop() {
  WiFiClient client = server.available();  // 傾聽clients傳入的值

  if (client) {                     // 假如有一個新的client端連線,
    Serial.println("New Client.");  // 在序列端口中印出訊息
    String currentLine = "";        // 設定String儲存從客戶端傳入的數據
    currentTime = millis();
    previousTime = currentTime;

    //ADC 數值
    // read the analog in value
    sensorValue = analogRead(analogInPin);  //讀取ADC 電壓值
    // print the readings in the Serial Monitor
    Serial.print("sensor = " + sensorValue);       //在序列端口印出訊息
    float voltage = sensorValue * (1.0 / 1023.0);  //轉換數值
    String message1 = "Sensor Value: " + String(sensorValue, 2) + " (ADC)";
    message = "\nVoltage: " + String(voltage, 2) + " V";
    //

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      // 當client端連線時，持續循環{}內動作
      currentTime = millis();
      if (client.available()) {  // 如果有客戶端讀取的字節
        char c = client.read();
        // 閱讀一個字元, 然後
        Serial.write(c);
        // 印出到序列顯示器
        header += c;
        if (c == '\n') {  // 如果字元是換行符號 // 如果當前行為空白，則您可以連續兩個換行符號
          // 這是client端HTTP請求的結束，所以發送回應：
          if (currentLine.length() == 0) {
            // HTTP標頭始終以回應代碼開始 (e.g. HTTP/1.1 200 OK)
            // 和content-type，使客戶知道即將到來，然後是一個空白行
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();  // 打開和關閉GPIO
            if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("GPIO 2 on");
              output2State = "on";
              digitalWrite(output2, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("GPIO 2 off");
              output2State = "off";
              digitalWrite(output2, LOW);
            }


            // 顯示HTML網頁
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS 格式的ON/OFF按鍵
            // 隨時更改背景顏色和字體大小屬性
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");

            // 網頁標題
            client.println("<body>");
            client.println("<h1>CHELIC 流量計 SERVER </h1>");
            client.println("<p>流量計 " + message1 + "</p>");
            client.println("<p>流量計 " + message + "</p>");
            // 顯示當前狀態，以及GPIO 5的開/關按鍵狀態
            client.println("<p>LED 燈 - 等待 " + output2State + "</p>");
            // 如果output5State狀態是關閉，則按鍵會顯示ON
            if (output2State == "off") {
              client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            //表格
            client.println("<h2>CHELIC 流量 耗能 記綠表 </h2>");
            client.println("<table border='2';table align=center><tr><th>日期時間</th><th>流量數值</th><th>累計流量</th><th>碳排數值</th><th>累計碳排</th><th>耗能統計</th></tr>");
            client.println("<tr><td>即   時</td><td>" + String(sensorValue+0.01, 2) + "</td><td>" + String(voltage+0.012, 2) + "</td><td>" + String(voltage+0.02, 3) + "</td><td>" + String(voltage+0.15, 3) + "</td><td>" + String(voltage+0.012, 3) + "</td></tr>");
            client.println("<tr><td>前1小時</td><td>" + String(sensorValue+0.1, 2) + "</td><td>" + String(voltage+0.1, 2) + "</td><td>" + String(voltage+0.12, 3) + "</td><td>" + String(voltage+0.13, 3) + "</td><td>" + String(voltage+0.12, 3) + "</td></tr>");
            client.println("<tr><td>前2小時</td><td>" + String(sensorValue+0.2, 2) + "</td><td>" + String(voltage+0.2, 2) + "</td><td>" + String(voltage+0.13, 3) + "</td><td>" + String(voltage+0.14, 3) + "</td><td>" + String(voltage+0.13, 3) + "</td></tr>");
            client.println("<tr><td>前3小時</td><td>" + String(sensorValue+0.3, 2) + "</td><td>" + String(voltage+0.3, 2) + "</td><td>" + String(voltage+0.14, 3) + "</td><td>" + String(voltage+0.15, 3) + "</td><td>" + String(voltage+0.14, 3) + "</td></tr>");
            client.println("</table>");
            client.println("<P>\n 每1小時自動記錄</P>");
            client.println("<h2>\n CHELIC 流量 耗能 比軙圖 </h2>");
           

            client.println("</body></html>");

            // HTTP回應以隔一行結尾
            client.println();
            // 跳出循環
            break;
          } else {  // 如果你有一個換行符號，就清空currentLine變數值
            currentLine = "";
          }
        } else if (c != '\r') {  // 如果您有其他任何訊息，但不是回車字符,
          currentLine += c;      // 則將其值加到currentLine變數尾端
        }
      }
    }
    // 清除標題變數
    header = "";
    // 關閉連接
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}