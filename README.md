# The Dreamdesk Project
The aim of this project was to create a framework that makes compatible any electrical standing desk with a home automation solution like Apple Homekit, Google Home, or Amazon Alexa, allowing your desk to be controlled by your phone, a computer, or a home speaker.

It has been tested and currently works with any desk relying on the Dynamic Motion system by Logicdata, like Yaasa Desks, Gravit iDesk, etc. The IKEA Bekant desks will be supported soon. The code was developped in C for the ESP32 microcontrollers family.

## Config
Rename wifi.h.default to wifi.h and set your Wifi SSID and password.
```
mv main/wifi.h.default main/wifi.h

#define WIFI_SSID      "WIFI_SSID"
#define WIFI_PASS      "WIFI_PASSWORD"
```

## Install
### Espressif SDK
```
git clone -b v4.4 --recursive https://github.com/espressif/esp-idf.git esp-idf-v4.4
cd esp-idf-v4.4
bash install.sh
source export.sh
```

### HomeKit ADK
ESP-IDF currently uses MbedTLS 2.16.x, whereas HomeKit ADK requires 2.18. A branch mbedtls-2.16.6-adk is being maintained [here](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk) which has the required patches from 2.18, on top of 2.16.6.
```
cd $IDF_PATH/components/mbedtls/mbedtls
git pull
git checkout -b mbedtls-2.16.6-adk origin/mbedtls-2.16.6-adk
cd $IDF_PATH/../
git clone --recursive https://github.com/espressif/esp-apple-homekit-adk.git
```

### HomeKit Setup Code
```
cd $IDF_PATH/../esp-apple-homekit-adk/homekit_adk
make tools
cd Tools
./provision_raspi.sh --category 14 --setup-code 133-71-337 --setup-id DDSK Dreamdesk
cd Dreamdesk
cp ../../../tools/accessory_setup/accessory_setup.csv .
python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate accessory_setup.csv accessory_setup.bin 0x6000
esptool.py -p $ESPPORT write_flash 0x340000 accessory_setup.bin
```

### Resetting HomeKit Pairing

The accessory pairing information is stored in the NVS (Non Volatile Storage) partition. Once paired, the accessory cannot be paired again, without clearing this pairing information first. It can be done as below:

```
esptool.py -p $ESPPORT erase_region 0x10000 0x6000
```

### Dreamdesk
```
cd $IDF_PATH/../
git clone --recursive https://github.com/ma-lwa-re/dreamdesk.git
cd dreamdesk
idf.py set-target esp32s3
export ESPPORT=/dev/tty.usbserial-130
idf.py build flash
```

## Console output
```
sudo cu -l /dev/cu.usbserial-130 -s 115200
```

## Project
```
dreamdesk
├── CMakeLists.txt
├── README.md
├── main
│   ├── CMakeLists.txt
│   ├── dreamdesk.c
│   ├── dreamdesk.h
│   ├── homekit.c
│   ├── homekit.h
│   ├── ikea.c
│   ├── ikea.h
│   ├── lin.c
│   ├── lin.h
│   ├── logicdata.c
│   ├── logicdata.h
│   ├── main.c
│   ├── wifi.c
│   └── wifi.h.default
├── partitions.csv
├── sdkconfig
└── sdkconfig.defaults
```

## TODO
- [ ] IKEA desk testing
- [ ] PCB prototype
- [ ] PCB assembly
- [ ] HomeKit memory integration
- [ ] HomeKit sensors (humidity + air + temperature)
- [ ] Google Home support
- [ ] Amazon Alexa support
- [ ] Dump Logicdata firwmare

## Feedback & Issues

Issues and feature requests should be raised on GitHub using

- https://github.com/ma-lwa-re/dreamdesk/issues/new