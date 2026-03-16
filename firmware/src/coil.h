#ifndef COIL_H_
#define COIL_H_

#include <Arduino.h>
#include "defines.h"

#define COIL_ACTIVATION_MS 50
#define MIN_COIL_INTERVAL_MS 200
#define HOME_MINUTES 75

class Coil {
    public:
        Coil() {
            positive_pulse();
            delay(MIN_COIL_INTERVAL_MS);
            negative_pulse();
            delay(MIN_COIL_INTERVAL_MS);
            positive_pulse();
            delay(MIN_COIL_INTERVAL_MS);
            negative_pulse();
            polarity = 1;

            last_advance = millis();
        }

        inline void advance() {
            advance(NULL);
        }

        inline boolean advance_if_possible() {
            uint32_t now = millis();
            if (now - last_advance > MIN_COIL_INTERVAL_MS) {
                advance(&now);
                return true;
            } else {
                return false;
            }
        }

        inline int get_display_minutes() {
            return display_minutes;
        }

        inline void home() {
            display_minutes = HOME_MINUTES;
        }

    private:
        // 1 if next pulse is to be positive, -1 if next pulse is to be negative
        int8_t polarity = 0;
        uint32_t last_advance = 0;
        int display_minutes = -1;

        inline void advance(uint32_t* now) {
            if (polarity > 0) {
                positive_pulse();
            } else if (polarity < 0) {
                negative_pulse();
            }
            polarity = polarity * (-1);
            last_advance = now == NULL ? millis() : *now;

            if (display_minutes != -1) {
                display_minutes = (display_minutes + 1) % 720;
            }
        }

        inline void positive_pulse() {
            digitalWrite(COIL_POSITIVE, HIGH);
            delay(COIL_ACTIVATION_MS);
            digitalWrite(COIL_POSITIVE, LOW);
        }

        inline void negative_pulse() {
            digitalWrite(COIL_NEGATIVE, HIGH);
            delay(COIL_ACTIVATION_MS);
            digitalWrite(COIL_NEGATIVE, LOW);
        }
};

#endif //COIL_H_