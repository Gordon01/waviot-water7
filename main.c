#include <stdio.h>
#include <stdlib.h>
#include "lib/WVT_Water7.h"

int main(int argc, char *argv[]) {
    if (argc != 3)
    {
        puts("Please pass two argument");
        return 1;
    }

    const uint16_t event = strtol(argv[1], NULL, 10);
    const uint16_t payload = strtol(argv[2], NULL, 10);

    uint8_t responce[100];
    char responce_text[100];
    uint8_t responce_length = WVT_W7_Event(event, payload, responce);

    printf("Responce length = %u\n", responce_length);

    for (int i = 0; i < responce_length; i++)
    {
        printf("%02x ", responce[i]);
    }
    
    putchar(0x0A);
    
    return 0;
}