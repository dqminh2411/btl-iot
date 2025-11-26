#define BLYNK_TEMPLATE_ID "TMPL6D2c1Q49U"
#define BLYNK_TEMPLATE_NAME "btl iot"
#define BLYNK_AUTH_TOKEN "73BUlmnRizeDoq7Gy3l8lkWLty6Zn-dS"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>

#define PIR_PIN 13
#define LED_PIN 14
#define BUZZER_PIN 12
#define FLASH_PIN 4


char ssid[] = "TP-Link_7130";
char password[] = "22515360";
BlynkTimer timer;
WidgetTerminal terminal(V5);
// Flask server configuration
const char* flaskServer = "http://192.168.0.107:5000/upload";  // Replace with your PC's IP address

/**
 * Capture and upload image to Flask server
 * @return true if upload successful, false otherwise
 */
bool uploadImageToFlask() {
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
  
  // Begin HTTP connection
  http.begin(flaskServer);
  
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
bool taken = false;
void setup() {
  Serial.begin(115200);
  pinMode(13, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("✅ Got IP: " + WiFi.localIP().toString());

  // Blynk.begin(BLYNK_AUTH_TOKEN,ssid,password);
  // terminal.println("connected to blynk");
  // terminal.flush();
  // digitalWrite(BUZZER_PIN,LOW);
  // digitalWrite(LED_PIN,LOW);

}



  BLYNK_WRITE(V1){
    if (param.asInt() == 1){
          // bật buzzer (tùy mạch)
          digitalWrite(BUZZER_PIN, HIGH);
          Serial.println("Buzzer turned on (V1 = 1)");
      } else {
          digitalWrite(BUZZER_PIN, LOW);
          Serial.println("Buzzer turned off (V1 = 0)");
      }
  }

  BLYNK_WRITE(V2){
    if (param.asInt() == 1){
        digitalWrite(LED_PIN, HIGH);
        Serial.println("LED turned on (V2 = 1)");
    } else {
        digitalWrite(LED_PIN, LOW);
        Serial.println("LED turned off (V2 = 0)");
    }
  }

void loop() {
  // Blynk.run();
  // Serial.println(digital(BUZZER_PIN));
  // terminal.println(digitalRead(BUZZER_PIN));
  // terminal.flush();
  
  // digitalWrite(BUZZER_PIN, HIGH);   // kêu
  // digitalWrite(LED_PIN,HIGH);
  // delay(1000);
  // digitalWrite(BUZZER_PIN, LOW);  // tắt
  // digitalWrite(LED_PIN,LOW);
  // delay(1000);
  if(!taken){
    uploadImageToFlask();
    taken=true;
  }

}

