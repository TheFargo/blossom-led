#include "wifi_manager.h"
#include <WiFi.h>

const char* AP_SSID = "Blossom_Setup";
static const byte DNS_PORT = 53;

bool connectToWiFi(const String& ssid, const String& password, int timeoutSeconds) {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  for (int i = 0; i < password.length(); i++) Serial.print("*");
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > (unsigned long)timeoutSeconds * 1000) {
      Serial.println("\n✗ Connection timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✓ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void startProvisioningAP(DNSServer& dnsServer) {
  Serial.println("Starting Provisioning Mode...");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  // Pure AP mode — no radio contention with STA during provisioning
  WiFi.mode(WIFI_AP);

  // IP config MUST happen before softAP()
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Open network — no password parameter
  if (WiFi.softAP(AP_SSID)) {
    Serial.println("✓ Access Point started successfully!");
  } else {
    Serial.println("✗ Failed to start Access Point");
  }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Rogue DNS — redirects all hostnames to the captive portal
  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("✓ DNS server started (captive portal active)");
}
