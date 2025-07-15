#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <fixedptc.h>

int main()
{
     fixedpt a = fixedpt_rconst(1.2);
     fixedpt b = fixedpt_fromint(10);
     int c = 0;
     if (b > fixedpt_rconst(7.9)) {
          c = fixedpt_toint(fixedpt_div(fixedpt_mul(a + FIXEDPT_ONE, b), fixedpt_rconst(2.3)));
     }
     printf("c:%d\n", c);
     fixedpt a2 = fixedpt_ceil(fixedpt_abs(fixedpt_rconst(-1.2)));
     fixedpt b2 = fixedpt_floor(fixedpt_abs(fixedpt_rconst(-10.2)));
     int c2 = 0;
     if (b2 > fixedpt_rconst(7.9)) {
          c2 = fixedpt_toint(fixedpt_div(fixedpt_mul(a2 + FIXEDPT_ONE, b2), fixedpt_rconst(2.3)));
     }
     printf("a2:%d b2:%d c2:%d\n", fixedpt_toint(a2), fixedpt_toint(b2), c2);
     // printf("a3:%d\n", fixedpt_toint(fixedpt_abs(fixedpt_rconst(-1.2))));
     // printf("test: %f\n", fixedpt_tofloat((fixedpt_abs(fixedpt_rconst(-1.2)))));
     // printf("test: %f\n", fixedpt_tofloat((fixedpt_floor(fixedpt_rconst(-1.2)))));
     // printf("test: %f\n", fixedpt_tofloat((fixedpt_rconst(-1.2))));
     return 0;
}

