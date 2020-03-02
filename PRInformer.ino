#include <ESP8266WiFi.h>
#include <Servo.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID       "YOUR_SSID"
#define WLAN_PASS       "YOUR_SSID_PASSWORD"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOUR_ADAFRUIT_USERNAME"
#define AIO_KEY         "YOUR_ADAFRUIT_KEY"

#define BUTTON_PIN D1
#define SERVO_PIN D2
#define BAUD_SPEED 115200
#define TEN_SECONDS_DELAY 10000
#define HALF_SECOND_DELAY 500
#define MAX_MOTOR_POSITION 135
#define MIN_MOTOR_POSITION 45

WiFiClient client;
Servo servo;
volatile uint8_t pos = MIN_MOTOR_POSITION;
volatile bool shouldMoveMotor = false;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe pullRequest = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/YOUR_FEED", MQTT_QOS_1);

void pullRequestCallback(char *data, uint16_t len) {
  pos = MAX_MOTOR_POSITION;
  shouldMoveMotor = true;
}

//See https://github.com/esp8266/Arduino/issues/6127#issuecomment-521728962
void ICACHE_RAM_ATTR triggerMotor(){
  pos = MIN_MOTOR_POSITION;
  moveMotor();
}

void setup() {
  Serial.begin(BAUD_SPEED);
  delay(10);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), triggerMotor, RISING);
  servo.attach(SERVO_PIN);
  servo.write(MIN_MOTOR_POSITION);
 
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(HALF_SECOND_DELAY);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
  
  pullRequest.setCallback(pullRequestCallback);
  mqtt.subscribe(&pullRequest);

}

void loop() {
  if (shouldMoveMotor) {
    shouldMoveMotor = false;
    moveMotor();
  }
  
  MQTT_connect();
  mqtt.processPackets(TEN_SECONDS_DELAY);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}

void moveMotor() {
  servo.write(pos);
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { 
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(TEN_SECONDS_DELAY);  
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
