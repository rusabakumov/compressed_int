#include "cint.h"
#include "cint_internal.h"

#include <stdio.h>

static uint8_t buf[MAX_NUMLEN], buf2[MAX_NUMLEN];

char *cint_to_str(const cint_t initial, char *str) 
{
    /* Converts given cint to string with it's hex value */

    struct parsed_number pn = {initial};
    parse_number(&pn);

    cint_t num = initial;
    int startind = (pn.order < 0)? 1: 0;    //Saving sign of the original number
    if (pn.order < 0) {
        num = (char *)buf2;
        uint8_t *num_byte = (uint8_t *)num;
        uint8_t *initial_byte = (uint8_t *)initial;
        for (int i = 0; i < pn.total_len; i++) {
            *(num_byte++) = ~*(initial_byte++);
        }
        cint_sadd(num, 1);
        pn.num = num;
        parse_number(&pn);
    }

    memset((char *)buf, 0, pn.total_len);
    uint8_t *unpacked_num = buf;
    int size = unpack_cint(&pn, unpacked_num);

    uint8_t *unpacked_byte;

    //Building string representation
    int slen;
    if (startind == 1) {
        str[0] = '-';
    }
    str[startind] = '0';
    str[startind + 1] = 'x';
    slen = startind + 2;

    unpacked_byte = unpacked_num + size - 1;
    bool had_significant = false;
    for (int i = 0; i < size; i++) {
        if ((*unpacked_byte >> 4) != 0 || had_significant) {
            sprintf(str + slen, "%x", *unpacked_byte >> 4);
            slen = strlen(str);
            had_significant = true;
        }
        if ((*unpacked_byte & 0xf) != 0 || had_significant) {
            sprintf(str + slen, "%x", *unpacked_byte & 0xf);
            slen = strlen(str);
            had_significant = true;
        }
        unpacked_byte--;
    }
    if (!had_significant) {
        str[startind + 2] = '0';
        str[startind + 3] = '\0';
    }
    
    return str;
}
