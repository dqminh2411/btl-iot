#define BLYNK_TEMPLATE_ID "TMPL6D2c1Q49U"
#define BLYNK_TEMPLATE_NAME "btl iot"
#define BLYNK_AUTH_TOKEN "73BUlmnRizeDoq7Gy3l8lkWLty6Zn-dS"
#define BLYNK_PRINT Serial

#include <HTTPClient.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClient.h>
#include "esp_camera.h"
#include <ESPping.h>
#include "board_config.h"
#include "camera_pins.h"

// #define PHOTO_PIN 12
#define PIR_PIN 13
#define LED_PIN 14
#define BUZZER_PIN 12
#define FLASH_PIN 4

// Cấu hình WiFi
// char ssid[] = "TP-Link_7130";
// char password[] = "22515360";
char ssid[] = "111111";
char password[]= "lessless";

void startCameraServer(); // phải có triển khai (ví dụ từ camera_web_server sample)
bool uploadImageToServer();
void takePhoto(); // khai báo trước
void setupLedFlash(int pin = 4){
  ledcAttach(pin,5000,8);
  ledcWrite(pin,0);
};
BlynkTimer timer;
WidgetTerminal terminal(V5);
void sendLog(const String &msg){
  if(!Blynk.connected()) return;
  Serial.println(msg);
  terminal.println(msg);
  terminal.flush();
}

bool autoTakePhoto;
bool firstTrigger;
bool lowPowerMode = true;
unsigned long lastActive = 0;

void setup() {
  
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();
  // SET PIN MODE
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FLASH_PIN,OUTPUT);

  digitalWrite(BUZZER_PIN,LOW);
  digitalWrite(LED_PIN,LOW);
  
  firstTrigger = true;

  toLowPowerMode();
}

  // control buzzer
  BLYNK_WRITE(V1){
    if (param.asInt() == 1){
          // bật buzzer (tùy mạch)
          digitalWrite(BUZZER_PIN,HIGH);
          Serial.println("Buzzer turned on (V1 = 1)");
      } else {
          digitalWrite(BUZZER_PIN,LOW);
          Serial.println("Buzzer turned off (V1 = 0)");
      }
  }

  // control led
  BLYNK_WRITE(V2){
    if (param.asInt() == 1){
        digitalWrite(LED_PIN,HIGH);
        Serial.println("LED turned on (V2 = 1)");
    } else {
        digitalWrite(LED_PIN,LOW);
        Serial.println("LED turned off (V2 = 0)");
    }
  }

  // control auto take photo
  BLYNK_WRITE(V7){
    if (param.asInt() == 1){
        autoTakePhoto = true;
        Serial.println("auto take photo ON");
    } else {
        autoTakePhoto = false;
        Serial.println("auto take photo OFF");
    }
  }

  // control take photo
  BLYNK_WRITE(V0){
    if(param.asInt()==1){
      Serial.println("TAKE PHOTO");
      takePhoto();
    }
  }

// Thời gian tối thiểu giữa 2 lần báo động (ms)
const unsigned long MOTION_COOLDOWN = 3000;

// Lọc nhiễu: PIR phải giữ mức HIGH trong X ms mới được coi là thật
const unsigned long PIR_STABLE_TIME = 1000;

unsigned long lastMotionTime = 0;
unsigned long pirHighStart = 0;
bool pirWasHigh = false;
int ACTIVE_LIFE_TIME = 30*60*1000; // 30 minutes
long startActiveTime = 0;

void loop() {
  unsigned long now = millis();

  // PIR check (thường PIR = HIGH khi phát hiện chuyển động)
  if (digitalRead(PIR_PIN) == HIGH) {
    if(!pirWasHigh){
      pirWasHigh = true;
      pirHighStart = now;
    }else{
      if(now - pirHighStart >= PIR_STABLE_TIME){
        sendLog("Motion detected");
        if(firstTrigger){
          toActiveMode();

          // auto take photo
          autoTakePhoto = true;
          if(Blynk.connected()){
            Blynk.virtualWrite(V7,1);
            Serial.println("set auto take photo true");
          }

          digitalWrite(BUZZER_PIN,HIGH);
          Blynk.virtualWrite(V1,1);
          digitalWrite(LED_PIN,HIGH);
          Blynk.virtualWrite(V2,1);
          firstTrigger = false;
        }
        if(now - lastMotionTime >= MOTION_COOLDOWN){
            if (Blynk.connected()) {
              Blynk.logEvent("intrusion_detected", "intrusion detected !!!!!!!!!");
            } else {
              Serial.println("Blynk not connected: can't log event now.");
            }
        }
        lastMotionTime = now;
        if(autoTakePhoto)
          takePhoto();
      }
    }
  }else{
    if(now - lastMotionTime >= 8000){
      pirWasHigh = false;
      pirHighStart = 0;
      digitalWrite(BUZZER_PIN,LOW);
      Blynk.virtualWrite(V1,0);
      digitalWrite(LED_PIN,LOW);
      Blynk.virtualWrite(V2,0);
    }
    if(now - lastMotionTime >= ACTIVE_LIFE_TIME){
      toLowPowerMode();
    }
  }

  delay(100);
}

void initCamera(){
  setupLedFlash(FLASH_PIN);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_SVGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  //SETUP IMAGE QUALITY
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  }
//   } else {
//     // Best option for face detection/recognition
//     config.frame_size = FRAMESIZE_240X240;
// #if CONFIG_IDF_TARGET_ESP32S3
//     config.fb_count = 2;
// #endif
//   }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
  startCameraServer();
}
void initWifi(){
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  while (WiFi.localIP().toString() == "0.0.0.0") {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("✅ Got IP: " + WiFi.localIP().toString());

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}
void initBlynk(){
  Blynk.begin(BLYNK_AUTH_TOKEN,ssid,password);
  sendLog("Connected to Blynk cloud =))))");
  sendLog("ESP32-CAM started");
}
void toActiveMode(){
  Serial.println(">>> Enter ACTIVE mode");
  initWifi();
  initCamera();
  initBlynk();
  lowPowerMode = false;
  lastActive = millis();
}
void toLowPowerMode(){
  Serial.println(">>> Enter LOW POWER mode");
  esp_camera_deinit();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Blynk.disconnect();
  lowPowerMode = true;
}
bool uploadImageToServer() {
  
  // Capture image
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  Serial.println("Camera capture success");
  Serial.printf("Image size: %d bytes\n", fb->len);

  // Create HTTP client
  HTTPClient http;
  WiFiClient wfclient;
  // Begin HTTP connection
  http.begin(wfclient, "192.168.172.148", 5000,"/upload");
  http.setTimeout(15000);  // 5 second timeout

  // Set content type as image/jpeg
  http.addHeader("Content-Type", "image/jpeg");
  
  // Send POST request with image data
  int httpResponseCode = http.POST(fb->buf, fb->len);
  
  // Return camera frame buffer
  esp_camera_fb_return(fb);
  
  // Check response
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    Serial.printf("Error on sending POST: %d\n", httpResponseCode);
    Serial.println(http.errorToString(httpResponseCode));
    http.end();
    return false;
  }
}
void takePhoto()
{
  // TURN ON FLASH LED
 
  ledcWrite(FLASH_PIN,27);
  delay(200);

  String local_IP = WiFi.localIP().toString();
  String url = "http://" + local_IP + "/capture?_cb=" + String(millis());

  uploadImageToServer();
  Serial.println("Photo URL: " + url);

  ledcWrite(FLASH_PIN,0);
  // Gọi Blynk chỉ nếu đã kết nối
  if (Blynk.connected()) {
    Blynk.virtualWrite(V3,1);
    // setProperty để đẩy url (V1) — nếu bạn dùng widget URLs
    Blynk.setProperty(V3, "urls", url);
    // hoặc Blynk.virtualWrite(V1, url); // nếu bạn dùng Value Display
  } else {
    Serial.println("Blynk not connected: cannot setProperty now.");
  }

  delay(2000);
}