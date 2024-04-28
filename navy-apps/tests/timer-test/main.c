#include <stdio.h>
#include <sys/time.h>

int main() {
    int time = 0;
    struct timeval oldtv;
    struct timeval newtv;
    gettimeofday(&oldtv, NULL);
    while (1){
        gettimeofday(&newtv, NULL);  
        if ((newtv.tv_usec - oldtv.tv_usec) >= 500000){
            printf("access 0.5s interval at %d times", time++);
            oldtv = newtv;
        }
    }
    return 0;
}
