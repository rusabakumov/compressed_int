#ifndef _CINT_INTERNAL_H_
#define _CINT_INTERNAL_H_

#include <stdlib.h>
#include <string.h>

/* Length of the number representation, according to the mask of the byte. For positive and negative numbers */
static const
int length_lookup[2][256] = {
    {8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 
    3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8}
};

/* Byte values for compressed parts of orders. They are looped. */
static
uint8_t ci_sign_lookup[2][7] = {{0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x81},
                                {0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e}};

struct parsed_number {
    const char *num;

    int total_len; /* Total byte length of number string */
    int num_len; /* Number of significant bytes */
    uint8_t msb_len; /* Number of significant bits in last byte */
    int order; /* Length of the number with respect to sign */
    int negative; /* If negative then 1, 0 otherwise */

    uint8_t last_byte; /* Actual value of last significant byte (without a mask) */
    const uint8_t *last_byte_ptr; /* Pointer to the last significant byte */
};

inline static 
struct parsed_number *parse_number(struct parsed_number *n) {
    /* Extracts significant information from compressed representation */
    const uint8_t *body = (const uint8_t *) n->num;
      
    int total_len = 0;
    int positive = (*body >> 7) & 1;

    int bitlen;
    int unsignificant_len = 0;

    while ((bitlen = length_lookup[positive][*body]) == 8) {
        total_len += 8;
        body++;
        unsignificant_len++;
    };
    total_len += bitlen;

    uint8_t last_byte = *body;
    n->total_len = total_len;
    n->num_len = total_len - unsignificant_len;
    n->msb_len = 8 - bitlen - 1;
    n->last_byte_ptr = (const uint8_t *)body;
    n->last_byte = last_byte & ((1 << n->msb_len) - 1);

    n->order = positive ? total_len - 1 : -total_len;
    n->negative = 1 - positive;

    return n;
}

inline static
void set_num(cint_t num, int z_order, uint8_t *value) {
    /* Writing compressed number to it's destination and setting correct mask */
    int left_fill = 0xff;

    int total_len = (z_order >= 0) ? z_order + 1 : -z_order;
    int msb = total_len / 8;
    int msb_len = 7 - total_len % 8;
    uint8_t msb_mask = (0xff << msb_len) & 0xff;
    uint8_t msb_c = (msb_mask << 1) & 0xff;

    if (z_order < 0) {
        msb_c = 1 << msb_len;
        left_fill = 0;
    }

    memset((char *)num, left_fill, msb);

    uint8_t *x = (uint8_t *)num + total_len - 1;
    uint8_t *msb_msb = (uint8_t *)num + msb;

    while (x != msb_msb) {
        *(x--) = *(value++);
    }

    *msb_msb = (*value & ~msb_mask) | msb_c;
}

inline static
int unpack_next_byte(int byte_ind, parsed_number *pn, 
                      uint8_t cur_byte, int *counted_order)
{
    if (pn->negative) {
        cur_byte = ~cur_byte;    
    }
    uint8_t cur_ci = ci_sign_lookup[0][byte_ind % 7];

    int result;
    if (byte_ind < pn->num_len - 1) {
        result = cur_byte + cur_ci;
        (*counted_order)++;
        if (byte_ind % 7 == 6) {
            (*counted_order)++;
        }
    }
    else if (byte_ind == pn->num_len - 1) {
        if (pn->order - *counted_order == 0)  {
            cur_ci = 0;
        } else if (pn->order - *counted_order == 1)  {
            cur_ci &= 0x7f;
        }
        result = pn->last_byte + cur_ci;
    } else {
       result = 0;
    }
    return result;
}

static
int unpack_cint(parsed_number *pn, uint8_t *unpacked) {
    /* Unpackes given cint_t and inverts it, if it is negative */
    int counted_order = 0;
    int shift = 0;
    if (pn->negative) {
        shift = 1;
        pn->order = -pn->order - 1;
        pn->last_byte = ~pn->last_byte & ((1 << pn->msb_len) - 1);
    }
    int len = 0, i;
    uint8_t *cur_byte = (uint8_t *)pn->num + pn->total_len - 1;
    for (i = 0; i < pn->total_len; i++, cur_byte--) {
        shift += unpack_next_byte(i, pn, *cur_byte, &counted_order);
        unpacked[len++] = shift & 0xff;
        shift >>= 8;
    }
    while (shift != 0) {
        unpacked[len++] = shift & 0xff;    
        shift >>= 8;
    }
    return len;
}

#endif /* _CINT_INTERNAL_H_ */
