#ifndef CLOCK_H_
#define CLOCK_H_

#include "coil.h"

#define MINUTE_HOME D6
#define HOUR_HOME D5

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
        bool minute_homed = digitalRead(MINUTE_HOME) == LOW;
        bool hour_homed = digitalRead(HOUR_HOME) == LOW;

        if (minute_homed && hour_homed) {
            coil.home();
        }
        if (minute_homed && !hour_homed) {
            coil.minute_home();
        }

        update_minutes();

        if (coil.get_display_minutes() == -1) {
            // if pointer position is unknown -> advance

            coil.advance_if_possible();
        
        } else if (minutes != -1) {
            // pointer position and time is known

            int diff = modulo(minutes - coil.get_display_minutes(), 720);

            if (diff < 710 && diff > 0) {
                #ifdef DEBUG
                    bool advanced = coil.advance_if_possible();
                    if (advanced) {
                        Serial.printf("dm = %d\n", coil.get_display_minutes());
                    }
                #else
                    coil.advance_if_possible();
                #endif
            } else {
                // just wait for 10 minutes
            }   
        }

    }

private:
    Coil coil;

    int minutes = -1;

    inline int modulo(int a, int b) {
        const int result = a % b;
        return result >= 0 ? result : result + b;
    }

    inline void update_minutes() {
        time_t epoch;
        time(&epoch);
        struct tm localTime;
        localtime_r(&epoch, &localTime);

        if (epoch > 42000) {
            int new_minutes = (localTime.tm_hour * 60 + localTime.tm_min) % 720;
            if (new_minutes != minutes) {
                minutes = new_minutes;
                Serial.printf("%02d:%02d\n", localTime.tm_hour, localTime.tm_min);

                #ifdef DEBUG
                Serial.printf("m = %d\n", minutes);
                Serial.printf("dm = %d\n", coil.get_display_minutes());
                #endif
            }
        }
    }


};

#endif // CLOCK_H_