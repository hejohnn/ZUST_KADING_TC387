#include "base64.h"

void base64_encode(uint8_t* input, char* output, uint32_t len)
{
    static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint32_t i, j;
    for(i = 0, j = 0; i < len; i += 3)
    {
        uint32_t octet_a = i < len ? input[i] : 0;
        uint32_t octet_b = (i + 1) < len ? input[i + 1] : 0;
        uint32_t octet_c = (i + 2) < len ? input[i + 2] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = base64_table[(triple >> 18) & 0x3F];
        output[j++] = base64_table[(triple >> 12) & 0x3F];
        output[j++] = (i + 1) < len ? base64_table[(triple >> 6) & 0x3F] : '=';
        output[j++] = (i + 2) < len ? base64_table[triple & 0x3F] : '=';
    }
    output[j] = '\0';
}
