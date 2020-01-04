#include <WiFiClient.h>

#define SERVER     "agritronics.nstda.or.th"
#define PORT       80
#define BOUNDARY   "--------------------------133747188241686651551404"  
#define TIMEOUT    20000


void WebPostSetup() {

}

//#define F(A)  String(A)
#define toStr(A) String(A)
String CreateHeader(size_t body_len)
{
  String  data;
      data =  toStr("POST /webpost0606/log_imgcap.php HTTP/1.1\r\n");
      data += toStr("cache-control: no-cache\r\n");
      data += toStr("Content-Type: multipart/form-data; boundary=");
      data += BOUNDARY;
      data += "\r\n";
      data += toStr("User-Agent: PostmanRuntime/6.4.1\r\n");
      data += toStr("Accept: */*\r\n");
      data += toStr("Host: ");
      data += SERVER;
      data += toStr("\r\n");
      data += toStr("accept-encoding: gzip, deflate\r\n");
      data += toStr("Connection: keep-alive\r\n");
      data += toStr("content-length: ");
      data += String(body_len);
      data += "\r\n";
      data += "\r\n";
    return(data);
}

String CreateBody(String name , String value)
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += toStr("\r\n");
  data += "Content-Disposition: form-data; name=\"" + name +"\"\r\n";
  data += "\r\n";
  data += value;
  data += "\r\n";
  return (data);
}

String CreateBody4File(String name , String filename)
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += toStr("\r\n");
  data += toStr("Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + filename + "\"\r\n");
  // data += toStr("Content-Disposition: form-data; name=\"" );
  // data += toStr(name);
  // data += toStr("\"; filename=\"");
  // data += toStr(filename) ;
  // data += toStr("\"\r\n");
  data += toStr("Content-Type: image/jpeg\r\n");
  data += toStr("\r\n");
  
  return(data);
}

char buff[256];

String WebPostSendImage(String img_name,String time_str, uint8_t *data_pic,size_t size)
{
  String body1 = CreateBody4File("uploadedfile",img_name);
  String body2 = CreateBody("network","agricam00");
  String body3 = CreateBody("filedatetime",time_str);
  String body_end = String("--")+BOUNDARY+String("--\r\n");
  size_t len = body1.length() + body2.length() + body3.length() + body_end.length() + size;
  String header = CreateHeader(len);


   Serial.print(header);
   Serial.print(body1);
   Serial.print("\r\nimg_size " +String(size) + "\r\n" );
   Serial.print(body2);
   Serial.print(body3);
   Serial.print("\r\n"+body_end);


//#define DRY_RUN
#ifndef DRY_RUN

  WiFiClient client;
   if (!client.connect(SERVER,PORT)) 
   {
    return("connection failed");   
   }

   client.print(header);
   client.print(body1);
   client.write(data_pic,size);
   client.print(body2);
   client.print(body3);
   client.print("\r\n"+body_end);

   delay(20);
   long tOut = millis() + TIMEOUT;
   String serverRes;
   while(client.connected() && tOut > millis()) 
   {
    if (client.available()) 
    {
      size_t len;
      //String serverRes = client.readString();
      len = client.readBytes(buff,255);
      buff[len]='\0';
        //return(String(len));
        if(len==214) return("ok"); else return("nok");
    }
   }
#endif
  return String("Time out\r\n");


}
