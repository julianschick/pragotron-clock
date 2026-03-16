#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "defines.h"
#include "time.h"
#include "sync.h"
#include "decoder.h"
#include "coil.h"

int clock_minutes = -1;
int display_minutes = -1;

Decoder* decoder;
Coil* coil;

inline void init_timer1() {
    timer1_enable(TIM1_DIVIDER, TIM_EDGE, TIM_SINGLE);
    timer1_write(TIM1_SECOND);
    timer1_attachInterrupt(Sync::timerFiredISR);
}

inline void init_pins() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    digitalWrite(COIL_POSITIVE, LOW);
    pinMode(COIL_POSITIVE, OUTPUT);
    digitalWrite(COIL_POSITIVE, LOW);

    digitalWrite(COIL_NEGATIVE, LOW);
    pinMode(COIL_NEGATIVE, OUTPUT);
    digitalWrite(COIL_NEGATIVE, LOW);

    pinMode(DCF_IN, INPUT);
    pinMode(MINUTE_HOME, INPUT);
    pinMode(HOUR_HOME, INPUT);

    attachInterrupt(digitalPinToInterrupt(DCF_IN), Sync::inputLevelChangedISR, CHANGE);
}

void setup() {
    WiFi.mode(WIFI_OFF);
    #if defined DEBUG || defined DEBUG_FINE || defined DEBUG_FINEST
    Serial.begin(9600);
    #endif

    noInterrupts();

    init_timer1();
    init_pins();

    delay(100);
    coil = new Coil();
    decoder = new Decoder();

    interrupts();
}

bool is_home_position() {
    return digitalRead(MINUTE_HOME) == LOW && digitalRead(HOUR_HOME) == LOW;
}

// 0 = homing
// 1 = homed
int state = 0;

void loop() {

    if (state == 0) {
        coil->advance_if_possible();
        
        if (is_home_position()) {
            state = 1;
            display_minutes = HOME_MINUTES;
        }
    } else if (state == 1) {
        if (clock_minutes != -1 && display_minutes != clock_minutes) {
            if (coil->advance_if_possible()) {
                display_minutes = (display_minutes + 1) % 720;

                #ifdef DEBUG_FINE
                Serial.printf("dm = %d\n", display_minutes);
                #endif
            }
        }
    }

    auto signal = Sync::signal_pending();
    if (signal.has_value()) {
        decoder->next(std::get<0>(signal.value()), std::get<1>(signal.value()));
    }

    if (Sync::is_second_pending()) {
        
        int clock_seconds = Sync::get_clock_seconds();
        if (clock_seconds != -1) {
            clock_minutes = (clock_seconds / 60) % 720;

            #ifdef DEBUG_FINE
            Serial.printf("%02d:%02d:%02d cm=%d | dm = %d | state = %d\n", 
                clock_seconds / 3600,
                (clock_seconds / 60) % 60,
                clock_seconds % 60,
                clock_minutes, display_minutes, state
            );
            #endif

            if (clock_seconds % 60 == 0) {
                if (display_minutes != -1 && ((display_minutes + 1) % 720) == clock_minutes) {
                    if (state == 1) {
                        #ifdef DEBUG_FINE
                        Serial.println(">> advance one minute");
                        #endif

                        coil->advance();
                        display_minutes = (display_minutes + 1) % 720;
                    }
                }
            }
        } else {
            #ifdef DEBUG_FINE
            Serial.printf("?:?:? cm=%d | dm = %d | state = %d\n", 
                clock_minutes, display_minutes, state
            );
            #endif
        }
    }

}