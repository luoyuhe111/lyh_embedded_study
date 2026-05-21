#ifndef __APP_H__
#define __APP_H__

#define WEATHER_URL "https://api.seniverse.com/v3/weather/now.json?key=YOUR_SENIVERSE_KEY&location=YOUR_LOCATION&language=en&unit=c"

#define APP_VERSION "v1.0"
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

void wifi_init(void);
void wifi_wait_connect(void);
void main_loop(void);
void main_loop_init(void);

#endif
