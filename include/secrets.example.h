#pragma once
// Copy this file to secrets.h (same folder) and fill it in to turn on Wi-Fi
// and Home Assistant logging. secrets.h is in .gitignore, so your passwords
// stay out of the repo. Without secrets.h the scale builds and works with no
// network at all.

#define WIFI_SSID "your-wifi-name"      // 2.4 GHz network (the MKR's NINA module has no 5 GHz)
#define WIFI_PASSWORD "your-wifi-password"

// MQTT broker, e.g. Home Assistant's Mosquitto add-on. Use the IP address:
// the Wi-Fi module can't resolve ".local" names like homeassistant.local.
#define MQTT_HOST "192.168.1.10"
#define MQTT_PORT 1883
#define MQTT_USER "grinderbot"          // an HA user (or "" if the broker allows anonymous)
#define MQTT_PASSWORD "mqtt-password"
