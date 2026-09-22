#ifndef COIL_H_
#define COIL_H_

#include <Arduino.h>

#define COIL_POSITIVE D2
#define COIL_NEGATIVE D3
#define COIL_ACTIVATION_MS 50
#define MIN_COIL_INTERVAL_MS 200
#define HOME_MINUTES 75
#define MINUTE_CORRECTION_MAXDIFF 12

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

        inline int get_display_minutes() const {
            return display_minutes;
        }

        inline void home() {
            if (display_minutes != HOME_MINUTES) {
                Serial.println("Homed");
            }
            display_minutes = HOME_MINUTES;
        }

        inline void minute_home() {
            if (display_minutes != -1) {
                int diff = modulo(display_minutes - HOME_MINUTES, 60);

                if (diff > 0 && diff <= MINUTE_CORRECTION_MAXDIFF) {
                    display_minutes = modulo(display_minutes - diff, 720);
                    Serial.printf("Minute correction -%d\n", diff);
                } else if (diff >= 60 - MINUTE_CORRECTION_MAXDIFF) {
                    display_minutes = modulo(display_minutes + 60 - diff, 720);
                    Serial.printf("Minute correction +%d\n", 60 - diff);
                } else if (diff != 0) {
                    display_minutes = -1;
                    Serial.println("Minute correction failed -> dm invalidated");
                }
            }
        }

    private:
        // 1 if next pulse is to be positive, -1 if next pulse is to be negative
        int8_t polarity = 0;
        uint32_t last_advance = 0;
        int display_minutes = -1;

        inline int modulo(int a, int b) {
            const int result = a % b;
            return result >= 0 ? result : result + b;
        }

        inline void advance(const uint32_t* now) {
            #ifdef DEBUG
                if (polarity > 0) {
                    Serial.println("advance(+)");
                } else {
                    Serial.println("advance(-)");
                }
            #endif

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