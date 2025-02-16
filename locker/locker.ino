#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Pin definitions
const int lock1 = 6;
const int lock2 = 7;
const int lock3 = 8;
const int lock4 = 9;

// State and tracking variables
int selectedLock = 0;
int selectedAction = 0;  // 0: main page, 1: new box, 2: unlock box
String phone_number = "";
String tempPassword = "";
String confirmPassword = "";
String storedPasswords[4] = { "", "", "", "" };
String storedPhoneNumbers[4] = { "", "", "", "" };

unsigned long lockOpenTime = 0;
const unsigned long LOCK_OPEN_DURATION = 30000;  // 3 seconds

int LockChoosen = 1;

// LCD and Keypad setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 3;

char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

byte rowPins[ROWS] = { 5, 4, 3, 2 };
byte colPins[COLS] = { A3, A2, A1 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  // Initialize pins
  pinMode(lock1, OUTPUT);
  pinMode(lock2, OUTPUT);
  pinMode(lock3, OUTPUT);
  pinMode(lock4, OUTPUT);
  Serial.begin(9600);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.backlight();

  // Show main page
  showMainPage();
}

void loop() {
  char key = keypad.getKey();
  Serial.println(key);


  if (key) {
    handleKeyPress(key);
  }

  // Handle lock auto-close
  checkLockAutoOpen();
  lockOpenTime = millis();
}

void handleKeyPress(char key) {
  switch (selectedAction) {
    case 0:  // Main Page
      handleMainPageInput(key);
      break;
    case 1:  // Get New Box
      handleNewBoxInput(key);
      break;
    case 2:  // Unlock Owned Box
      handleUnlockBoxInput(key);
      break;
  }
}

void showMainPage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("* New Box");
  lcd.setCursor(0, 1);
  lcd.print("# Unlock Box");
  delay(200);
  selectedAction = 0;
}

void handleMainPageInput(char key) {
  if (key == '*') {
    Serial.print("MainPageInput");
    // Start new box process
    selectedAction = 1;
    phone_number = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter phone #:");
    lcd.setCursor(0, 1);
  } else if (key == '#') {
    Serial.print("NewBoxInput");

    // Start unlock box process
    selectedAction = 2;
    showLockSelection();
  }
}

void handleNewBoxInput(char key) {
  static int passwordStep = 0;
  switch (passwordStep) {
    case 0:  // Phone Number Input
      if (key >= '0' && key <= '9' && phone_number.length() < 11) {
        phone_number += key;
        lcd.setCursor(0, 1);
        lcd.print(phone_number);
      } else if (key == '#' && phone_number.length() == 11) {
        // Proceed to lock selection
        selectedLock = 0;
        showLockSelection();
        passwordStep = 1;
      }
      break;

    case 1:  // Lock Selection
      if (key >= '1' && key <= '4' && storedPasswords[(key - '0') - 1] == "") {
        selectedLock = key - '0';
        tempPassword = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter 4-digit PW:");
        lcd.setCursor(0, 1);
        passwordStep = 2;
      } else if (storedPasswords[(key - '0') - 1]) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("The box is full:");
        lcd.setCursor(0, 0);
        lcd.print("Choose another box");
      }
      break;

    case 2:  // First Password Input
      if (handlePasswordInput(key, tempPassword)) {
        confirmPassword = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Confirm Password:");
        lcd.setCursor(0, 1);
        passwordStep = 3;
      }
      break;

    case 3:  // Password Confirmation
      if (handlePasswordInput(key, confirmPassword)) {
        if (tempPassword == confirmPassword) {
          // Store password and phone number
          storedPasswords[selectedLock - 1] = tempPassword;
          storedPhoneNumbers[selectedLock - 1] = phone_number;

          unlockSpecificLock(selectedLock);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Box Assigned!");
          delay(1500);

          showMainPage();
          passwordStep = 0;
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Passwords");
          lcd.setCursor(0, 1);
          lcd.print("Don't Match!");
          delay(1500);

          // Reset to first password input
          tempPassword = "";
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Enter 4-digit PW:");
          lcd.setCursor(0, 1);
          passwordStep = 2;
        }
      }
      break;
  }
}

bool handlePasswordInput(char key, String& password) {
  if (key >= '0' && key <= '9' && password.length() < 4) {
    password += key;
    lcd.setCursor(0, 1);
    lcd.print(password);
    return false;
  } else if (key == '#') {
    if (password.length() < 4) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PW too short!");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(1500);
      password = "";
      return false;
    }
    return true;
  }
  return false;
}

void showLockSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Lock:");
  lcd.setCursor(0, 1);
  lcd.print("1 2 3 4");
}

void handleUnlockBoxInput(char key) {
  static String enteredPassword = "";
  static int unlockStep = 0;

  switch (unlockStep) {
    case 0:  // Lock Selection
      if (key >= '1' && key <= '4') {
        selectedLock = key - '0';
        enteredPassword = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enter Password:");
        lcd.setCursor(0, 1);
        unlockStep = 1;
      }
      break;

    case 1:  // Password Entry
      if (key >= '0' && key <= '9' && enteredPassword.length() < 4) {
        enteredPassword += key;
        lcd.setCursor(0, 1);
        lcd.print(enteredPassword);
      } else if (key == '#') {
        if (enteredPassword == storedPasswords[selectedLock - 1]) {
          // Unlock the specific lock
          unlockSpecificLock(selectedLock);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Door Opened!");

          unlockStep = 0;
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong Password!");
          delay(1500);
          showMainPage();
          unlockStep = 0;
        }
      }
      break;
  }
}

void unlockSpecificLock(int lockNumber) {
  switch (lockNumber) {
    case 1:
      digitalWrite(lock1, HIGH);
      break;
    case 2:
      digitalWrite(lock2, HIGH);
      break;
    case 3:
      digitalWrite(lock3, HIGH);
      break;
    case 4:
      digitalWrite(lock4, HIGH);
      break;
  }
}

void checkLockAutoOpen() {
  if (lockOpenTime > 0 && (millis() - lockOpenTime >= LOCK_OPEN_DURATION)) {
    if (storedPasswords[0] = "") {
      unlockSpecificLock(1);
      if (storedPasswords[1] = "") {
        unlockSpecificLock(2);
      }
      if (storedPasswords[2] = "") {
        unlockSpecificLock(3);
      }
      if (storedPasswords[3] = "") {
        unlockSpecificLock(4);
      }
    }
  }
  showMainPage();
  lockOpenTime = 0;
}
