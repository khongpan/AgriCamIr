#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <heltec.h>
#include <Wire.h>


//#include "soc/soc.h"           // Disable brown out problems
//#include "soc/rtc_cntl_reg.h"  // Disable brown out problems

//#include "Storage.h"
//#include "Camera.h"
#include "WebPost.h"
#include "MLX90621.h"
#include "toojpeg.h"

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

#define LED 33
String my_id_str = "__agricam00__1000__100";
String pict_folder = "/pict/";
const char* ssid = "ssid";
const char* password = "magicword";


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

void task_main(void *p) {
  esp_task_wdt_delete(NULL);  
  while(1) {
    esp_task_wdt_reset();

    do_job();
    delay(100);
  }
}

void OLEDsetup() {
  Wire.begin(4,15);
  Heltec.display->init();
  Heltec.display->clear();
  //Heltec.display->flipScreenVertically();
  //Heltec.display->mirrorScreen();
  Heltec.display->setFont(ArialMT_Plain_24);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
}


void setup() {

  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(LED,OUTPUT_OPEN_DRAIN);
  OLEDsetup();
  //StorageSetup();
  MLX90621Setup();
  MLX90621Capture();
  //CameraSetup();

  WiFi.begin(ssid, password);
  delay(1000);
  NetMaintain();
  TimeSync();

  //startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  static TaskHandle_t loopTaskHandle = NULL;
  xTaskCreateUniversal(task_main, "task_main", 50000, NULL, 1, &loopTaskHandle, CONFIG_ARDUINO_RUNNING_CORE);
 
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
unsigned char jpg_pict_buff[1024];
int jpg_pict_buff_index=0;
void write_jpg_pict_buff(unsigned char byte) {
  jpg_pict_buff[jpg_pict_buff_index++]=byte;
  //Serial.print(byte);
}
void reset_jpg_pict_buff() {
  jpg_pict_buff_index=0;
}
int get_jpg_size() {
  return jpg_pict_buff_index;
}






#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
float img_data[SCREEN_WIDTH*SCREEN_HEIGHT];
Normalized_Img_t display_img;

#define MIN_TEMP 27
#define MAX_TEMP 32
#define T2G(d)  (d-MIN_TEMP)/(MAX_TEMP-MIN_TEMP)

Normalized_Img_t *IrData2ScreenImg()  {
  int x,y,i,j;
  int ox,oy;

  float *t;
  float color;


  t=MLX90621GetCaptureTemp();

  display_img.height=64;
  display_img.width=128;
  display_img.buf = img_data;

  for (y = 0; y < 16; y++) {
    for (x = 0; x < 4; x++) {
       color=T2G(t[y*4+x]);
       if (color>1) color=1;
       if (color<0) color=0;
       ox=(15-y)*8;
       oy=x*16;
       for(j=0;j<16;j++) {
         for(i=0;i<8;i++) {
           display_img.buf[(oy+j)*128+(ox+i)]=color;
         }
       }
    }
  }

  return &display_img;
}


//adapted from JEFworks https://gist.github.com/JEFworks/637308c2a1dd8a6faff7b6264104847a
 Normalized_Img_t* DitheringImage(Normalized_Img_t *img) {

  float *buf=img->buf;
  int nrow=64;
  int ncol=128;

  #define a(y,x) (buf[(y)*128+(x)])

    for (int i = 0; i < nrow-1; i++) {
      for (int j = 1; j < ncol-1; j++) {
          int P = trunc(a(i, j)+0.5);
          if (P>1) P=1;
          float e = a(i, j) - P;
          a(i, j) = P;
          a(i, j+1) = a(i, j+1) + (e * 7/16);
          a(i+1, j-1) = a(i+1, j-1) + (e * 3/16);
          a(i+1, j) = a(i+1, j) + (e * 5/16);
          a(i+1, j+1) = a(i+1, j+1) + (e * 1/16);
          //Serial.printf("%1.0f",a(i,j));
          
      }
      //Serial.println();
      
  }
  return img;

}

void ShowSummary(void) {
  static char str[100];
  float min,max,avg,amb;
  min=MLX90621GetCaptureMinTemp();
  max=MLX90621GetCaptureMaxTemp();
  avg=MLX90621GetCaptureAvgTemp();
  amb=MLX90621GetCaptureTa();
  
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0,10,String("MLX90621 far Ir sensor"));

  Heltec.display->setFont(ArialMT_Plain_24);
  sprintf(str,"%2.1f",avg);
  Heltec.display->drawString(0,26,String(str));

  Heltec.display->setFont(ArialMT_Plain_10);
  sprintf(str,"%2.1f %2.1f",min,max);
  Heltec.display->drawString(48,26,String(str));

  Heltec.display->setFont(ArialMT_Plain_10);
  sprintf(str,"Ta=%2.1f",amb);
  Heltec.display->drawString(0,50,String(str));
    
  Heltec.display->display();

}


void ShowImg() {
  int x,y;
  Normalized_Img_t *img;
  //Serial.println("IrData2ScreenImg");
  img=IrData2ScreenImg();
  //Serial.println("DitheringImg");
  img=DitheringImage(img);
  //Serial.println("DisplayImg");
  Heltec.display->clear();
  for(y=0;y<SCREEN_HEIGHT;y++) {
    for(x=0;x<SCREEN_WIDTH;x++) {
      if(img->buf[y*SCREEN_WIDTH+x]>0.5)
       Heltec.display->setPixel(x,y);
    }
  }
  Heltec.display->setBrightness(255);
  Heltec.display->display();
  
}

void loop() {
  //Serial.println("Ir image Capture");
  MLX90621Capture();
  //Serial.println("OLED update...");
  //UpdateDisplay();
  ShowImg();
  delay(10);
}

void do_job() {

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
      if (((t.tm_min % 5 != 0) || (t.tm_sec != 0))) return;
      //if (((t.tm_min % 5 != 4) || (t.tm_sec != 55))) return;
      //if (((t.tm_min % 15 != 14) || (t.tm_sec != 55))) return;
      //if (((t.tm_min  != 0) || (t.tm_sec != 0))) return; //every hour
      //if (t.tm_sec != 0) return; //every minute
      //if (t.tm_sec%10 !=0) return;
    }
  }


  if (capture_retry>=3) {
    capture_retry = 0;
    delay(1000);
    return;
    //put code to init camera here
    //ESP.restart();
    
  }

  //camera_fb_t *fb= (camera_fb_t *)1;
  //void *fb=(void *)1;
  MLX90621_img_t *img;
  
  String time_str= GetCurrentTimeString();
  String img_name = time_str + my_id_str + ".jpg";

  //CameraFlash(1);
  //fb=CameraCapture();
  //CameraFlash(0);

  
  //img=MLX90621Capture();
  img = MLX90621GetCaptureImage();
  
  reset_jpg_pict_buff();
  //static char comments[] = "MLX90621 image";
  if (TooJpeg::writeJpeg(write_jpg_pict_buff,img->buf,img->width,img->height,true,90,false, "MLX90621 image")) {
    //img->buf=jpg_pict_buff;
    //img->len=buff_index;
    Serial.printf("converted jpg size %d \r\n",get_jpg_size());
    delay(1000);
    //return;
  }else{
    Serial.printf("converted jpg failed\r\n");
    Serial.printf("f%x b%x w%d h%d\r\n",write_jpg_pict_buff,img->buf,img->width,img->height);
    img=NULL;
  }
  

  if (img==NULL)  {
    if(pic_cnt==0) pic_cnt++;
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
    res=WebPostSendImage(img_name,time_str,jpg_pict_buff,get_jpg_size());
    Serial.println("Server Response:");
    Serial.print(res);
  } while(res!="ok" && (send_try < 10));
  Serial.println("");

  //CameraRelease(fb);
 

}

