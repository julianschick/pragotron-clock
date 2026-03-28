#ifndef DECODER_H_
#define DECODER_H_

#include <cstddef>
#include "time.h"
#include "sync.h"

#define MINIMAL_ZERO_TIME 50
#define MAXIMAL_ZERO_TIME 145
#define MINIMAL_ONE_TIME 146
#define MAXIMAL_ONE_TIME 250

#define MINIMAL_MINUTE_PAUSE_TIME 1900
#define MAXIMAL_MINUTE_PAUSE_TIME 2100

#define MAX_SUSPICIOUS_TIME_FRAMES 9
#define SUSPICIOUS_SECOND_DIFF_THRS 5

class Decoder {
    public:
        Decoder(Sync* sync) : sync(sync) { };

        void next(uint32_t rx_time, uint32_t signal_duration);

    private:
        Sync* sync;

        int cursor = 0;
        bool invalid_bit = false;
        uint8_t buffer[8];
        unsigned long last_bit_rx_time;
        //
        int suspicious_time_counter = 0;
        
        Time decode_buffer();
        static bool parity_check(const uint8_t* buffer, const int begin, const int end);
};


#endif //DECODER_H_