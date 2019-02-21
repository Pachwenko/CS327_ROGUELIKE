#include <stdio.h>


int main(void) {
    unsigned char attributes = 1;

    printf("%i\n", attributes);

    attributes++;

    printf("%i\n", attributes);

    attributes += 14;
    printf("%d\n", attributes);
    return 0;
}