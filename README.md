# blitch-firmware
Bluetooth switch firmware

# Data format
Company ID: 0x0059 (Nordic)  
Manufacturer data:
* 0x64 ('d')
* 0x74 ('t')
* Product Number - 1 byte
* Data length - 1 byte
* Data
* Checksum - all bytes XOR-ed

## Product numbers
* 0x01: Blitch with only one input