#include "cint.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>

/* This code produces a lot of debug output, if VERBOSE option is defined */

//#define VERBOSE

#define MIN_STORED_INT -135274560l
#define MAX_STORED_INT 135274559l

#define ALWAYS_INT_TEST 1000 /* All integers with absolute value less than this will be considered in all tests */

#define BORDERS_SIZE 6
static int borders[BORDERS_SIZE] = 
{
    1056835l,
    8257l,
    -65l,
    64l,
    8256l,
    1056832l
}; 

#define NEIGHBORHOOD 10 /* All integers within neighborhood of interesting points will be considered */

#define RANDOM_FACTOR 500 /* Amount of random integers to use in tests */
#define INT_RANGE (MAX_STORED_INT - MIN_STORED_INT + 1 - 2 * NEIGHBORHOOD)
#define RANGE_START (MIN_STORED_INT + NEIGHBORHOOD)
/* Do not include neighborhood because it's already tested and we can run out of int borders */

#define MAX_SUMMAND 60
/* Maximum summand for small addition */

/* Buffers for c_ints */
static char buf1[MAX_NUMLEN];
static char buf2[MAX_NUMLEN];
static char buf3[MAX_NUMLEN];

void report_error(int value1, int value2, int expected, int actual, std::string message) {
    printf("------------------------\n");
    printf("%s\n", message.data());
    printf("Arguments: %d %d\n", value1, value2);
    printf("Expected result: %d\n", expected);
    printf("Calculated result: %d\n", actual);
    printf("------------------------\n");
    printf("Testing failed!!!\n\n");
    _exit(0);
}

void print_info(int value1, int value2, int expected, int actual, std::string message) {
#ifdef VERBOSE
    printf("------------------------\n");
    printf("%s\n", message.data());
    printf("Arguments: %d %d\n", value1, value2);
    printf("Expected result: %d\n", expected);
    printf("Calculated result: %d\n", actual);
    printf("------------------------\n");
#endif
}

/*
 * Compression, decompression and inverting
 */

void test_number_compression(int i) {
    cint_t num = buf2;
    int res;

    cint_assign(num, i);
    if (!cint_decompress(num, &res)) {
        report_error(i, 0, 0, 0, "Cannot decompress, but should");
    }
    if (res != i) {
        report_error(0, 0, i, res, "Decompressed not the same as compressed");
    }
    cint_neg(num);
    if (!cint_decompress(num, &res) && i != MIN_STORED_INT) {
        report_error(i, 0, -i, 0, "Cannot decompress negative, but should");
    }
    if (res != -i && i != MIN_STORED_INT) {
        report_error(i, 0, -i, res, "Decompressed negative not the same as compressed");
    }
}

void test_compression() {
    printf("------------------------\n");
    printf("Testing compression, decompression and inverting\n");

    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        test_number_compression(i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            test_number_compression(j);
        }
    }
    
    /* Minimum and maximum integer numbers that can be decompressed */
    for (int i = MIN_STORED_INT; i <= MIN_STORED_INT + NEIGHBORHOOD; i++) {
        test_number_compression(i);
    }
    for (int i = MAX_STORED_INT - NEIGHBORHOOD; i <= MAX_STORED_INT; i++) {
        test_number_compression(i);
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = RANGE_START + rand() % INT_RANGE;
        test_number_compression(cur_int);
    }
    printf("Compression testing succeded\n");
    printf("------------------------\n");
}

/*
 * Small addition
 */

void test_number_small_addition(int x) {
    cint_t num = buf1;
    cint_t res = buf2;
    int sum;

    cint_assign(num, x);
    int size = cint_get_size(num);

    for (int i = -MAX_SUMMAND; i <= MAX_SUMMAND; i++) {
        memcpy(res, num, size);
        cint_sadd(res, i);
        if (!cint_decompress(res, &sum) && x + i >= MIN_STORED_INT && x + i <= MAX_STORED_INT) {
            report_error(x, i, x + i, 0, "Cannot decompress resulted sum, but should");
        }
        if (sum != x + i && x + i >= MIN_STORED_INT && x + i <= MAX_STORED_INT) {
            report_error(x, i, x + i, sum, "Sum is wrong :(");
        } else {
            print_info(x, i, x + i, sum, "Small addition");
        }
    }
}

void test_small_addition() {
    printf("------------------------\n");
    printf("Testing small addition\n");

    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        test_number_small_addition(i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            test_number_small_addition(j);
        }
    }
    
    /* Minimum and maximum integer numbers that can be decompressed */
    for (int i = MIN_STORED_INT; i <= MIN_STORED_INT + NEIGHBORHOOD; i++) {
        test_number_small_addition(i);
    }
    for (int i = MAX_STORED_INT - NEIGHBORHOOD; i <= MAX_STORED_INT; i++) {
        test_number_small_addition(i);
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = RANGE_START + rand() % INT_RANGE;
        test_number_small_addition(cur_int);
    }
    printf("Small addition testing succeded\n");
    printf("------------------------\n");
}

/*
 * Addition and subtraction
 */

void test_number_addition(cint_t num1, int x, int y) {
    cint_t num2 = buf2;
    cint_t res = buf3;
    cint_assign(num2, y);
    int ex_sum = x + y; /* Expected sum */
    int ex_sub = x - y;
    int ex_between = floor((x + y) / 2.0);
    int result;

    cint_add(num1, num2, res);
    if (!cint_decompress(res, &result) && ex_sum >= MIN_STORED_INT && ex_sum <= MAX_STORED_INT) {
        report_error(x, y, ex_sum, 0, "Cannot decompress resulted sum, but should");
    }
    if (result != ex_sum && ex_sum >= MIN_STORED_INT && ex_sum <= MAX_STORED_INT) {
        report_error(x, y, ex_sum, result, "Sum is wrong :(");
    } else {
        print_info(x, y, ex_sum, result, "Addition");
    }
    
    cint_subtract(num1, num2, res);
    if (!cint_decompress(res, &result) && ex_sub >= MIN_STORED_INT && ex_sub <= MAX_STORED_INT) {
        report_error(x, y, ex_sub, 0, "Cannot decompress result of subtraction, but should");
    }
    if (result != ex_sub && ex_sub >= MIN_STORED_INT && ex_sub <= MAX_STORED_INT) {
        report_error(x, y, ex_sub, result, "Result of subtraction is wrong :(");
    } else {
        print_info(x, y, ex_sub, result, "Subtraction");
    }
    
    cint_between(num1, num2, res);
    if (!cint_decompress(res, &result)) {
        report_error(x, y, ex_between, 0, "Cannot decompress between result, but should");
    }
    if (result != ex_between) {
        report_error(x, y, ex_between, result, "Between number is incorrect");
    } else {
        print_info(x, y, ex_between, result, "Between");
    }
}

void addition_second_summand(int x) {
    //Doing it here because we don't want to do it always in test_number_addition
    cint_t num1 = buf1;
    cint_assign(num1, x);
    
    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        test_number_addition(num1, x, i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            test_number_addition(num1, x, j);
        }
    }
    
    /* Minimum and maximum integer numbers that can be decompressed */
    for (int i = MIN_STORED_INT; i <= MIN_STORED_INT + NEIGHBORHOOD; i++) {
        test_number_addition(num1, x, i);
    }
    for (int i = MAX_STORED_INT - NEIGHBORHOOD; i <= MAX_STORED_INT; i++) {
        test_number_addition(num1, x, i);
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = RANGE_START + rand() % INT_RANGE;
        test_number_addition(num1, x, cur_int);
    }
}

void test_addition() {
    printf("------------------------\n");
    printf("Testing addition, subtraction and between\n");

    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        addition_second_summand(i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            addition_second_summand(j);
        }
    }
    
    /* Minimum and maximum integer numbers that can be decompressed */
    for (int i = MIN_STORED_INT; i <= MIN_STORED_INT + NEIGHBORHOOD; i++) {
        addition_second_summand(i);
    }
    for (int i = MAX_STORED_INT - NEIGHBORHOOD; i <= MAX_STORED_INT; i++) {
        addition_second_summand(i);
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = RANGE_START + rand() % INT_RANGE;
        addition_second_summand(cur_int);
    }
    printf("Addition testing succeded\n");
    printf("------------------------\n");
}

/*
 * Multiplication
 */

void test_number_multiplication(cint_t num1, int x, int y) {
    cint_t num2 = buf2;
    cint_t res = buf3;
    cint_assign(num2, y);
    int ex_mul = x * y; /* Expected product */
    int result;

    cint_mul(num1, num2, res);
    if (!cint_decompress(res, &result) && ex_mul >= MIN_STORED_INT && ex_mul <= MAX_STORED_INT) {
        report_error(x, y, ex_mul, 0, "Cannot decompress resulted product, but should");
    }
    if (result != ex_mul && ex_mul >= MIN_STORED_INT && ex_mul <= MAX_STORED_INT) {
        report_error(x, y, ex_mul, result, "Product is wrong :(");
    } else {
        print_info(x, y, ex_mul, result, "Multiplication");
    }
}

void multiplication_second_multiplier(int x) {
    //Doing it here because we don't want to do it always in test_number_addition
    cint_t num1 = buf1;
    cint_assign(num1, x);

    int max_multiplier = 1;
    if (x != 0) {
        max_multiplier = abs((MAX_STORED_INT + 1)/ x);
    }

    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        if (abs(i) <= max_multiplier) {
            test_number_multiplication(num1, x, i);
        }
    }

    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            if (abs(j) <= max_multiplier) {
                test_number_multiplication(num1, x, j);
            }
        }
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = -max_multiplier + rand() % (2 * max_multiplier);
        test_number_multiplication(num1, x, cur_int);
    }
}

void test_multiplication() {
    printf("------------------------\n");
    printf("Testing multiplication\n");
    
    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        multiplication_second_multiplier(i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            multiplication_second_multiplier(j);
        }
    }
    
    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = RANGE_START + rand() % INT_RANGE;
        multiplication_second_multiplier(cur_int);
    }
    
    int max_multiplier = floor(sqrt(MAX_STORED_INT));
    /* Some small random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = -max_multiplier + rand() % (2 * max_multiplier);
        multiplication_second_multiplier(cur_int);
    }
    printf("Multiplication testing succeded\n");
    printf("------------------------\n");
}

/*
 * Small division
 */

void test_number_small_division(cint_t num1, int x, int y) {
    cint_t res = buf2;
    int ex_div = x / y;
    int ex_mod = x % y;
    int div, mod;

    mod = cint_sdiv(num1, y, res);
    if (mod != ex_mod) {
        report_error(x, y, ex_mod, mod, "Remainder is wrong :(");
    } else {
        print_info(x, y, ex_mod, mod, "Remainder");
    }
    if (!cint_decompress(res, &div) && ex_div >= MIN_STORED_INT && ex_div <= MAX_STORED_INT) {
        report_error(x, y, ex_div, 0, "Cannot decompress resulted quotient, but should");
    }
    if (div != ex_div && ex_div >= MIN_STORED_INT && ex_div <= MAX_STORED_INT) {
        report_error(x, y, ex_div, div, "Quotient is wrong :(");
    } else {
        print_info(x, y, ex_div, div, "Quotient");
    }
}

void test_divider(int x) {
    //Doing it here because we don't want to do it always in test_number_addition
    cint_t num1 = buf1;
    cint_assign(num1, x);

    /* Testing all small integers */
    for (int i = 1; i <= std::min(x, ALWAYS_INT_TEST); i++) {
        test_number_small_division(num1, x, i);
    }

    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            if (j >= 1) {
                test_number_small_division(num1, x, j);
            }
        }
    }

    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int;
        int seed = x - 1;
        if (seed == 0) {
            seed++;
        }
        cur_int = 1 + rand() % seed;
        test_number_small_division(num1, x, cur_int);
    }
}

void test_small_division() {
    printf("------------------------\n");
    printf("Testing small division\n");
    
    /* Testing all small integers */
    for (int i = -ALWAYS_INT_TEST; i <= ALWAYS_INT_TEST; i++) {
        test_divider(i);
    }
    
    /* Testing all interesting integers */
    /* Size borders */
    for (int i = 0; i < BORDERS_SIZE; i++) {
        for (int j = borders[i] - NEIGHBORHOOD; j <= borders[i] + NEIGHBORHOOD; j++) {
            if (j >= 0) {
                test_divider(j);
            }
        }
    }
    
    /* Some random integers */
    for (int i = 0; i < RANDOM_FACTOR; i++) {
        int cur_int = rand() % (INT_RANGE / 2);
        test_divider(i);
    }
    printf("Division testing succeded\n");
    printf("------------------------\n");
}

/*
 * Calling tests
 */

void perform_tests() {
    printf("\nTesting...\n");

    cint_t num1 = buf1;
    cint_assign(num1, -2);
    test_number_small_division(num1, -2, 1056842);

    test_compression();
    test_small_addition();
    test_addition();
    test_multiplication();
    test_small_division();

    printf("Testing completed succesfully!\n\n");
}

int main() {
    //Some unit tests
    perform_tests();
    return 0;
}
