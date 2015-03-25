#include <stdio.h>
#include <math.h>

int main(void){
  unsigned int n = pow(2, 8); //256

  while (n) {
    if (n & 1)
        printf("1");
    else
        printf("0");

    n >>= 1;
}
printf("\n");

if(0)
printf("0 is true\n");
else
printf("0 is false\n");

  return 0;
}
