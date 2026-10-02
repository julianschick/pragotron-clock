#include <Arduino.h>
#include <WiFiManager.h>
#include <time.h>

//#define DEBUG
#include "_secrets.h"
#include "clock.h"

WiFiManager wm;
Clock* cl;

time_t last_second = 0;

void captivePortalTimedOut() {
    Serial.println("AP timeout.");
    Led::ap_open = false;
}

void paramsStored() {
    Serial.println("WLAN network selected.");
    Led::ap_open = false;
}

static void wifi_event_received(WiFiEvent_t event)
{
  switch (event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
        Led::wlan_connected = true;
        break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
        Led::wlan_connected = false;
        break;
    default:
        break;
  }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(115200);

    WiFi.onEvent(&wifi_event_received);
    
    wm.setConnectTimeout(20);
    wm.setConnectRetries(10);
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(300);
    wm.setConfigPortalTimeoutCallback(&captivePortalTimedOut);
    wm.setSaveParamsCallback(&paramsStored);


    if (wm.autoConnect("Pragotron", WIFI_PORTAL_PASS)) {
        Serial.println("WLAN connected.");
    } else {
        Serial.println("WLAN not connected, opening AP for 120 seconds.");
        Led::ap_open = true;
    }

    Serial.println("Init NTP.");

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1);
    tzset();

    Serial.println("Starting clock.");
    cl = new Clock();
}

void loop() {
    wm.process();
    cl->process();
    Led::process();
}