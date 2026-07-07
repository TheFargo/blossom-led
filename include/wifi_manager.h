#pragma once
#include <Arduino.h>
#include <DNSServer.h>

extern const char* AP_SSID;

bool connectToWiFi(const String& ssid, const String& password, int timeoutSeconds = 15);
void startProvisioningAP(DNSServer& dnsServer);
