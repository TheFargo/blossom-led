#pragma once
#include <WebServer.h>

// WebServer instance is defined in main.cpp
extern WebServer server;

// Called by handleConnect() to schedule a deferred reboot — defined in main.cpp
void scheduleReboot(unsigned long delayMs);

// Register routes and start the web server for each operating mode
void setupProvisioningRoutes();
void setupConnectedRoutes();
