#ifndef DEFINES_H_
#define DEFINES_H_

#define DEBUG
#define DEBUG_FINE
//#define DEBUG_FINEST
#define DEBUG_SYNC

#define DCF_IN D7
#define MINUTE_HOME D5
#define HOUR_HOME D6
#define COIL_POSITIVE D2
#define COIL_NEGATIVE D3

#define OVERFLOW_MINS 720
#define OVERFLOW_SECS 86400

inline int modulo(int a, int b) {
    const int result = a % b;
    return result >= 0 ? result : result + b;
}

#endif //DEFINES_H_