//2. Coding Mikrokontroller
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include "FS.h"
#include "SD_MMC.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

unsigned long lastTime = 0; // menyimpan waktu terakhir data dibaca
unsigned long timerDelay = 100; // interval 10 detik

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

bool isFileEmpty(fs::FS &fs, const char * path) {
  File file = fs.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return true; // Treat non-existent file as empty
  }
  bool isEmpty = file.size() == 0;
  file.close();
  return isEmpty;
}

void writeDataToFile(fs::FS &fs, const char * path, sensors_event_t a, sensors_event_t g) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  if (isFileEmpty(fs, path)) {
    String header = String("TimeStamp") + "," + 
                    String("AccX") + "," + String("AccY") + "," + String("AccZ") + "," +
                    String("GyrX") + "," + String("GyrY") + "," + String("GyrZ") + "\n";
    if (!file.print(header)) {
      Serial.println("Failed to write header");
      file.close();
      return;
    }
  }

  // Konversi nilai percepatan dari m/s^2 ke g
  float accel_scale = 1.0f / 9.81f; // 1 g = 9.81 m/s^2
  float accel_x_g = a.acceleration.x * accel_scale;
  float accel_y_g = a.acceleration.y * accel_scale;
  float accel_z_g = a.acceleration.z * accel_scale;

  // Membuat string data dengan nilai percepatan yang telah diubah ke satuan g
  String dataString = String(getTimeStamp()) + "," + 
                      String(accel_x_g) + "," + String(accel_y_g) + "," + String(accel_z_g) + "," +
                      String(g.gyro.x) + "," + String(g.gyro.y) + "," + String(g.gyro.z) + "\n";

  if (file.print(dataString)) {
    Serial.println("Data appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

String getTimeStamp() {
  unsigned long currentTime = millis();
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  return String(hours) + ":" + String(minutes) + ":" + String(seconds) + ":" + String(currentTime % 1000);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup");

  // Try to initialize the MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // Set up the display
  Serial.println("Initializing display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  Serial.println("Display initialized");
  display.display();
  delay(2000);
  display.clearDisplay();

  // Setting up MPU6050 settings
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize SD card
  Serial.println("Initializing SD card");
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("SD card initialized");

  listDir(SD_MMC, "/", 2); // List directory contents
}

void loop() {
  unsigned long currentMillis = millis(); // mendapatkan waktu saat ini

  // Cek apakah interval waktu telah berlalu
  if ((currentMillis - lastTime) >= timerDelay) {
    lastTime = currentMillis; // memperbarui waktu terakhir data dibaca

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("AccX : "); display.print(a.acceleration.x / 9.81f);
    display.print(" Y : "); display.print(a.acceleration.y / 9.81f);
    display.print(" Z : "); display.print(a.acceleration.z / 9.81f);
    display.println(" g");
    display.print("GyrX : "); display.print(g.gyro.x);
    display.print(" Y: "); display.print(g.gyro.y);
    display.print(" Z: "); display.print(g.gyro.z);
    display.println(" g");
    display.display();

    writeDataToFile(SD_MMC, "/data.csv", a, g); // Simpan data ke file CSV
  }
}
