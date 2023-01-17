#include <iostream>
#include <ws2tcpip.h>
#include "inetpton4.h"
#define NS_INADDRSZ  4

int inet_pton4(const char *src, char *dst){
    uint8_t tmp[NS_INADDRSZ], *tp;
    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;
    int ch;

    while ((ch = *src++) != '\0'){
        if (ch >= '0' && ch <= '9'){
            uint32_t n = *tp * 10 + (ch - '0');
            if (saw_digit && *tp == 0){
                return 0;
            }
            if (n > 255) {
                return 0;
            }

            *tp = n;
            if (!saw_digit){
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit){
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else {
            return 0;
        }
    }
    if (octets < 4){
        return 0;
    }
    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}
