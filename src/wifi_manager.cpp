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

  // CRITICAL for iOS: disable the CYW43's WiFi power-save mode. In its default
  // power-save state the radio naps between beacons, so bursts of small packets
  // (exactly what iOS's captive-portal probes look like: several DNS queries +
  // HTTP requests fired within milliseconds of association) get delayed by
  // hundreds of ms or dropped outright. iOS gives up quickly and never shows
  // the portal sheet — retrying the connection over and over "sometimes" works.
  // Full-power mode makes the AP respond instantly. Power draw is a non-issue:
  // Blossom is a mains/USB-powered light display, not a battery device.
  WiFi.noLowPowerMode();
  Serial.println("✓ WiFi power-save disabled (fast captive portal response)");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // CRITICAL for iOS: reply NOERROR (not the default NXDOMAIN) to query types
  // we don't serve. Apple devices look up AAAA and HTTPS(type 65) records for
  // captive.apple.com alongside the A record. NXDOMAIN means "this NAME does
  // not exist" — it applies to the whole hostname, not just the record type —
  // so iOS may conclude captive.apple.com is unreachable and never send the
  // HTTP probe that triggers the portal sheet. NOERROR-with-zero-answers is
  // the correct "name exists, but no record of that type" response.
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  // Rogue DNS — redirects all hostnames to the captive portal
  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("✓ DNS server started (captive portal active)");
}
