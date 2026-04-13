/*
 * ESP32 Smart Door Lock System
 * ----------------------------
 * A dual-access door lock I built to explore embedded systems and IoT security.
 * Users can unlock the door in two ways: by entering a PIN on a 4x4 keypad,
 * or remotely through a password-protected web page hosted on the ESP32 itself.
 *
 * Hardware: ESP32 Dev Board, 4x4 Keypad, 16x2 I2C LCD, SG90 Servo Motor
 * Libraries: Keypad, LiquidCrystal_I2C, ESP32Servo, WiFi, WebServer
 */

#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ---- WiFi credentials ----
// Replace these with your own network details before uploading.
const char* ssid = "YOUR_WIFI_NAME";
const char* pass = "YOUR_WIFI_PASSWORD";

// ---- Web login credentials ----
// Used to protect the remote unlock page from unauthorized users.
const char* webUser = "admin";
const char* webPass = "4242";

// Web server runs on port 80 (standard HTTP).
WebServer server(80);

// ---- Keypad PIN ----
// This is the code users type on the physical keypad to unlock the door.
String password = "4242";
String input = "";

// ---- LCD setup ----
// I2C address 0x27 is common; run an I2C scanner if yours is different.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- Servo setup ----
// The servo acts as the physical lock mechanism: 0° = locked, 90° = unlocked.
Servo lockServo;
const int servoPin = 15;
bool isLocked = true;

// ---- Keypad wiring ----
// Standard 4x4 matrix keypad mapped to ESP32 GPIO pins.
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// ===================== WEB INTERFACE =====================
// Builds a simple, mobile-friendly HTML page that shows the current
// lock status and gives the user two buttons: Unlock and Lock.
String htmlPage() {
  String status = isLocked ? "LOCKED" : "UNLOCKED";
  String color  = isLocked ? "#c0392b" : "#27ae60";
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Smart Lock</title><style>";
  html += "body{font-family:Arial;text-align:center;background:#2c3e50;color:white;padding:40px;}";
  html += ".status{font-size:32px;padding:20px;background:" + color + ";border-radius:10px;margin:20px;}";
  html += "a{display:inline-block;padding:20px 40px;font-size:24px;margin:10px;border-radius:10px;";
  html += "text-decoration:none;color:white;}";
  html += ".unlock{background:#27ae60;} .lock{background:#c0392b;}";
  html += "</style></head><body><h1>ESP32 Smart Lock</h1>";
  html += "<div class='status'>" + status + "</div>";
  html += "<a class='unlock' href='/unlock'>UNLOCK</a>";
  html += "<a class='lock' href='/lock'>LOCK</a></body></html>";
  return html;
}

// Each web route first checks for valid login credentials.
// If the user isn't authenticated, the browser is prompted to log in.
void handleRoot() {
  if (!server.authenticate(webUser, webPass))
    return server.requestAuthentication();
  server.send(200, "text/html", htmlPage());
}

void handleUnlock() {
  if (!server.authenticate(webUser, webPass))
    return server.requestAuthentication();
  unlockDoor();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLock() {
  if (!server.authenticate(webUser, webPass))
    return server.requestAuthentication();
  lockDoor();
  server.sendHeader("Location", "/");
  server.send(303);
}


// ===================== LOCK ACTIONS =====================
// Rotates the servo and updates the LCD to show access status.
void unlockDoor() {
  lockServo.write(90);
  isLocked = false;
  lcd.clear();
  lcd.print("Access Granted");
  lcd.setCursor(0, 1);
  lcd.print("Door Unlocked");
}

// Returns the servo to the locked position and resets the LCD prompt.
void lockDoor() {
  lockServo.write(0);
  isLocked = true;
  showEnterPassword();
}


// ===================== SETUP =====================
// Runs once at boot: initializes the LCD, servo, WiFi, and web server.
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lockServo.attach(servoPin);
  lockServo.write(0); // start in locked position

  // Try connecting to WiFi for up to ~10 seconds.
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, pass);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    tries++;
  }

  // Show the assigned IP on the LCD so the user knows where to connect.
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(3000);
  } else {
    lcd.print("WiFi Failed");
    delay(2000);
  }

  // Register the web server routes.
  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);
  server.on("/lock", handleLock);
  server.begin();

  showEnterPassword();
}


// ===================== MAIN LOOP =====================
// Continuously listens for both web requests and keypad presses.
void loop() {
  server.handleClient();

  char key = keypad.getKey();
  if (key) {
    if (key == '#') {            // '#' submits the entered PIN
      checkPassword();
    } else if (key == '*') {     // '*' clears the current input
      input = "";
      showEnterPassword();
    } else {
      // Each digit is stored internally but shown as '*' on the LCD
      // so that anyone watching can't read the PIN.
      input += key;
      lcd.setCursor(input.length() - 1, 1);
      lcd.print('*');
    }
  }
}


// ===================== HELPERS =====================
// Resets the LCD to the default "Enter Password" screen.
void showEnterPassword() {
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0, 1);
}

// Compares the entered PIN with the stored password.
// On success: unlock for 5 seconds, then auto-lock again.
// On failure: show "Access Denied" briefly and reset.
void checkPassword() {
  lcd.clear();
  if (input == password) {
    unlockDoor();
    delay(5000);
    lockDoor();
  } else {
    lcd.print("Access Denied");
    delay(2000);
    showEnterPassword();
  }
  input = "";
}
