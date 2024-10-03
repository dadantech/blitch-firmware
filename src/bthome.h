#ifndef SRC_BTHOME
#define SRC_BTHOME

#define BTHOME_PACKET_TYPE                      0x40
#define BTHOME_PACKET_ID                        0x00

// Sensor data
#define BTHOME_SENSOR_TEMPERATURE_SINT8         0x57

// Binary Sensor data
#define BTHOME_SENSOR_BIN_VIBRATION             0x2C        // 0 (False = Clear) 1 (True = Detected)

// Events
#define BTHOME_EVENT_TYPE_BUTTON                0x3A
#define BTHOME_EVENT_TYPE_DIMMER                0x3C

#define BTHOME_EVENT_BUTTON_NONE                0x00
#define BTHOME_EVENT_BUTTON_PRESS               0x01
#define BTHOME_EVENT_BUTTON_DOUBLE_PRESS        0x02
#define BTHOME_EVENT_BUTTON_TRIPLE_PRESS        0x03
#define BTHOME_EVENT_BUTTON_LONG_PRESS          0x04
#define BTHOME_EVENT_BUTTON_LONG_DOUBLE_PRESS   0x05
#define BTHOME_EVENT_BUTTON_LONG_TRIPLE_PRESS   0x06
#define BTHOME_EVENT_BUTTON_HOLD_PRESS          0x80

#endif /* SRC_BTHOME */
