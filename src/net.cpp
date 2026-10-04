/**
 net.cpp - optional Wi-Fi + MQTT logging to Home Assistant (see net.h)
**/

#include <Arduino.h>
#include "net.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define GRINDERBOT_NET 1
#endif

#ifndef GRINDERBOT_NET

void NetBegin() { Serial.println("Wi-Fi off (no include/secrets.h)"); }
void NetUpdate(bool, bool) {}
void NetPublishGrind(const GrindRecord&) {}
bool NetConnected() { return false; }
String NetInfo() { return String("Wi-Fi off\nAdd include/secrets.h\nto log to Home Asst."); }

#else

#include <stdarg.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include "watchdog.h"

namespace {

const unsigned long STATUS_POLL_MS = 1000;
const unsigned long WIFI_RETRY_MIN_MS = 20000;   // retries back off, doubling up to the max
const unsigned long WIFI_RETRY_MAX_MS = 300000;
const unsigned long MQTT_RETRY_MIN_MS = 15000;
const unsigned long MQTT_RETRY_MAX_MS = 300000;
const uint16_t TCP_CONNECT_TIMEOUT_MS = 3000;
const unsigned long MQTT_CONNACK_TIMEOUT_MS = 3000;
// The keep-alive only runs between grinds; brokers drop a client after 1.5x
// this with no traffic, which has to outlast the longest grind (~70 s)
const unsigned long MQTT_KEEPALIVE_MS = 150000;

WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

bool moduleOk = false;
char deviceId[24] = "grinderbot";  // grinderbot_<last 3 bytes of the MAC>
char baseTopic[32] = "grinderbot";
char ipText[16] = "";
uint8_t wifiStatus = WL_IDLE_STATUS;
unsigned long lastStatusPoll = 0;
unsigned long nextWifiTry = 0;
unsigned long wifiBackoff = WIFI_RETRY_MIN_MS;
unsigned long nextMqttTry = 0;
unsigned long mqttBackoff = MQTT_RETRY_MIN_MS;
bool mqttUp = false;
bool havePending = false;
GrindRecord pending;

bool due(unsigned long now, unsigned long when) { return (long)(now - when) >= 0; }

unsigned long doubled(unsigned long ms, unsigned long maxMs) { return ms * 2 > maxMs ? maxMs : ms * 2; }

// snprintf into a fixed buffer, remembering if anything didn't fit
class Text {
 public:
  void add(const char* fmt, ...) {
    if (len_ >= sizeof(buf_) - 1) {
      overflow_ = true;
      return;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf_ + len_, sizeof(buf_) - len_, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    len_ += (size_t)n;
    if (len_ >= sizeof(buf_)) {
      len_ = sizeof(buf_) - 1;
      overflow_ = true;
    }
  }
  const char* c_str() const { return buf_; }
  bool ok() const { return !overflow_; }

 private:
  char buf_[640] = "";
  size_t len_ = 0;
  bool overflow_ = false;
};

bool publish(const char* topic, const char* payload, bool retain) {
  WatchdogFeed();  // each write can wait a couple of seconds on a bad link
  unsigned long n = strlen(payload);
  // Known size, so the library streams it instead of using its 256-byte buffer
  if (!mqtt.beginMessage(topic, n, retain, 0, false)) return false;
  if (mqtt.write((const uint8_t*)payload, n) != n) {
    mqtt.stop();  // the broker has half a message; start over on reconnect
    return false;
  }
  return mqtt.endMessage();
}

struct Entity {
  const char* key;
  const char* name;
  const char* topic;  // "grind" (every grind) or "dose" (only grinds that reached the target)
  const char* field;  // key in the JSON
  const char* unit;
  const char* deviceClass;
  const char* stateClass;
  const char* icon;
};

const Entity ENTITIES[] = {
    // Dose statistics only from grinds that finished normally, not partial
    // weights from stopped or cut-off ones
    {"dose", "Last dose", "dose", "actual", "g", "weight", "measurement", "mdi:coffee"},
    {"target", "Last target", "dose", "target", "g", "weight", "measurement", "mdi:target"},
    {"time", "Last grind time", "dose", "time", "s", "duration", "measurement", "mdi:timer-outline"},
    {"offset", "Grind offset", "grind", "new_offset", "g", "weight", "measurement", "mdi:tune"},
    {"result", "Last result", "grind", "result", nullptr, nullptr, nullptr, "mdi:information-outline"},
    {"grinds", "Grinds", "grind", "grinds", nullptr, nullptr, "total_increasing", "mdi:counter"},
};

// Home Assistant MQTT discovery: one retained config message per sensor
void sendDiscovery() {
  for (const Entity& e : ENTITIES) {
    char topic[80];
    snprintf(topic, sizeof topic, "homeassistant/sensor/%s/%s/config", deviceId, e.key);
    Text p;
    p.add("{\"name\":\"%s\",\"uniq_id\":\"%s_%s\"", e.name, deviceId, e.key);
    p.add(",\"stat_t\":\"%s/%s\",\"val_tpl\":\"{{ value_json.%s }}\"", baseTopic, e.topic, e.field);
    p.add(",\"avty_t\":\"%s/status\",\"frc_upd\":true,\"ic\":\"%s\"", baseTopic, e.icon);
    if (e.unit) p.add(",\"unit_of_meas\":\"%s\"", e.unit);
    if (e.deviceClass) p.add(",\"dev_cla\":\"%s\"", e.deviceClass);
    if (e.stateClass) p.add(",\"stat_cla\":\"%s\"", e.stateClass);
    // The whole grind record as attributes on the main sensor
    if (strcmp(e.key, "dose") == 0) p.add(",\"json_attr_t\":\"%s/dose\"", baseTopic);
    p.add(",\"dev\":{\"ids\":[\"%s\"],\"name\":\"GrinderBot\",\"mf\":\"Tailslide\","
          "\"mdl\":\"MKR WiFi 1010 grinder scale\"}}",
          deviceId);
    if (!p.ok()) {
      Serial.print("Discovery message too long: ");
      Serial.println(e.key);
      continue;
    }
    publish(topic, p.c_str(), true);
  }
}

// Every grind goes to <base>/grind; ones that reached the target also go to
// <base>/dose. Both retained, so Home Assistant has them after a restart.
void publishGrind(const GrindRecord& r) {
  char topic[48];
  snprintf(topic, sizeof topic, "%s/grind", baseTopic);
  char doseTopic[48];
  snprintf(doseTopic, sizeof doseTopic, "%s/dose", baseTopic);
  char payload[200];
  snprintf(payload, sizeof payload,
           "{\"dose\":%u,\"target\":%.1f,\"actual\":%.1f,\"time\":%.1f,\"offset\":%.2f,"
           "\"new_offset\":%.2f,\"learned\":%s,\"result\":\"%s\",\"grinds\":%lu}",
           (unsigned)r.dose, r.targetG, r.actualG, r.seconds, r.offsetG, r.newOffsetG,
           r.learned ? "true" : "false", GrindResultName(r.result), (unsigned long)r.count);
  bool done = (r.result == GrindResult::Done);
  if (publish(topic, payload, true) && (!done || publish(doseTopic, payload, true))) {
    havePending = false;
    Serial.print("MQTT: published ");
    Serial.println(payload);
  }
}

bool mqttConnect() {
  char willTopic[48];
  snprintf(willTopic, sizeof willTopic, "%s/status", baseTopic);
  mqtt.setId(deviceId);
  if (strlen(MQTT_USER) > 0) mqtt.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);
  mqtt.setConnectionTimeout(MQTT_CONNACK_TIMEOUT_MS);
  mqtt.setKeepAliveInterval(MQTT_KEEPALIVE_MS);
  // Broker marks the scale offline if it drops off
  mqtt.beginWill(willTopic, strlen("offline"), true, 1);
  mqtt.print("offline");
  mqtt.endWill();
  wifiClient.setConnectionTimeout(TCP_CONNECT_TIMEOUT_MS);
  WatchdogFeed();  // a failed connect can take ~11 s (TCP, CONNACK, close)
  Serial.print("MQTT: connecting to " MQTT_HOST "... ");
  IPAddress ip;
  bool ok = ip.fromString(MQTT_HOST) ? mqtt.connect(ip, MQTT_PORT) : mqtt.connect(MQTT_HOST, MQTT_PORT);
  if (!ok) {
    Serial.print("failed, error ");
    Serial.println(mqtt.connectError());
    return false;
  }
  Serial.println("connected");
  publish(willTopic, "online", true);
  sendDiscovery();
  return true;
}

}  // namespace

void NetBegin() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Wi-Fi module not found");
    return;
  }
  moduleOk = true;
  String fw = WiFi.firmwareVersion();
  Serial.print("Wi-Fi module firmware ");
  Serial.print(fw);
  if (fw < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print(" (WiFiNINA expects " WIFI_FIRMWARE_LATEST_VERSION "; update it with the Arduino IDE's firmware updater if MQTT won't connect)");
  }
  Serial.println();
  byte mac[6];
  WiFi.macAddress(mac);  // WiFiNINA returns the bytes last-first
  snprintf(deviceId, sizeof deviceId, "grinderbot_%02x%02x%02x", mac[2], mac[1], mac[0]);
  snprintf(baseTopic, sizeof baseTopic, "grinderbot/%02x%02x%02x", mac[2], mac[1], mac[0]);
  WiFi.setHostname("grinderbot");
  // begin() normally waits up to 50 s; with no timeout it starts the
  // connection and returns, and NetUpdate() watches the status instead
  WiFi.setTimeout(0);
  nextWifiTry = millis();
}

void NetUpdate(bool canPoll, bool canBlock) {
  if (!moduleOk || !canPoll) return;
  unsigned long now = millis();

  if (now - lastStatusPoll >= STATUS_POLL_MS) {
    lastStatusPoll = now;
    uint8_t status = WiFi.status();
    if (status == WL_CONNECTED && wifiStatus != WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      snprintf(ipText, sizeof ipText, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      Serial.print("Wi-Fi connected, IP ");
      Serial.println(ipText);
      wifiBackoff = WIFI_RETRY_MIN_MS;
      nextMqttTry = now;
    }
    wifiStatus = status;
  }

  if (wifiStatus != WL_CONNECTED) {
    if (mqttUp) {
      mqtt.stop();
      mqttUp = false;
    }
    if (canBlock && due(now, nextWifiTry)) {
      Serial.println("Wi-Fi: connecting to " WIFI_SSID);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // returns straight away (setTimeout(0))
      nextWifiTry = now + wifiBackoff;
      wifiBackoff = doubled(wifiBackoff, WIFI_RETRY_MAX_MS);
    }
    return;
  }

  if (mqttUp) {
    mqtt.poll();
    if (!mqtt.connected()) {
      Serial.println("MQTT disconnected");
      mqttUp = false;
      mqttBackoff = MQTT_RETRY_MIN_MS;
      nextMqttTry = now + MQTT_RETRY_MIN_MS;
      return;
    }
    if (havePending) publishGrind(pending);
    return;
  }

  if (canBlock && due(now, nextMqttTry)) {
    if (mqttConnect()) {
      mqttUp = true;
      mqttBackoff = MQTT_RETRY_MIN_MS;
      if (havePending) publishGrind(pending);
    } else {
      nextMqttTry = millis() + mqttBackoff;
      mqttBackoff = doubled(mqttBackoff, MQTT_RETRY_MAX_MS);
    }
  }
}

void NetPublishGrind(const GrindRecord& record) {
  pending = record;  // NetUpdate() sends it on its next pass (or after reconnecting)
  havePending = true;
}

bool NetConnected() { return mqttUp; }

String NetInfo() {
  if (!moduleOk) return String("No Wi-Fi module");
  char line1[24];
  switch (wifiStatus) {
    case WL_CONNECTED: snprintf(line1, sizeof line1, "WiFi %s", ipText); break;
    case WL_NO_SSID_AVAIL: snprintf(line1, sizeof line1, "WiFi: SSID not found"); break;
    case WL_CONNECT_FAILED: snprintf(line1, sizeof line1, "WiFi: bad password?"); break;
    default: snprintf(line1, sizeof line1, "WiFi connecting..."); break;
  }
  char line2[24];
  if (mqttUp) {
    snprintf(line2, sizeof line2, "MQTT connected");
  } else if (wifiStatus == WL_CONNECTED) {
    long wait = (long)(nextMqttTry - millis()) / 1000;
    snprintf(line2, sizeof line2, "MQTT retry in %lds", wait > 0 ? wait : 0);
  } else {
    snprintf(line2, sizeof line2, "MQTT waiting");
  }
  String text = String(line1) + "\n" + String(line2) + "\nID " + String(deviceId + 11);
  return text;
}

#endif
