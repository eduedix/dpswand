#include <Wire.h>
#include <SPI.h>
#include <SparkFunLSM9DS1.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "AndroidAP";
const char* password =  "byob6208";

boolean sending = true;
boolean recording = true;

float gyrX, gyrY, gyrZ, accX, accY, accZ, magX, magY, magZ, roll, pitch, heading;

String recorded_data = "";

LSM9DS1 imu;

#define LSM9DS1_M 0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW
static unsigned long lastPrint = 0; // Keep track of print time
#define DECLINATION -3.2 // Declination (degrees) in Munich, Germany.

void setup() {
  
  Serial.begin(115200);

  WiFi.begin(ssid, password); 

  while(WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  
  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = LSM9DS1_M;
  imu.settings.device.agAddress = LSM9DS1_AG;
  
  if(!imu.begin()) {
    Serial.println("Failed to communicate with LSM9DS1.");
    Serial.println("Double-check wiring.");
    Serial.println("Default settings in this sketch will " \
                  "work for an out of the box LSM9DS1 " \
                  "Breakout, but may need to be modified " \
                  "if the board jumpers are.");
    while (1);
  }
}

void loop() {
  
  if(recording) {
    // Update the sensor values whenever new data is available
      if(imu.gyroAvailable()) {
        imu.readGyro();
      }
      if(imu.accelAvailable()) {
        imu.readAccel();
      }
      if(imu.magAvailable()) {
        imu.readMag();
      }
      setData();
      
      recorded_data += "\n";
    }

  if(sending) {
    if(WiFi.status()== WL_CONNECTED) {
      HTTPClient http;   
      http.begin("http://dpswand.appspot.com/gesture/template");
      http.addHeader("Content-Type", "text/plain");
      int httpResponseCode = http.POST(recorded_data);
      
      if(httpResponseCode>0){
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
        sending = false;
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(recorded_data);
      }
      http.end();
    } else {
      Serial.println("Error in WiFi connection");   
    }

    //clear data
    recorded_data = "";
  }
}

void setData() {
  gyrX = store_data(imu.calcGyro(imu.gx));
  gyrY = store_data(imu.calcGyro(imu.gy));
  gyrZ = store_data(imu.calcGyro(imu.gz));
  accX = store_data(imu.calcAccel(imu.ax));
  accY = store_data(imu.calcAccel(imu.ay));
  accZ = store_data(imu.calcAccel(imu.az));
  magX = store_data(imu.calcMag(imu.mx));
  magY = store_data(imu.calcMag(imu.my));
  magZ = store_data(imu.calcMag(imu.mz));
  roll = store_data(getRoll(imu.ay, imu.az));
  pitch = getPitch(imu.ax, imu.ay, imu.az));
  heading = getHeading(-imu.my, -imu.mx, imu.mz));
}

void store_data(float x) {
  recorded_data += String(x);
}

float getRoll(float ay, float az) {
  float roll = atan2(ay, az);
  // Convert from radians to degrees:
  roll  *= 180.0 / PI;
  return roll;
}

float getPitch(float ax, float ay, float az) {
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));
  // Convert from radians to degrees:
  pitch *= 180.0 / PI;
  return pitch;
}

float getHeading(float mx, float my, float mz) {
  if (my == 0)
    heading = (mx < 0) ? PI : 0;
  else
    heading = atan2(mx, my);
    
  heading -= DECLINATION * PI / 180;
  
  if (heading > PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);
  else if (heading < 0) heading += 2 * PI;
  
  // Convert from radians to degrees:
  heading *= 180.0 / PI;
  return heading;
}
