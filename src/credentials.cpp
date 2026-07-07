#include "credentials.h"
#include <LittleFS.h>

bool saveCredentials(const String& ssid, const String& password) {
  File file = LittleFS.open(CRED_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open credentials file for writing");
    return false;
  }
  file.println(ssid);
  file.println(password);
  file.close();
  Serial.println("✓ Credentials saved to flash");
  return true;
}

bool loadCredentials(String& ssid, String& password) {
  if (!LittleFS.exists(CRED_FILE)) {
    Serial.println("No credentials file found");
    return false;
  }
  File file = LittleFS.open(CRED_FILE, "r");
  if (!file) {
    Serial.println("✗ Failed to open credentials file for reading");
    return false;
  }
  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');
  file.close();
  ssid.trim();
  password.trim();
  if (ssid.length() == 0) {
    Serial.println("✗ Invalid credentials in file");
    return false;
  }
  Serial.println("✓ Credentials loaded from flash");
  return true;
}

void clearCredentials() {
  if (LittleFS.exists(CRED_FILE)) {
    LittleFS.remove(CRED_FILE);
    Serial.println("✓ Credentials cleared");
  }
}
