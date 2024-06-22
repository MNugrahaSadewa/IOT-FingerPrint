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

const char* FIREBASE_HOST = "final-iot-fingerprint-default-rtdb.asia-southeast1.firebasedatabase.app";
const char* FIREBASE_AUTH = "AIzaSyB_g_b9D-uXXFAKS_gLhmVHItrD6rbx2Pc";

// Initialize RTC
RTC_DS3231 rtc;

// Initialize LCD with I2C address 0x27 and size 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize fingerprint sensor
#define RX_PIN 16
#define TX_PIN 17
Adafruit_Fingerprint finger(&Serial2);

// Define pin for LED and Buzzer
#define LED_PIN 18
#define BUZZER_PIN 19
#define RED_LED_PIN 14  // Red LED pin for indicating data presence

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
  lcd.print(" SELAMAT DATANG");
  lcd.setCursor(0, 1);
  lcd.print("DI ZFORCE STORE");
  delay(3000);
  lcd.clear();

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

  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

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
    Serial.println("Fingerprint sensor is active!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

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
        checkUsernameExists(username);
      }
    }
  }
}

void checkUsernameExists(const String& uname) {
  String cleanUname = uname;
  cleanUname.trim();
  cleanUname.toLowerCase(); // Convert the username to lower case to ensure case-insensitivity
  String path = "/Registrasi/" + cleanUname;

  Firebase.get(firebaseData, path);
  Serial.print("Checking path: ");
  Serial.println(path);
  Serial.print("Firebase returned data type: ");
  Serial.println(firebaseData.dataType());
  Serial.print("Firebase returned data: ");
  Serial.println(firebaseData.to<String>());

  if (firebaseData.dataType() == "null" || firebaseData.dataType() == "undefined") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello, ");
    lcd.print(cleanUname);
    Serial.print("Hello, ");
    Serial.println(cleanUname);
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Silahkan Scan:");
    digitalWrite(RED_LED_PIN, LOW);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Data sudah ada");
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(800);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(800);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(800);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(800);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(800);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(800);
    username = "";
    requestUsername();
  }
}

uint8_t getFingerprintID() {
  uint8_t id = finger.getImage();
  if (id != FINGERPRINT_OK) return -1;
  
  id = finger.image2Tz();
  if (id != FINGERPRINT_OK) return -1;
  
  id = finger.fingerFastSearch();
  if (id == FINGERPRINT_OK) {
    // Fingerprint is recognized
    handleSuccess(finger.fingerID);
  } else {
    // Fingerprint is not recognized, attempt registration
    Serial.println("No match found, register new fingerprint");
    registerFingerprint();
  }
  return finger.fingerID;
}

void handleSuccess(int id) {
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
  lcd.print("Welcome Bang");
  lcd.setCursor(0, 1);
  lcd.print(currentTime);
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(4000);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  lcd.clear();

  // Print to Serial
  Serial.print("Found ID #"); Serial.print(id);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  // Send to Firebase
  sendToFirebase(id, currentTime);

  // Reset username for next input
  username = "";
  requestUsername();
}

void registerFingerprint() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Silakan");
  lcd.setCursor(0, 1); // Set cursor to the second row, first column
  lcd.print("registrasi");
  delay(2000);

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        lcd.setCursor(0, 0);
        lcd.print("No finger detected");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        lcd.setCursor(0, 0);
        lcd.print("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        lcd.setCursor(0, 0);
        lcd.print("Imaging error");
        break;
      default:
        lcd.setCursor(0, 0);
        lcd.print("Unknown error");
        break;
    }
    delay(200);
  }

  // Convert image to template
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print("Error");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Scan Lagi Bang");
  delay(2000);

  // Check for the same finger again
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      lcd.setCursor(0, 0);
      lcd.print("OK");
      delay(2000);
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print("Error");
    return;
  }

  // Create a model
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print("Finger registered");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Error registering");
    return;
  }

  // Save model
  p = finger.storeModel(lastFingerprintID++);
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0, 0);
    lcd.print("Stored!");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Store error");
  }
  delay(2000);
  requestUsername(); // Go back to requesting username
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
  result = Firebase.setInt(firebaseData, "/lastUsedId", lastFingerprintID);
  if (result) {
    Serial.println("Last used ID updated in Firebase.");
  } else {
    Serial.print("Failed to update last used ID in Firebase: ");
    Serial.println(firebaseData.errorReason());
  }
}
