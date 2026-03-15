#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "decode.h"

#define DEBUG
//#define DEBUG_FINE
//#define DEBUG_FINEST

#define DCF_IN D7
#define MINUTE_HOME D5
#define HOUR_HOME D6
#define COIL_POSITIVE D2
#define COIL_NEGATIVE D3

#define TIM1_SECOND     5000000
#define TIM1_HALFSECOND 2500000
#define TIM1_DIVIDER TIM_DIV16

#define COIL_ACTIVATION_MS 50
#define MIN_COIL_INTERVAL_MS 250

#define HOME_MINUTES 75

#define ACCEPTED_SIGNAL_DURATION_DEVIATION_MS 45
#define ACCEPTED_MINUTE_SIGNAL_DURATION_DEVIATION_MS 200

#define UNACCEPTED_TELEGRAMS 15

int next_second = -1;
int clock_seconds = -1;
//
int clock_minutes = -1;
int display_minutes = -1;

uint32_t up_millis;
boolean up_millis_pending = false;
uint32_t down_millis;
boolean down_millis_pending = false;

uint8_t unsynced_counter = 255;
uint32_t timer1_at_flank_up = 0;

bool is_home_position();
void fix_polarity();

volatile uint32_t timer_read = 0;
void IRAM_ATTR inputLevelChangedISR() {
    int v = digitalRead(DCF_IN);
    if (v) {
        up_millis = millis();
        down_millis_pending = false;
        up_millis_pending = true;
        if (timer1_enabled()) {
            timer1_at_flank_up = timer1_read();
        }
    } else {
        down_millis = millis();
        down_millis_pending = true;
    }
}

uint32_t last_timer_max = TIM1_SECOND;

volatile bool sec_fired = false;
void IRAM_ATTR timerFiredISR() {
    int8_t sign = timer1_at_flank_up < TIM1_HALFSECOND ? -1 : 1;
    uint32_t diff = sign == -1 ? 
        timer1_at_flank_up : last_timer_max - timer1_at_flank_up;

    if (diff > 100000 && unsynced_counter < 255) {
        unsynced_counter++;
    }

    if (diff > 100 && (diff <= 100000 || unsynced_counter >= 15)) {
        if (sign == -1) {
            // timer fired too late -> make next timer period shorter
            last_timer_max = TIM1_SECOND - diff;
        } else {
            // timer fired too early -> make next timer period longer
            last_timer_max = TIM1_SECOND + diff;
        }
        unsynced_counter = 0;
    } else {
        last_timer_max = TIM1_SECOND;
    }
    timer1_write(last_timer_max);
    
    if (next_second != -1) {
        clock_seconds = next_second;
        next_second = -1;
    } else if (clock_seconds != -1) {
        clock_seconds = (clock_seconds + 1) % 86400;
    }
    
    sec_fired = true;

    #ifdef DEBUG_FINEST
    Serial.printf("@up = %d, D = %d*%d, [[%d]]\n", 
        timer1_at_flank_up,
        sign, diff,
        unsynced_counter
    );
    #endif
}

void setup() {
    WiFi.mode(WIFI_OFF);

    noInterrupts();
    timer1_enable(TIM1_DIVIDER, TIM_EDGE, TIM_SINGLE);
    timer1_write(5000000);
    timer1_attachInterrupt(timerFiredISR);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    {
        digitalWrite(COIL_POSITIVE, LOW);
        pinMode(COIL_POSITIVE, OUTPUT);
        digitalWrite(COIL_POSITIVE, LOW);
    }
    {
        digitalWrite(COIL_NEGATIVE, LOW);
        pinMode(COIL_NEGATIVE, OUTPUT);
        digitalWrite(COIL_NEGATIVE, LOW);
    }

    pinMode(DCF_IN, INPUT);
    pinMode(MINUTE_HOME, INPUT);
    pinMode(HOUR_HOME, INPUT);

    attachInterrupt(digitalPinToInterrupt(DCF_IN), inputLevelChangedISR, CHANGE);

    
    Serial.begin(9600);

    delay(100);
    fix_polarity();

    interrupts();
}

int cursor = 0;
boolean invalid_bit = false;
uint8_t buffer[8];
unsigned long last_bit_rx_time;
//
Time* last_accepted_time = NULL;
int minute_diff_to_last_accepted = 1;

void next(uint32_t rx_time, uint32_t signal_duration) {
    //Serial.printf("next: sig_dur = %d\n", signal_duration);

    int bit = -1;
    if (signal_duration > 100 - ACCEPTED_SIGNAL_DURATION_DEVIATION_MS && 
        signal_duration < 100 + ACCEPTED_SIGNAL_DURATION_DEVIATION_MS) {
        bit = 0;
    }
    if (signal_duration > 200 - ACCEPTED_SIGNAL_DURATION_DEVIATION_MS && 
        signal_duration < 200 + ACCEPTED_SIGNAL_DURATION_DEVIATION_MS) {
        bit = 1;
    }

    if (bit == -1) {
        invalid_bit = true;
    } else {
        unsigned long rx_time_diff = rx_time - last_bit_rx_time;
        last_bit_rx_time = rx_time;

        if (cursor >= 59 || 
            ((rx_time_diff > 2000 - ACCEPTED_MINUTE_SIGNAL_DURATION_DEVIATION_MS) && 
             (rx_time_diff < 2000 + ACCEPTED_MINUTE_SIGNAL_DURATION_DEVIATION_MS))) {
            cursor = 0;
        }

        buffer[cursor / 8] = (bit == 1)
            ? buffer[cursor / 8] | (0x01 << (cursor % 8))
            : buffer[cursor / 8] & (~(0x01 << (cursor % 8)));
        cursor++;

        #ifdef DEBUG_FINE
        if (clock_minutes == -1) {
            Serial.printf("buf[%d] = %d\n", cursor - 1, bit);
        }
        #endif

        #ifdef DEBUG_FINEST
        Serial.println("-----------------");
        Serial.printf("delta_t = %d; sig_duration = %d; buffer[%d] = %d\n", rx_time_diff, signal_duration, cursor - 1, bit);
        Serial.printf("clock_seconds=%d\n", clock_seconds);
        Serial.printf("timer1_at_flank_up=%d\n", timer1_at_flank_up);
        #endif

        if (cursor == 59) {
            Time* t = Time::decode_telegram(buffer);

            boolean accept = t->parity_error_count == 0 && !invalid_bit &&
                (last_accepted_time == NULL || last_accepted_time->is_timewise_succ(t, minute_diff_to_last_accepted));

            invalid_bit = false;

            if (!accept && minute_diff_to_last_accepted > UNACCEPTED_TELEGRAMS) {
                accept = true;
            } else if (!accept) {
                minute_diff_to_last_accepted++;
            }

            if (accept) {
                if (last_accepted_time != NULL) {
                    delete last_accepted_time;
                }
                last_accepted_time = t;
                next_second = 3600*t->hour + 60*t->minute - 1;
                minute_diff_to_last_accepted = 1;
            }

            #ifdef DEBUG
            if (!accept) {
                Serial.printf("[NOT ACCEPTED] %02d-%02d-%02d %02d:%02d %s, dow = %d, err_count=%d, minute_diff_to_last_accepted=%d\n", 
                    t->year, t->month, t->day, t->hour, t->minute, t->summer_time ? "MESZ" : "MEZ", t->dow, t->parity_error_count, minute_diff_to_last_accepted
                );
            } else {
                Serial.printf("%02d-%02d-%02d %02d:%02d %s, dow = %d, err_count=%d, minute_diff_to_last_accepted=%d\n", 
                    t->year, t->month, t->day, t->hour, t->minute, t->summer_time ? "MESZ" : "MEZ", t->dow, t->parity_error_count, minute_diff_to_last_accepted
                );
            }
            Serial.flush();
            #endif

            if (!accept) {
                delete t;
            }
        }
    }
}

unsigned long last_advance = 0;

// 1 if next pulse is to be positive, -1 if next pulse is to be negative
int8_t polarity = 0;

void positive_pulse();
void negative_pulse();

void fix_polarity() {
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

void positive_pulse() {
    digitalWrite(COIL_POSITIVE, HIGH);
    delay(COIL_ACTIVATION_MS);
    digitalWrite(COIL_POSITIVE, LOW);
}

void negative_pulse() {
    digitalWrite(COIL_NEGATIVE, HIGH);
    delay(COIL_ACTIVATION_MS);
    digitalWrite(COIL_NEGATIVE, LOW);
}

void advance() {
    if (polarity > 0) {
        positive_pulse();
    } else if (polarity < 0) {
        negative_pulse();
    }
    polarity = polarity * (-1);
}

bool is_home_position() {
    return digitalRead(MINUTE_HOME) == LOW && digitalRead(HOUR_HOME) == LOW;
}

// 0 = homing
// 1 = homed
int state = 0;

void loop() {

    if (state == 0) {
        unsigned long now = millis();
        if (now - last_advance > MIN_COIL_INTERVAL_MS) {
            advance();
            last_advance = now;
        }
        if (is_home_position()) {
            state = 1;
            display_minutes = HOME_MINUTES;
        }
    } else if (state == 1) {
        if (clock_minutes != -1 && display_minutes != clock_minutes) {
            unsigned long now = millis();
            if (now - last_advance > MIN_COIL_INTERVAL_MS) {
                advance();
                last_advance = now;
                display_minutes = (display_minutes + 1) % 720;

                #ifdef DEBUG_FINE
                Serial.printf("dm = %d\n", display_minutes);
                #endif
            }
        }
    }

    if (up_millis_pending && down_millis_pending) {
        uint32_t up_time = down_millis - up_millis;
        down_millis_pending = false;
        up_millis_pending = false;
        next(up_millis, up_time);
    }

    if (sec_fired) {
        sec_fired = false;
        
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

                        advance();
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