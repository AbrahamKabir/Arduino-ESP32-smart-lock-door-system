# Arduino-ESP32-smart-lock-door-system
A dual-access smart lock built on the ESP32. Users unlock the door either through a 4x4 keypad with masked PIN entry on an LCD, or remotely via a password-protected web page hosted on the ESP32 itself. Written in embedded C/C++, covering I2C, PWM, WiFi, and basic IoT security — no cloud service required.



The lock can be opened in two ways. The first is a physical 4x4 keypad mounted near the door, where the user enters a 4-digit PIN. The entered digits are masked on the 16x2 LCD as asterisks, so anyone watching over the user's shoulder can't read the actual code. Once the correct PIN is submitted, the servo motor rotates to the unlocked position for a few seconds and then automatically re-locks.

The second method is remote unlock over WiFi. On boot, the ESP32 connects to the local network and hosts a lightweight web page that displays the current lock status and two buttons — Unlock and Lock. The page is protected by HTTP authentication, so only someone with the correct login credentials can actually trigger the lock. The IP address is shown on the LCD at startup so the user knows where to connect from their phone or laptop.

I built this project to get hands-on with embedded C/C++, hardware interfacing (I2C, PWM, GPIO matrix scanning), and the basics of IoT security — specifically how to design a device that's convenient to use but not trivially exploitable. It also gave me a practical look at how microcontrollers handle concurrent tasks, since the ESP32 has to listen for keypad input and web requests at the same time without missing either.

## Hardware Used
- ESP32 Dev Board
- 4x4 Matrix Keypad
- 16x2 I2C LCD Display
- SG90 Servo Motor
- Breadboard + jumper wires

## Libraries
- Keypad
- LiquidCrystal_I2C
- ESP32Servo
- WiFi
- WebServer

## How to Run
1. Install the libraries listed above through the Arduino IDE Library Manager.
2. Add ESP32 board support via the Boards Manager.
3. Open the `.ino` file, update the WiFi SSID and password at the top.
4. Upload to the ESP32. The LCD will display the IP address once connected.
5. Default keypad PIN: `4242`. Default web login: `admin` / `4242`.
