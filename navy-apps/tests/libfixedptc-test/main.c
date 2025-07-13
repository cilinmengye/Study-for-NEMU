#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <fixedptc.h>


float threshold = 0.01;
int num = 2;

#define TEST(irefFormula, irelFormula, frefFormula, frelFormula, testName)  \
    for (int i = 0; i < leni; i++) {    \
        int ref = irefFormula;   \
        int rel = irelFormula;   \
        if (ref != rel) {   \
            printf("error in %s integers index %d\n ref: %d\n rel: %d\n", testName, i, ref, rel);  \
            assert(0);  \
        }   \
    }   \
    for (int i = 0; i < lenf; i++) {    \
        float ref = frefFormula;    \
        float rel = frelFormula;    \
        float sub = ref > rel ? ref - rel : rel - ref;   \
        if (sub > threshold) {  \
            printf("error in %s floats index %d\n ref: %f\n rel: %f\n", testName, i, ref, rel);    \
            assert(0);  \
        }   \
    }   \
    printf("%s PASS!\n", testName)



int main() {
    int integers[] = {1234567, -1234567};        int leni = sizeof(integers) / sizeof(int);
    float floats[] = {1234567.996, -1234567.996};   int lenf = sizeof(floats) / sizeof(float);
    // test fixedpt_muli
    TEST(integers[i] * num, fixedpt_toint((fixedpt_muli(fixedpt_fromint(integers[i]), num))),
         floats[i] * num,   fixedpt_tofloat((fixedpt_muli(fixedpt_rconst(floats[i]), num))),
         "fixedpt_muli");
    // test fixedpt_divi
    TEST(integers[i] / num, fixedpt_toint((fixedpt_divi(fixedpt_fromint(integers[i]), num))),
         floats[i] / num,   fixedpt_tofloat((fixedpt_divi(fixedpt_rconst(floats[i]), num))),
         "fixedpt_divi");
    // test fixedpt_mul
    TEST(integers[i] * num, fixedpt_toint((fixedpt_mul(fixedpt_fromint(integers[i]), fixedpt_fromint(num)))),
         floats[i] * num,   fixedpt_tofloat((fixedpt_mul(fixedpt_rconst(floats[i]), fixedpt_fromint(num)))),
         "fixdpt_mul");
    // test fixedpt_div
    TEST(integers[i] / num, fixedpt_toint((fixedpt_div(fixedpt_fromint(integers[i]), fixedpt_fromint(num)))),
         floats[i] / num,   fixedpt_tofloat((fixedpt_div(fixedpt_rconst(floats[i]), fixedpt_fromint(num)))),
         "fixdpt_div");
    // test fixedpt_abs
    TEST(integers[i] >= 0 ? integers[i] : -1 * integers[i], 
         fixedpt_toint((fixedpt_abs(fixedpt_fromint(integers[i])))),
         floats[i] >= 0 ? floats[i] : -1 * floats[i],   
         fixedpt_tofloat((fixedpt_abs(fixedpt_rconst(floats[i])))),
         "fixdpt_abs");
    // test fixedpt_floor
    TEST(floor(integers[i]), 
         fixedpt_toint((fixedpt_floor(fixedpt_fromint(integers[i])))),
         floor(floats[i]),   
         fixedpt_tofloat((fixedpt_floor(fixedpt_rconst(floats[i])))),
         "fixdpt_floor");
    // test fixedpt_ceil
    TEST(ceil(integers[i]), 
         fixedpt_toint((fixedpt_ceil(fixedpt_fromint(integers[i])))),
         ceil(floats[i]),   
         fixedpt_tofloat((fixedpt_ceil(fixedpt_rconst(floats[i])))),
         "fixdpt_ceil");
    // test fixedpt_div
    TEST(integers[i] / integers[i], fixedpt_toint((fixedpt_div(fixedpt_fromint(integers[i]), fixedpt_fromint(integers[i])))),
         floats[i] / floats[i],   fixedpt_tofloat((fixedpt_div(fixedpt_rconst(floats[i]), fixedpt_rconst(floats[i])))),
         "fixdpt_div2");
    // test fixedpt_div
    TEST(integers[i] / floats[i], fixedpt_toint((fixedpt_div(fixedpt_fromint(integers[i]), fixedpt_rconst(floats[i])))),
         floats[i] / integers[i],   fixedpt_tofloat((fixedpt_div(fixedpt_rconst(floats[i]), fixedpt_fromint(integers[i])))),
         "fixdpt_div3");
    return 0;
}