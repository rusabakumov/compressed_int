#include "cint.h"
#include "cint_internal.h"

#include <math.h>
#include <assert.h>

/* Temporary buffer for calculations */
static uint8_t buf[MAX_NUMLEN], buf2[MAX_NUMLEN], buf3[MAX_NUMLEN];

/* neg ci lookup
 *  |11000000|
 *-------------
 *  |10111111|
 *  |11011111|
 *  |11101111|
 *  |11110111|
 *  |11111011|
 *  |11111101|
 *  |01111110|
 */

static
void pack_to_cint(uint8_t *a, int msb_ind, cint_t res, int negative) {
    /* Compressing number */
    uint8_t *a_byte = a;
    int counted_order = 0;

    assert(negative >=0 && negative <= 1);
    int l_ind = 1 - negative; //If number is positive, we should subtract numbers, add otherwise
    
    int shift = 0;
    int byte_ind;
    uint8_t cur_ci;
    for (byte_ind = 0; byte_ind < msb_ind - 1; byte_ind++) {
        cur_ci = ci_sign_lookup[l_ind][byte_ind % 7];
        if (byte_ind == 0 && negative == 0) {
            cur_ci = 0xc0;
        }
        shift += *a_byte + cur_ci;
        *(a_byte++) = shift & 0xff;
        shift >>= 8;

        counted_order++;
        if (byte_ind % 7 == 6) {
            counted_order++;
        }
    }

    uint8_t last_ci = ci_sign_lookup[l_ind][byte_ind % 7];
    if (negative == 0 && byte_ind == 0) {
        last_ci = 0xc0;
    }

    if (negative) {
        if (byte_ind % 7 == 6) {
            if (*a_byte + last_ci + shift > 0xff) {
                last_ci = 1;
            } else {
                counted_order++;
            }
        }

        if (*a_byte + last_ci + shift > 0xff) {
            last_ci = 0;
        } else {
            counted_order++;
        }
    } else {
        if (byte_ind % 7 == 6) {
            if (*a_byte + last_ci + shift < 0x100) {
                last_ci |= 0xc0;
            } else {
                counted_order++;
            }
        }

        if (*a_byte + last_ci + shift < 0x100) {
            last_ci = 0xff;
            if (byte_ind == 0) {
                last_ci = 0;
            }
        } else {
            counted_order++;
        }
    }
    *a_byte = (*a_byte + shift + last_ci) & 0xff;
    
    if (negative) {
        set_num(res, -counted_order - 1, a);
    } else {
        set_num(res, counted_order, a);
    }
}

#define num_min(a, b) (((a) < (b)) ? (a) : (b)) 

//********************************************************
//Interface functions implementation
//********************************************************

int cint_get_size(const cint_t a) {
    /* Returns total size of a number sting */
    /* Here some code from parse_number is repeated */    
    const uint8_t *body = (const uint8_t *)a;

    int total_len = 0;
    int positive = (*body >> 7) & 1;

    int bitlen;
    while ((bitlen = length_lookup[positive][*body]) == 8) {
        total_len += 8;
        body++;
    };
    total_len += bitlen;

    return total_len;
}

int cint_is_even(const cint_t a) {
    /* 1 if even, 0 otherwise */
    int len = cint_get_size(a);
    return a[len - 1] % 2 == 0;
}

int cint_cmp(const cint_t a, const cint_t b) {
    /* Same as memcmp - positive number if a greater than b, negative if a less than b, 0 if equal */
    int len1 = cint_get_size(a);
    int len2 = cint_get_size(b);
    return memcmp(a, b, num_min(len1, len2));
}

void cint_sadd(cint_t a, int8_t b) {
    /* Adding a small constant to the given compressed number in place. It should not exceed 60, otherwise correctness not guaranteed */
    struct parsed_number pa = {a};
    parse_number(&pa);
    /* Z = A + B; z_i = a_i + b_i mod 2^8 */

    /* resulting number, lsb at the beginning */
    memset((char *)buf, 0, pa.total_len);
    uint8_t *z_init = buf; 
    uint8_t *z = z_init; 

    /* source number, lsb at the end */
    uint8_t *a_byte = (uint8_t *)pa.num + pa.total_len - 1; 

    int current_byte;
    int rest = b;

    while (a_byte != pa.last_byte_ptr) {
        current_byte = *(a_byte--) + rest;
        *(z++) = (uint8_t) (current_byte & 0xff);
        rest = current_byte >> 8;
    };

    current_byte = pa.last_byte + rest;
    int overflow_rest = current_byte >> pa.msb_len;
    //current_byte ^= (1 << n.msb_len);
    z[0] = current_byte & ((1 << pa.msb_len) - 1);
    z[1] = (current_byte >> 8) & 0xff;

    if (overflow_rest == 1) {
        set_num(a, pa.order + 1, z_init);
    } else if (overflow_rest == -1) {
        z[0] |= (0xff << pa.msb_len);
        set_num(a, pa.order - 1, z_init);
    } else if (overflow_rest == 0) {
        set_num(a, pa.order, z_init);
    }
}

static
void count_next_shift(int byte_ind, parsed_number *pn, 
                      uint8_t *cur_byte, int *counted_order, int *shift,
                      uint8_t fill, uint8_t ci_fill) {
    uint8_t cur_ci = ci_sign_lookup[pn->negative][byte_ind % 7];
    if (pn->negative == 1 && byte_ind == 0) {
        cur_ci = 0xc0;
    }
    /* Unpacking next byte of number and adding it's value to the shift */

    if (byte_ind < pn->num_len - 1) {
        *shift += *cur_byte + cur_ci;
        (*counted_order)++;
        if (byte_ind % 7 == 6) {
            (*counted_order)++;
        }
    }
    else if (byte_ind == pn->num_len - 1) {
        if (abs(pn->order) - *counted_order == 0)  {
            cur_ci = ci_fill;
        } else if (abs(pn->order) - *counted_order == 1)  {
            if (pn->negative == 1) {
                cur_ci |= 0x80;
            } else {
                cur_ci &= 0x7f;
            }
        }
        uint8_t msb_fill = (-1 << pn->msb_len) & fill;
        *shift += (pn->last_byte | msb_fill) + cur_ci;
    } else {
        *shift += fill + ci_fill;
    }
}

void cint_add(const cint_t a, const cint_t b, cint_t res) {
    /* Just make an addition */
    struct parsed_number pa = {a};
    parse_number(&pa);
    uint8_t *a_byte = (uint8_t *)pa.num + pa.total_len - 1;

    struct parsed_number pb = {b};
    parse_number(&pb);
    uint8_t *b_byte = (uint8_t *)pb.num + pb.total_len - 1;

    int total_len;
    if (pa.total_len > pb.total_len) {
        total_len = pa.total_len + 2;
    } else {
        total_len = pb.total_len + 2;
    }
    
    //Filling bytes for the compressed values
    uint8_t a_fill = (pa.order < 0) ? 0xff : 0;
    uint8_t b_fill = (pb.order < 0) ? 0xff : 0;
    //Filling bytes for their compression parts
    uint8_t a_ci_fill = (pa.order < -1) ? 0xff : 0;
    uint8_t b_ci_fill = (pb.order < -1) ? 0xff : 0;
    
    uint8_t *res_init = buf;
    uint8_t *res_byte = res_init;

    int a_counted_order = 0;
    if (pa.negative == 1) {
        a_counted_order++;
    }
    
    int b_counted_order = 0;
    if (pb.negative == 1) {
        b_counted_order++;
    }
    
    int shift = 0;
    int byte_ind;
    for (byte_ind = 0; byte_ind < total_len; byte_ind++, a_byte--, b_byte--) {
        count_next_shift(byte_ind, &pa, a_byte,
            &a_counted_order, &shift, a_fill, a_ci_fill);
        count_next_shift(byte_ind, &pb, b_byte,
            &b_counted_order, &shift, b_fill, b_ci_fill);
    
        //Setting result byte
        *(res_byte++) = shift & 0xff;
        shift >>= 8;
    }
    
    int unpacked_size = byte_ind;
    int negative = (res_init[unpacked_size - 1] & 0x80) ? 1 : 0;

    while (((!negative && res_init[unpacked_size - 1] == 0) || (negative && res_init[unpacked_size - 1] == 0xff)) && unpacked_size > 1) {
        unpacked_size--;
    }

    pack_to_cint(res_init, unpacked_size, res, negative);
}

void cint_neg(cint_t num) {
    parsed_number pn = {num};
    parse_number(&pn);
    for (int i = 0; i < pn.total_len; i++) {
        num[i] = ~num[i];
    }
    cint_sadd(num, 1);
}

void cint_subtract(const cint_t a, const cint_t b, cint_t res) {
    cint_t tmp = (char *)buf2;
    memcpy(tmp, b, cint_get_size(b));
    cint_neg(tmp);
    cint_add(a, tmp, res);
}

void cint_between(const cint_t a, const cint_t b, cint_t res) {
    /* Counts (a + b) / 2 */
    struct parsed_number pa = {a};
    parse_number(&pa);
    uint8_t *a_byte = (uint8_t *)pa.num + pa.total_len - 1;

    struct parsed_number pb = {b};
    parse_number(&pb);
    uint8_t *b_byte = (uint8_t *)pb.num + pb.total_len - 1;

    int total_len;
    if (pa.total_len > pb.total_len) {
        total_len = pa.total_len + 2;
    } else {
        total_len = pb.total_len + 2;
    }
    
    //Filling bytes for the compressed values
    uint8_t a_fill = (pa.order < 0) ? 0xff : 0;
    uint8_t b_fill = (pb.order < 0) ? 0xff : 0;
    //Filling bytes for their compression parts
    uint8_t a_ci_fill = (pa.order < -1) ? 0xff : 0;
    uint8_t b_ci_fill = (pb.order < -1) ? 0xff : 0;
    
    uint8_t *res_init = buf;
    uint8_t *res_byte = res_init;

    int a_counted_order = 0;
    if (pa.negative == 1) {
        a_counted_order++;
    }
    
    int b_counted_order = 0;
    if (pb.negative == 1) {
        b_counted_order++;
    }
    
    int shift = 0;
    int byte_ind;
    for (byte_ind = 0; byte_ind < total_len; byte_ind++, a_byte--, b_byte--) {
        count_next_shift(byte_ind, &pa, a_byte,
            &a_counted_order, &shift, a_fill, a_ci_fill);
        count_next_shift(byte_ind, &pb, b_byte,
            &b_counted_order, &shift, b_fill, b_ci_fill);
    
        //Setting result byte
        *(res_byte++) = shift & 0xff;
        shift >>= 8;
    }
    
    int unpacked_size = byte_ind;
    //Getting median - shifting one position right

    //If resulting number is negative, we should have 1 in shift
    shift = (res_init[unpacked_size - 1] & 0x80) ? 1 : 0;
    int negative = shift;

    int next_shift;
    do {
        res_byte--;
        next_shift = *res_byte & 1;
        *res_byte >>= 1;
        *res_byte |= 0x80 * shift;
        shift = next_shift;
    } while (res_byte != res_init);

    while (((!negative && res_init[unpacked_size - 1] == 0) || (negative && res_init[unpacked_size - 1] == 0xff)) && unpacked_size > 1) {
        unpacked_size--;
    }

    pack_to_cint(res_init, unpacked_size, res, negative);
}

void cint_mul(const cint_t a, const cint_t b, cint_t res) {
    /* Just multiplication */
    struct parsed_number pa = {a};
    parse_number(&pa);

    struct parsed_number pb = {b};
    parse_number(&pb);

    //Unpacking a and b once, instead of every iteration
    uint8_t *a_unpacked = buf2;
    int a_len = unpack_cint(&pa, a_unpacked);

    uint8_t *b_unpacked = buf3;
    int b_len = unpack_cint(&pb, b_unpacked);

    uint8_t *res_buf = buf;
    memset(res_buf, 0, a_len + b_len + 1);

    int i, j, shift;
    uint8_t *a_byte = a_unpacked;
    for (i = 0; i < a_len; i++, a_byte++) {
        uint8_t *b_byte = b_unpacked;
        uint8_t *res_byte = res_buf + i;
        shift = 0;
        for (j = 0; j < b_len; j++, b_byte++, res_byte++) {
            shift += *a_byte * *b_byte;
            shift += *res_byte;
            *res_byte = shift & 0xff;
            shift >>= 8;
        }
        while (shift != 0) {
            shift += *res_byte;
            *(res_byte++) = shift & 0xff;
            shift >>= 8;
        }
    }

    int result_size = a_len + b_len + 1;
    while (res_buf[result_size - 1] == 0 && result_size > 1) {
        result_size--;
    }

    if (pa.negative ^ pb.negative && !(result_size == 1 && res_buf[0] == 0)) {
        //Inverting result, if it should be negative
        shift = 1;
        uint8_t *res_byte = res_buf;
        for (i = 0; i < result_size; i++, res_byte++) {
            shift += (uint8_t)~(*res_byte);
            *res_byte = shift & 0xff;
            shift >>= 8;
        }
        if (shift != 0) {
            res_buf[result_size] = shift & 0xff;
            result_size++;
        }
        res_buf[result_size] = 0xff;
        pack_to_cint(res_buf, result_size, res, 1);
    } else {
        res_buf[result_size] = 0;
        pack_to_cint(res_buf, result_size, res, 0);
    }
}

int cint_sdiv(const cint_t a, unsigned int b, cint_t quot = NULL) {
    /* Returns remainder a%b and, if quot is given, setting it as a/b. Operations are not splitted, because they are practically the same and it's inefficient to call them twice to calculate both quotient and remainder */
    struct parsed_number pa = {a};
    parse_number(&pa);

    //Unpacking a and b once, instead of every iteration
    uint8_t *a_unpacked = buf;
    int a_len = unpack_cint(&pa, a_unpacked);

    uint8_t *a_byte = a_unpacked + a_len - 1;

    int remainder = 0;
    while (a_byte >= a_unpacked) {
        remainder = (remainder << 8) + *a_byte;
        *(a_byte--) = remainder / b;
        remainder %= b;
    }

    if (quot != NULL) {
        int unpacked_size = a_len;
        while (a_unpacked[unpacked_size - 1] == 0 && unpacked_size > 1) {
            unpacked_size--;
        }

        if (pa.negative && !(unpacked_size == 1 && a_unpacked[0] == 0)) {
            //Inverting result, if it should be negative
            int shift = 1, i;
            uint8_t *a_byte = a_unpacked;
            for (i = 0; i < unpacked_size; i++, a_byte++) {
                shift += (uint8_t)~(*a_byte);
                *a_byte = shift & 0xff;
                shift >>= 8;
            }
            if (shift != 0) {
                a_unpacked[unpacked_size] = shift & 0xff;
                unpacked_size++;
            }
            a_unpacked[unpacked_size] = 0xff;
            pack_to_cint(a_unpacked, unpacked_size, quot, 1);
        } else {
            a_unpacked[unpacked_size] = 0;
            pack_to_cint(a_unpacked, unpacked_size, quot, 0);
        }
    }
    if (pa.negative) {
        return -remainder;
    } else {
        return remainder;
    }
}

void cint_assign(cint_t res, int x) {
    /* Initializing given cint with x */
    uint8_t *num = buf;
    uint8_t *num_byte = num;
    
    int negative = (x >= 0)? 0 : 1;
    
    for (unsigned int i = 0; i < sizeof(int); i++) {
        *(num_byte++) = x & 0xff;
        x >>= 8;
    }
    int unpacked_size = sizeof(int);
    
    while (((!negative && num[unpacked_size - 1] == 0) || (negative && num[unpacked_size - 1] == 0xff)) && unpacked_size > 1) {
        unpacked_size--;
    }
    pack_to_cint(num, unpacked_size, res, negative);
}

static uint64_t partitions[9] = {
    0l,
    0l,
    64l,
    8256l,
    1056832l,
    135274560l,
    17315143744l,
    2216338399296l,
    283691315109952l
};

uint8_t masks[9] = {0, 128, 192, 224, 240, 248, 252, 254, 255};

int cint_decompress(const cint_t x, int *value) {
    /* Returns 1 and setting res as the value of x if decompression succesfull, return 0 otherwise (value of res can be too large for int type) */
    unsigned char *id = (unsigned char *)x;
    //Reading value at place and shifting to the next byte
    uint8_t cur_id = *(id++);

    //Determining sign of the value
    int negative = 1;
    if (cur_id & 128) {
        negative = 0;
    } else {
        cur_id = ~cur_id;
    }

    uint32_t size = length_lookup[1][cur_id];
    if (size > sizeof(int)) {
        return 0;
    }
    cur_id &= ~masks[size];

    uint64_t num = cur_id;

    //Reading number to the end
    for (unsigned int i = 1; i < size; i++) {
        num <<= 8;
        cur_id = *(id++);
        if (negative) {
            cur_id = ~cur_id;
        }
        num |= cur_id;
    }
    
    *value = num;
    *value += partitions[size];
    if (negative) {
        *value += 1;
        *value *= -1;
    }
    return 1;
}

