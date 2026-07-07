#pragma once
#include <Arduino.h>

#define CRED_FILE "/wifi_creds.txt"

bool saveCredentials(const String& ssid, const String& password);
bool loadCredentials(String& ssid, String& password);
void clearCredentials();
