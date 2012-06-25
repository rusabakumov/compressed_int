#ifndef _CINT_H_
#define _CINT_H_

#include <stdint.h>

/* Temporary buffer size for calculations - can be changed if necessary */
#define MAX_NUMLEN 500

typedef char * cint_t;

void cint_assign(cint_t res, int x); //Initializes cint with given int number
int cint_decompress(const cint_t num, int *value); //Converts stored value into int number. Returns 1 in case of success, 0 otherwise (If value does not fit in int type)

int cint_is_even(const cint_t num); //Returns 1 if given cint is even, 0 otherwise
int cint_cmp(const cint_t a, const cint_t b);
int cint_get_size(const cint_t num); //Returns actual byte size of given cint

void cint_neg(cint_t num); //"Unary minus"

void cint_sadd(cint_t a, int8_t b); //Adds a small int to cint in place
void cint_add(const cint_t a, const cint_t b, cint_t res);
void cint_subtract(const cint_t a, const cint_t b, cint_t res);
void cint_between(const cint_t a, const cint_t b, cint_t res); //Counts two given cints

void cint_mul(const cint_t a, const cint_t b, cint_t res);
int cint_sdiv(const cint_t a, unsigned int b, cint_t quot);

char * cint_to_str(const cint_t initial, char * str);

#endif /* _CINT_H_ */
