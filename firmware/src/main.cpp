#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "defines.h"
#include "time.h"
#include "sync.h"
#include "decoder.h"
#include "coil.h"

Decoder* decoder;
Coil* coil;
Sync* sync;

inline void init_timer1() {
    timer1_enable(TIM1_DIVIDER, TIM_EDGE, TIM_SINGLE);
    timer1_write(TIM1_SECOND);
    timer1_attachInterrupt(Sync::timerFiredISR_);
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

    attachInterrupt(digitalPinToInterrupt(DCF_IN), Sync::inputLevelChangedISR_, CHANGE);
}

void setup() {
    WiFi.mode(WIFI_OFF);
    #if defined DEBUG || defined DEBUG_FINE || defined DEBUG_FINEST
    Serial.begin(9600);
    #endif

    noInterrupts();
    sync = new Sync();

    init_timer1();
    init_pins();

    delay(100);
    coil = new Coil();
    decoder = new Decoder(sync);

    interrupts();
}

bool is_home_position() {
    return digitalRead(MINUTE_HOME) == LOW && digitalRead(HOUR_HOME) == LOW;
}

// 0 = homing
// 1 = run forward
// 2 = normal operation
int state = 0;

void loop() {
    const bool sec_pending = sync->is_second_pending();
    const int clock_seconds = sync->get_clock_seconds();
    const int clock_minutes = clock_seconds == -1 ? -1 : (clock_seconds / 60) % OVERFLOW_MINS;

    #ifdef DEBUG_FINE
    if (sec_pending) {
        if (clock_seconds != -1) {
            Serial.printf("%02d:%02d:%02d cm=%d | dm = %d | state = %d\n", 
                clock_seconds / 3600,
                (clock_seconds / 60) % 60,
                clock_seconds % 60,
                clock_minutes, coil->get_display_minutes(), state
            );
        } else {
            Serial.printf("?:?:? cm=%d | dm = %d | state = %d\n", 
                clock_minutes, coil->get_display_minutes(), state
            );
        }   
    }
    #endif

    if (state == 0) {
        digitalWrite(LED_BUILTIN, HIGH);

        coil->advance_if_possible();
        
        if (is_home_position()) {
            #ifdef DEBUG
            Serial.println("<S1> HOMED / WAITING FOR SIGNAL RX");
            #endif

            state = 1;
            coil->home();
        }
    } else if (state == 1) {
        digitalWrite(LED_BUILTIN, HIGH);

        if (clock_minutes != -1) {
            if (coil->get_display_minutes() == clock_minutes) {
                #ifdef DEBUG
                Serial.println("<S2> NORMAL OPERATION");
                #endif

                state = 2;
            } else {
                if (coil->advance_if_possible()) {
                    #ifdef DEBUG_FINE
                    Serial.printf("dm = %d\n", coil->get_display_minutes());
                    #endif
                }
            }
        } 
    } else if (state == 2) {
        digitalWrite(LED_BUILTIN, LOW);
        
        const int diff = modulo(clock_minutes - coil->get_display_minutes(), OVERFLOW_MINS);

        if (diff >= 1 && diff <= OVERFLOW_MINS - 5) {
            coil->advance_if_possible();
        }
    }

    const auto signal = sync->signal_pending();
    if (signal.has_value()) {
        decoder->next(std::get<0>(signal.value()), std::get<1>(signal.value()));
    }
}