#include <Arduino.h>
#include <WiFiManager.h>
#include <time.h>

#define DEBUG
#include "_secrets.h"
#include "clock.h"

WiFiManager wm;
Clock* cl;

time_t last_second = 0;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(120);

    Serial.println("...");

    if(wm.autoConnect("Pragotron", WIFI_PORTAL_PASS)){
        Serial.println("connected...yeey :)");
    }
    else {
        Serial.println("Configportal running");
    }

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1);
    tzset();

    cl = new Clock();
}

void loop() {
    wm.process();
    cl->process();
}