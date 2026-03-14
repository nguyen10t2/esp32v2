#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#define MQTT_BROKER     "f5.soict.io" // Thay bằng địa chỉ IP máy tính đang chạy backend/broker
#define MQTT_PORT       1883

#define NODE_ID         1

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TOPIC_PUBLISH   "fire/" STR(NODE_ID) "/sensor"
#define TOPIC_SUBSCRIBE "fire/" STR(NODE_ID) "/cmd"

#endif