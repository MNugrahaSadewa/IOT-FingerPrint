#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Adafruit_Fingerprint.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

// WiFi credentials
const char* ssid = "POCO X3 GT";
const char* password = "Nugraha2023";

// Firebase credentials
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

const char* FIREBASE_HOST = "project-iot-fingerprint-default-rtdb.asia-southeast1.firebasedatabase.app";
const char* FIREBASE_AUTH = "AIzaSyAiojSjyNXrMOOsP75WQ1RPk2U3moM2dbA";

// Initialize RTC
RTC_DS3231 rtc;

// Initialize LCD with I2C address 0x27 and size 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize fingerprint sensor
#define RX_PIN 16
#define TX_PIN 17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

// Define pin for LED and Buzzer
#define LED_PIN 18
#define BUZZER_PIN 19

String username = "";
int lastFingerprintID = 0; // To store the last fingerprint ID used

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize I2C for RTC and LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCD initialized");

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Check and initialize the last used fingerprint ID
  initializeLastFingerprintID();

  // Other initializations...
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  } else {
    Serial.println("RTC initialized");
  }

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  requestUsername();
}

void loop() {
  getFingerprintID();
}

void initializeLastFingerprintID() {
  String lastIdPath = "/lastUsedId";
  if (Firebase.getInt(firebaseData, lastIdPath)) {
    lastFingerprintID = firebaseData.intData();
    Serial.print("Last used ID: ");
    Serial.println(lastFingerprintID);
  } else {
    Serial.println("Failed to retrieve last ID, starting from 0");
    lastFingerprintID = 0; // Start from 1, next available ID will be 1
  }
}

void requestUsername() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Masukkan Nama:");
  Serial.println("Masukkan Nama:");
  while (username == "") {
    if (Serial.available() > 0) {
      username = Serial.readStringUntil('\n');
      username.trim(); // Remove any extra newline or spaces
      if (username != "") {
        Serial.print("Hello, ");
        Serial.println(username);
      }
    }
  }
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Silahkan Scan:");
}

uint8_t getFingerprintID() {
  uint8_t id = finger.getImage();
  if (id != FINGERPRINT_OK) return -1;
  
  id = finger.image2Tz();
  if (id != FINGERPRINT_OK) return -1;
  
  id = finger.fingerFastSearch();
  if (id != FINGERPRINT_OK) return -1;

  lastFingerprintID++;  // Increment the ID for each new fingerprint

  DateTime now = rtc.now();
  String currentTime = String(now.year()) + "/" +
                       String(now.month()) + "/" +
                       String(now.day()) + " " +
                       String(now.hour()) + ":" +
                       String(now.minute()) + ":" +
                       String(now.second());

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Registrasi Berhasil");
  lcd.setCursor(0, 1);
  lcd.print(currentTime);
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(4000);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  lcd.clear();

  // Print to Serial
  Serial.print("Found ID #"); Serial.print(lastFingerprintID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  // Send to Firebase
  sendToFirebase(lastFingerprintID, currentTime);

  // Reset username for next input
  username = "";
  requestUsername();

  return lastFingerprintID;
}

void sendToFirebase(int id, String currentTime) {
  String basePath = "/Registrasi/" + username;
  bool result;

  result = Firebase.setInt(firebaseData, basePath + "/id", id);
  if (result) {
    Serial.println("ID sent to Firebase successfully.");
  } else {
    Serial.print("Failed to send ID to Firebase: ");
    Serial.println(firebaseData.errorReason());
  }

  result = Firebase.setString(firebaseData, basePath + "/time", currentTime);
  if (result) {
    Serial.println("Time sent to Firebase successfully.");
  } else {
    Serial.print("Failed to send time to Firebase: ");
    Serial.println(firebaseData.errorReason());
  }

  // Update the last used ID in Firebase
  result = Firebase.setInt(firebaseData, "/lastUsedId", id);
  if (result) {
    Serial.println("Last used ID updated in Firebase.");
  } else {
    Serial.print("Failed to update last used ID in Firebase: ");
    Serial.println(firebaseData.errorReason());
  }
}

