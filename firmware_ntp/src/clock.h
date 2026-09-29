#ifndef CLOCK_H_
#define CLOCK_H_

#include "coil.h"
#include "led.h"

#define MINUTE_HOME D6
#define HOUR_HOME D5

#define ADVANCE_DEAD_MILLIS 100

class Clock {

public:
    Clock() : coil(Coil()) {
        digitalWrite(COIL_POSITIVE, LOW);
        pinMode(COIL_POSITIVE, OUTPUT);
        digitalWrite(COIL_POSITIVE, LOW);

        digitalWrite(COIL_NEGATIVE, LOW);
        pinMode(COIL_NEGATIVE, OUTPUT);
        digitalWrite(COIL_NEGATIVE, LOW);

        pinMode(MINUTE_HOME, INPUT);
        pinMode(HOUR_HOME, INPUT);
    }

    void process() {
        bool minute_homed_read = digitalRead(MINUTE_HOME) == LOW && coil.millis_since_last_advance() > ADVANCE_DEAD_MILLIS;
        bool hour_homed_read = digitalRead(HOUR_HOME) == LOW;

        if (minute_homed_read && !minute_homed ) {
            minute_homed = true;
            #ifdef DEBUG
            Serial.println("MIN HOMED -> 1");
            #endif
        }
        if (!minute_homed_read && minute_homed) {
            minute_homed = false;
            #ifdef DEBUG
            Serial.println("MIN HOMED -> 0");
            #endif
        }

        if (hour_homed_read && !hour_homed) {
            hour_homed = true;
            #ifdef DEBUG
            Serial.println("HOUR HOMED -> 1");
            #endif
        }
        if (!hour_homed_read && hour_homed) {
            hour_homed = false;
            #ifdef DEBUG
            Serial.println("HOUR HOMED -> 0");
            #endif
        }

        // if (minute_homed_input && minute_homed_debounce < DEBOUNCE_CYCLES) {
        //     minute_homed_debounce++;
        //     if (minute_homed_debounce == DEBOUNCE_CYCLES && !minute_homed) {
        //         #ifdef DEBUG
        //         Serial.println("MH -> 1");
        //         #endif
        //         minute_homed = true;
        //     }
        // }
        // if (!minute_homed_input && minute_homed_debounce > -DEBOUNCE_CYCLES) {
        //     minute_homed_debounce--;
        //     if (minute_homed_debounce == -DEBOUNCE_CYCLES && minute_homed) {
        //         #ifdef DEBUG
        //         Serial.println("MH -> 0");
        //         #endif
        //         minute_homed = false;
        //     }
        // }
        // if (hour_homed_input && hour_homed_debounce < DEBOUNCE_CYCLES) {
        //     hour_homed_debounce++;
        //     if (hour_homed_debounce == DEBOUNCE_CYCLES && !hour_homed) {
        //         #ifdef DEBUG
        //         Serial.println("HH -> 1");
        //         #endif
        //         hour_homed = true;
        //     }
        // }
        // if (!hour_homed_input && hour_homed_debounce > -DEBOUNCE_CYCLES) {
        //     hour_homed_debounce--;
        //     if (hour_homed_debounce == -DEBOUNCE_CYCLES && hour_homed) {
        //         #ifdef DEBUG
        //         Serial.println("HH -> 0");
        //         #endif
        //         hour_homed = false;
        //     }
        // }

        if (minute_homed && hour_homed) {
            coil.home();
        }
        if (minute_homed && !hour_homed) {
            coil.minute_home();
        }

        update_minutes();

        if (coil.get_display_minutes() == -1 && minutes != -1) {
            // if pointer position is unknown and time is known -> advance
            Led::clock_sprinting = true;

            coil.advance_if_possible();
            
        
        } else if (coil.get_display_minutes() != -1 && minutes != -1) {
            // pointer position and time is known
            Led::clock_sprinting = false;

            int diff = modulo(minutes - coil.get_display_minutes(), 720);

            if (diff < 710 && diff > 0) {
                #ifdef DEBUG
                    bool advanced = coil.advance_if_possible();
                    if (advanced) {;
                        Serial.printf("dm = %d\n", coil.get_display_minutes());
                    }
                #else
                    coil.advance_if_possible();
                #endif
            } else {
                // just wait for 10 minutes
            }   
        } else {
            // pointer position known but time not known
            Led::clock_sprinting = false;
        }

    }

private:
    Coil coil;

    int minutes = -1;

    bool minute_homed = false;
    bool hour_homed = false;

    // int hour_homed_debounce = 0;
    // bool hour_homed = false;
    // int minute_homed_debounce = 0;
    // bool minute_homed = false;
    
    inline int modulo(int a, int b) {
        const int result = a % b;
        return result >= 0 ? result : result + b;
    }

    inline void update_minutes() {
        time_t epoch;
        time(&epoch);
        struct tm localTime;
        localtime_r(&epoch, &localTime);

        if (epoch > 31536000) {
            int new_minutes = (localTime.tm_hour * 60 + localTime.tm_min) % 720 - 55;
            if (new_minutes != minutes) {
                minutes = new_minutes;
                Serial.printf("%02d:%02d\n", localTime.tm_hour, localTime.tm_min);

                #ifdef DEBUG
                Serial.printf("m = %d\n", minutes);
                Serial.printf("dm = %d\n", coil.get_display_minutes());
                #endif
            }
            Led::time_known = true;
        } else {
            Led::time_known = false;
        }
    }


};

#endif // CLOCK_H_