#ifndef SRC_BTHOME
#define SRC_BTHOME

#define BTHOME_PACKET_TYPE                      0x40

#define BTHOME_DEVICE_PACKET_V2                 0x40
#define BTHOME_DEVICE_ADV_REGULAR               0x00        // device is sending BLE advertisements at a regular interval
#define BTHOME_DEVICE_ADV_IRREGULAR             0x04        // device is sending BLE advertisements at a irregular interval (e.g. only when someone pushes a button)
#define BTHOME_DEVICE_ENCRYPTION_ON             0x01
#define BTHOME_DEVICE_ENCRYPTION_OFF            0x00

#define BTHOME_PACKET_ID                        0x00

// Sensor data
#define BTHOME_SENSOR_BATTERY                   0x01        // uint8_t [%]
#define BTHOME_SENSOR_TEMPERATURE_SINT8         0x57        // sint8_t [degC]

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
