#ifndef CONFIG_H
#define CONFIG_H

#define HOSTNAME "DHT-Beacon"
#define SSID "mywlan"
#define PSK "xxxxx"

#define REMOTE_HOST "raspberrypi.fritz.box"
#define REMOTE_PORT 80
#define REMOTE_ENDPOINT "/sensordb/new"

#if __has_include("custom-config.h")
 #include "custom-config.h"
#endif

#endif
