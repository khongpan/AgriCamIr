#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

//#include "soc/soc.h"           // Disable brown out problems
//#include "soc/rtc_cntl_reg.h"  // Disable brown out problems

#include "Storage.h"
//#include "Camera.h"
#include "WebPost.h"
#include "MLX90621.h"

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

#define LED 33
String my_id_str = "__agricam03__1000__100";
String pict_folder = "/pict/";
const char* ssid = "FlyFly";
const char* password = "flyuntildie";


void startCameraServer();


void LedOn(){
  digitalWrite(LED,0);
}

void LedOff() {
  digitalWrite(LED,1);
}


void NetMaintain() {
  int connect_fail=0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi connecting");
    WiFi.disconnect();
    WiFi.begin();
    int wait_cnt=60;
    while (wait_cnt-- > 0) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\r\nWifi connected");
        connect_fail=0;
        return;
      }
      LedOn();
      delay(500);
      LedOff();
      delay(500);
      Serial.print(".");
    }  
    connect_fail++;
    if (connect_fail >= 5) ESP.restart();
  } 
}

void TimeSync() {
  struct tm tmstruct ;
  byte daysavetime=0;
  long timezone=7;
  int sync_fail=0;
  int wait_sec=0;
  int sync=0;

  while(sync_fail<3) {
  
    Serial.println("Contacting Time Server");
    //configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
    configTime(3600 * timezone, daysavetime * 3600, "clock.nectec.or.th", "0.pool.ntp.org", "1.pool.ntp.org");              //Set configTime NTP
    vTaskDelay(2000);
    tmstruct.tm_year = 0;
    Serial.print("OK");
    wait_sec=0;
    while (wait_sec<20)
    {
      Serial.print(".");
      LedOn();
      if (getLocalTime(&tmstruct, 900)) {
        Serial.printf("\r\nNow is : %d-%02d-%02d %02d:%02d:%02d\r\n", (tmstruct.tm_year) + 1900, ( tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
        return;
      }
      LedOff();
      delay(100);
      wait_sec++;
    }
    sync_fail++;
  }

  ESP.restart();

}


void setup() {

  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(LED,OUTPUT_OPEN_DRAIN);
  StorageSetup();
  MLX90621Setup();
  //CameraSetup();
  WiFi.begin(ssid, password);
  delay(1000);
  NetMaintain();
  TimeSync();
  //startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

}




String GetCurrentTimeString() {
  struct tm t;  
  char str[20];
  while (!getLocalTime(&t, 1000))
  {
    Serial.print(".");
  }
  sprintf(str,"%04d%02d%02d%02d%02d%02d",(t.tm_year) + 1900, (t.tm_mon) + 1, t.tm_mday, t.tm_hour , t.tm_min, t.tm_sec);
  Serial.printf("\r\n\nNow is : %s\r\n", str );

  return String(str);
}




int pic_cnt=0;
int capture_retry=0;
void loop() {

  NetMaintain();

  {
    struct tm t;
    static int cnt=0,led=0;
    cnt++;
    if (cnt==30) {
      cnt=0;
      LedOn();   
    } else if (cnt==4) {
      LedOff();
    }

    vTaskDelay(100);
    getLocalTime(&t, 5000);
    if (pic_cnt>0) {
      //if (((t.tm_min % 5 != 0) || (t.tm_sec != 0))) return;
      //if (((t.tm_min % 5 != 4) || (t.tm_sec != 55))) return;
      //if (((t.tm_min % 15 != 14) || (t.tm_sec != 55))) return;
      //if (((t.tm_min  != 0) || (t.tm_sec != 0))) return; //every hour
      //if (t.tm_sec != 0) return; //every minute
      if (t.tm_sec%10 !=0) return;
    }
  }


  if (capture_retry>=3) {
    capture_retry = 0;
    //put code to init camera here
    //ESP.restart();
    
  }

  //camera_fb_t *fb= (camera_fb_t *)1;
  void *fb=(void *)1;
  
  String time_str= GetCurrentTimeString();
  String img_name = time_str + my_id_str + ".jpg";

  //CameraFlash(1);
  //fb=CameraCapture();
  //CameraFlash(0);
  MLX90621GetImage();
  delay(1000);
  //return;

  if (fb==NULL)  {
    capture_retry++;
    return;
  }
  capture_retry=0;
  Serial.println("pic_number: " + String(pic_cnt++));
  int send_try=0;


  if(send_try==0) {
    String full_file_path = pict_folder + img_name;
    Serial.printf("Picture file name: %s\n", full_file_path.c_str());
    //StorageWriteFile(full_file_path.c_str(),fb->buf,fb->len);
  }

  String res="ok";
  do { 
    send_try++;
    Serial.println("try " + String(send_try));
    //res=WebPostSendImage(img_name,time_str,fb->buf,fb->len);
    Serial.println("Server Response:");
    Serial.print(res);
  } while(res!="ok" && (send_try < 10));

  //CameraRelease(fb);
 

}

