#include "Arduino.h"
#include <ESP8266WiFi.h>

int next_second = -1;
int display_seconds = -1;
int clock_seconds = -1;

uint32_t up_millis;
boolean up_millis_pending = false;
uint32_t down_millis;
boolean down_millis_pending = false;

uint32_t timer1_at_flank_up = 0;

volatile uint32_t timer_read = 0;
void IRAM_ATTR inputLevelChangedISR() {
    int v = digitalRead(D7);
    if (v) {
        up_millis = millis();
        up_millis_pending = true;
        if (timer1_enabled()) {
            timer1_at_flank_up = timer1_read();
        }
    } else {
        down_millis = millis();
        down_millis_pending = true;
    }
}

volatile bool sec_fired = false;
void IRAM_ATTR timerFiredISR() {
    if (timer1_at_flank_up > 100 && timer1_at_flank_up < 5000000 - 100) {
        if (timer1_at_flank_up < 2500000) {
            // timer fired too late -> make next timer period shorter
            timer1_write(5000000 - timer1_at_flank_up);
        } else {
            // timer fired too early -> make next timer period longer
            timer1_write(5000000 + (5000000 - timer1_at_flank_up));
        }
    } else {
        timer1_write(5000000);
    }
    
    if (next_second != -1) {
        clock_seconds = next_second;
        next_second = -1;
    } else if (clock_seconds != -1) {
        clock_seconds = (clock_seconds + 1) % 86400;
    }
    
    sec_fired = true;
}

void setup() {
    WiFi.mode(WIFI_OFF);

    noInterrupts();
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(5000000);
    timer1_attachInterrupt(timerFiredISR);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(D7, INPUT);

    attachInterrupt(digitalPinToInterrupt(D7), inputLevelChangedISR, CHANGE);

    interrupts();
    Serial.begin(9600);
}

int cursor = 0;
int buffer[59];
unsigned long last_bit_rx_time;

bool parity_check(int begin, int end) {
    int sum = 0;
    for (int i = begin; i <= end; i++) {
        sum += buffer[i];
    }
    if ((sum % 2) != 0) {
        Serial.println("Parity check failed.");
    }
    return (sum % 2) == 0;
}

void interprete() {
    int min_one = buffer[21] | 
              (buffer[22]<<1) |
              (buffer[23]<<2) |
              (buffer[24]<<3);
    int min_ten = buffer[25] |
                  (buffer[26]<<1) |
                  (buffer[27]<<2);
    boolean checks = true;
    checks = checks && parity_check(21, 28);

    int hour_one = buffer[29] | 
              (buffer[30]<<1) |
              (buffer[31]<<2) |
              (buffer[32]<<3);
    int hour_ten = buffer[33] |
                  (buffer[34]<<1);
    checks = checks && parity_check(29, 35);

    int day_one = buffer[36] | 
              (buffer[37]<<1) |
              (buffer[38]<<2) |
              (buffer[39]<<3);
    int day_ten = buffer[40] |
                  (buffer[41]<<1);

    int month_one = buffer[45] | 
              (buffer[46]<<1) |
              (buffer[47]<<2) |
              (buffer[48]<<3);
    int month_ten = buffer[49];

    int year_one = buffer[50] | 
              (buffer[51]<<1) |
              (buffer[52]<<2) |
              (buffer[53]<<3);
    int year_ten = buffer[54] |
                  (buffer[55]<<1) |
                  (buffer[56]<<2) |
                  (buffer[57]<<3);

    int dow = buffer[42] | (buffer[43]<<1) | (buffer[44]<<2);
    checks = checks && parity_check(36, 58);

    
    Serial.printf("%d-%d-%d %d:%d:XX, dow = %d\n", 
        year_one + year_ten*10,
        month_one + month_ten*10,
        day_one + day_ten*10,
        hour_one + hour_ten*10,
        min_one + min_ten*10,
        dow
    );
    if (checks) {
        next_second = hour_one * 3600 + hour_ten * 36000 + 
                      min_one * 60 + min_ten * 600
                      - 1;
    }
}

void next(uint32_t rx_time, uint32_t signal_duration) {
    int bit = -1;
    if (signal_duration > 55 && signal_duration < 145) {
        bit = 0;
    }
    if (signal_duration > 155 && signal_duration < 245) {
        bit = 1;
    }
    if (bit != -1) {
        unsigned long rx_time_diff = rx_time - last_bit_rx_time;
        last_bit_rx_time = rx_time;

        if (((rx_time_diff > 1800) && (rx_time_diff < 2200)) || cursor > 58) {
            cursor = 0;
        }

        buffer[cursor++] = bit;
        if (clock_seconds == -1) {
            Serial.printf("buf[%d] = %d\n", cursor - 1, bit);
        }
        // Serial.println("-----------------");
        // Serial.printf("delta_t = %d; sig_duration = %d; buffer[%d] = %d\n", rx_time_diff, signal_duration, cursor - 1, bit);
        // Serial.printf("clock_seconds=%d\n", clock_seconds);
        // Serial.printf("timer1_at_flank_up=%d\n", timer1_at_flank_up);

        if (cursor == 59) {
            interprete();
        }
    }
}

int level = 0;
unsigned long up_time = 0;

void loop() {

    if (up_millis_pending && down_millis_pending) {
        uint32_t up_time = down_millis - up_millis;
        down_millis_pending = false;
        up_millis_pending = false;
        next(up_millis, up_time);
    }

    if (sec_fired) {
        sec_fired = false;
        if (clock_seconds != -1) {
            Serial.printf("%02d:%02d:%02d\n", 
                clock_seconds / 3600,
                (clock_seconds / 60) % 60,
                clock_seconds % 60
            );
        } else {
            Serial.println("??:??:??");
        }
    }

}