# The Dreamdesk Project
The aim of this project was to create a framework that makes compatible any electrical standing desk with a home automation solution like Apple Homekit, Google Home, or Amazon Alexa, allowing your desk to be controlled by your phone, a computer, or a home speaker.

It has been tested and currently works with any desk relying on the Dynamic Motion system by Logicdata, like the Yaasa Desks, Gravit iDesk, or the Lidle LifeUP in addition to the IKEA Bekant desks.

The code was developped in C for the ESP32 microcontrollers family. A detailed write-up about the project is available at

- [`https://ma.lwa.re/dreamdesk`](https://ma.lwa.re/dreamdesk)

## Config
### Select Features
Edit the [`CMakeLists.txt`](CMakeLists.txt) file to select what kind of desk you want to control and other optional features.
The project version will be used to check if a newer version is available during the OTA update process.
```
# REQUIRED: Choose your desk type (LOGICDATA | IKEA)
set(DESK_TYPE "LOGICDATA")

# OPTIONAL: Choose your home automation ecosystem (HOMEKIT | NEST | ALEXA | NONE)
set(HOME_AUTOMATION "HOMEKIT")

# OPTIONAL: Enable sensors (ON | OFF)
set(SENSORS ON)

# OPTIONAL: Set the temperature scale (C | F | K)
set(SENSORS_SCALE "C")

# OPTIONAL: Enable Over The Air (OTA) updates (ON | OFF)
set(OTA_UPDATES ON)

# OPTIONAL: Set the project version
set(PROJECT_VER "2.4.0.2")
```

## Setup
### Espressif SDK
```
git clone -b v4.4 --recursive https://github.com/espressif/esp-idf.git esp-idf-v4.4
cd esp-idf-v4.4
bash install.sh
source export.sh
```

### HomeKit ADK
ESP-IDF currently uses MbedTLS 2.16.x, whereas HomeKit ADK requires 2.18. A branch mbedtls-2.16.6-adk is being maintained [`here`](https://github.com/espressif/mbedtls/tree/mbedtls-2.16.6-adk) which has the required patches from 2.18, on top of 2.16.6.
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
esptool.py -p $ESPPORT write_flash 0x34C000 accessory_setup.bin
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
export ESPPORT=/dev/cu.usbserial-0001
idf.py build flash
```

### Wi-Fi
The wifi credentials were previously hardcoded in the [`wifi.h`](main/wifi.h) header, but are now directly written in a standalone partition on the ESP32.

It has the advantage that no secrets are stored in the code section anymore, and thus allows the same code to run on multiple devices, over-the-air (OTA) updates, or modification of wifi credentials without having to reflash the device.

```
cp wifi.csv $IDF_PATH/components/nvs_flash/nvs_partition_generator
cd $IDF_PATH/components/nvs_flash/nvs_partition_generator
# Edit the wifi.csv and replace the DEFAULT_SSID and DEFAULT_PASSWORD strings
python3 nvs_partition_gen.py generate wifi.csv wifi.bin 0x3000
esptool.py -p $ESPPORT write_flash 0x340000 wifi.bin
```

### Code Signing
The integrity of the application can be secure and checked using an RSA signature scheme. The binary is signed after compilation with the private key that can be generated with `espsecure.py` or `openssl`, and the corresponding public key is embedded into the binary for verification.

```
espsecure.py generate_signing_key --version 2 secure_boot_signing_key.pem
```

The bootloader will be compiled with code to verify that an app is signed before booting it. In addition, the signature will be also proofed before updating the firmware and adds significant security against network-based attacks by preventing spoofing of OTA updates.

### Octal SPI Flash
If you're using a chip version that uses an Octal SPI interface to connect Flash/PSRAM, like the ESP32-S3-WROOM-2, you need to enable its support using the command below.

```
idf.py menuconfig
Serial flasher config ---> Enable Octal Flash
```

## Console Output
```
sudo cu -l $ESPPORT -s 115200
```

## Project
```
dreamdesk
├── CMakeLists.txt
├── LICENSE
├── README.md
├── lib
│   └── esp32-scd4x
│       ├── CMakeLists.txt
│       ├── LICENSE
│       ├── README.md
│       ├── images
│       │   └── SCD4x.png
│       ├── main.c
│       ├── scd4x.c
│       └── scd4x.h
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
│   ├── ota.c
│   ├── ota.h
│   ├── sensors.c
│   ├── sensors.h
│   ├── wifi.c
│   └── wifi.h
├── partitions.csv
├── sdkconfig
├── sdkconfig.defaults
└── wifi.csv
```

## TODO
- [ ] IKEA desk testing
- [x] PCB prototype
- [x] PCB assembly
- [x] OTA updates
- [ ] NVS encryption
- [ ] HomeKit memory integration
- [x] HomeKit sensors integration
- [x] Sensors (humidity + air + temperature)
- [ ] Google Home support
- [ ] Amazon Alexa support
- [ ] Dump Logicdata firwmare

## Feedback & Issues

Issues and feature requests should be raised on GitHub using

- [`https://github.com/ma-lwa-re/dreamdesk/issues/new`](https://github.com/ma-lwa-re/dreamdesk/issues/new)
