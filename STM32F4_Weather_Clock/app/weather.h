#ifndef __WEATHER_H
#define __WEATHER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    char city[32];
    char location[128];
    char weather[16];
    int weather_code;
    float temperature;

}weather_info_t;

bool parse_seniverse_response(const char* response, weather_info_t* info);

#endif
