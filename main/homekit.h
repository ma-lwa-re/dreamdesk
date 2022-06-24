/* MIT License
*
* Copyright (c) 2022 ma-lwa-re
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "HAP.h"

#define HOMEKIT_STACK_SIZE                              (8192)
#define kAttributeCount                                 ((size_t) 21)

#define kIID_AccessoryInformation                       ((uint64_t) 0x01)
#define kIID_AccessoryInformationIdentify               ((uint64_t) 0x02)
#define kIID_AccessoryInformationManufacturer           ((uint64_t) 0x03)
#define kIID_AccessoryInformationModel                  ((uint64_t) 0x04)
#define kIID_AccessoryInformationName                   ((uint64_t) 0x05)
#define kIID_AccessoryInformationSerialNumber           ((uint64_t) 0x06)
#define kIID_AccessoryInformationFirmwareRevision       ((uint64_t) 0x07)
#define kIID_AccessoryInformationHardwareRevision       ((uint64_t) 0x08)
#define kIID_AccessoryInformationADKVersion             ((uint64_t) 0x09)
#define kIID_AccessoryInformationProductData            ((uint64_t) 0x0A)

#define kIID_HAPProtocolInformation                     ((uint64_t) 0x10)
#define kIID_HAPProtocolInformationServiceSignature     ((uint64_t) 0x11)
#define kIID_HAPProtocolInformationVersion              ((uint64_t) 0x12)

#define kIID_Pairing                                    ((uint64_t) 0x20)
#define kIID_PairingPairSetup                           ((uint64_t) 0x22)
#define kIID_PairingPairVerify                          ((uint64_t) 0x23)
#define kIID_PairingPairingFeatures                     ((uint64_t) 0x24)
#define kIID_PairingPairingPairings                     ((uint64_t) 0x25)

#define kIID_Window                                     ((uint64_t) 0x30)
#define kIID_WindowServiceSignature                     ((uint64_t) 0x31)
#define kIID_WindowName                                 ((uint64_t) 0x32)
#define kIID_WindowCurrentPosition                      ((uint64_t) 0x33)
#define kIID_WindowTargetPosition                       ((uint64_t) 0x34)
#define kIID_WindowPositionState                        ((uint64_t) 0x35)

#define kIID_TemperatureSensor                          ((uint64_t) 0x40)
#define kIID_TemperatureSensorSignature                 ((uint64_t) 0x41)
#define kIID_TemperatureSensorCurrentTemperature        ((uint64_t) 0x43)
#define kIID_TemperatureSensorTemperatureDisplayUnits   ((uint64_t) 0x44)
#define kIID_TemperatureSensorName                      ((uint64_t) 0x42)

#define kIID_HumiditySensor                             ((uint64_t) 0x50)
#define kIID_HumiditySensorSignature                    ((uint64_t) 0x51)
#define kIID_HumiditySensorCurrentRelativeHumidity      ((uint64_t) 0x53)
#define kIID_HumiditySensorName                         ((uint64_t) 0x52)

#define kIID_CarbonDioxideSensor                        ((uint64_t) 0x60)
#define kIID_CarbonDioxideSensorSignature               ((uint64_t) 0x61)
#define kIID_CarbonDioxideSensorName                    ((uint64_t) 0x62)
#define kIID_CarbonDioxideSensorStatusActive            ((uint64_t) 0x63)
#define kIID_CarbonDioxideSensorDetected                ((uint64_t) 0x64)
#define kIID_CarbonDioxideSensorLevel                   ((uint64_t) 0x65)
#define kIID_CarbonDioxideSensorPeakLevel               ((uint64_t) 0x66)

#define kIID_AirQualitySensor                           ((uint64_t) 0x70)
#define kIID_AirQualitySensorSignature                  ((uint64_t) 0x71)
#define kIID_AirQualitySensorName                       ((uint64_t) 0x72)
#define kIID_AirQualitySensorAirQuality                 ((uint64_t) 0x73)

typedef struct AccessoryConfiguration {
    struct {
        int current_position;
        int target_position;
        uint8_t position_state;
        float current_temperature;
        uint8_t display_units;
        float current_relative_humidity;
        uint8_t air_quality;
        uint8_t co2_detected;
        bool co2_active;
        float co2_level;
        float co2_peak_level;
    } state;
    HAPAccessoryServerRef *server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfigurationT;

HAP_RESULT_USE_CHECK HAPError IdentifyAccessory(HAPAccessoryServerRef* server,
                                                const HAPAccessoryIdentifyRequest* request,
                                                void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCurrentPositionRead(HAPAccessoryServerRef* server,
                                                        const HAPIntCharacteristicReadRequest* request,
                                                        int* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleTargetPositionRead(HAPAccessoryServerRef* server,
                                                       const HAPIntCharacteristicReadRequest* request,
                                                       int* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleTargetPositionWrite(HAPAccessoryServerRef* server,
                                                        const HAPIntCharacteristicWriteRequest* request,
                                                        int value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandlePositionStateRead(HAPAccessoryServerRef* server,
                                                      const HAPIntCharacteristicReadRequest* request,
                                                      int* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCurrentTemperatureRead(HAPAccessoryServerRef* server,
                                                           const HAPFloatCharacteristicReadRequest* request,
                                                           float* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleTemperatureDisplayUnitsRead(HAPAccessoryServerRef* server,
                                                                const HAPIntCharacteristicReadRequest* request,
                                                                int* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCurrentHumidityRead(HAPAccessoryServerRef* server,
                                                        const HAPFloatCharacteristicReadRequest* request,
                                                        float* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideDetectedRead(HAPAccessoryServerRef* server,
                                                             const HAPIntCharacteristicReadRequest* request,
                                                             int* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideStatusActiveRead(HAPAccessoryServerRef* server,
                                                                  const HAPBoolCharacteristicReadRequest* request,
                                                                  bool* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideLevelRead(HAPAccessoryServerRef* server,
                                                           const HAPFloatCharacteristicReadRequest* request,
                                                           float* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxidePeakLevelRead(HAPAccessoryServerRef* server,
                                                               const HAPFloatCharacteristicReadRequest* request,
                                                               float* value, void* _Nullable context);

HAP_RESULT_USE_CHECK HAPError HandleAirQualityRead(HAPAccessoryServerRef* server,
                                                   const HAPIntCharacteristicReadRequest* request,
                                                   int* value, void* _Nullable context);

static const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationFirmwareRevision,
    .characteristicType = &kHAPCharacteristicType_FirmwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationFirmwareRevisionRead, .handleWrite = NULL }
};

static const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AccessoryInformationIdentify,
    .characteristicType = &kHAPCharacteristicType_Identify,
    .debugDescription = kHAPCharacteristicDebugDescription_Identify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = NULL, .handleWrite = HAPHandleAccessoryInformationIdentifyWrite }
};

static const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationManufacturer,
    .characteristicType = &kHAPCharacteristicType_Manufacturer,
    .debugDescription = kHAPCharacteristicDebugDescription_Manufacturer,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationManufacturerRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic accessoryInformationModelCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationModel,
    .characteristicType = &kHAPCharacteristicType_Model,
    .debugDescription = kHAPCharacteristicDebugDescription_Model,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationModelRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic accessoryInformationNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationNameRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationSerialNumber,
    .characteristicType = &kHAPCharacteristicType_SerialNumber,
    .debugDescription = kHAPCharacteristicDebugDescription_SerialNumber,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationSerialNumberRead, .handleWrite = NULL }
};

static const HAPDataCharacteristic hapProtocolInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HAPProtocolInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic hapProtocolInformationVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HAPProtocolInformationVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleHAPProtocolInformationVersionRead, .handleWrite = NULL }
};

static const HAPTLV8Characteristic pairingPairSetupCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairSetup,
    .characteristicType = &kHAPCharacteristicType_PairSetup,
    .debugDescription = kHAPCharacteristicDebugDescription_PairSetup,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairSetupRead, .handleWrite = HAPHandlePairingPairSetupWrite }
};

static const HAPTLV8Characteristic pairingPairVerifyCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairVerify,
    .characteristicType = &kHAPCharacteristicType_PairVerify,
    .debugDescription = kHAPCharacteristicDebugDescription_PairVerify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairVerifyRead, .handleWrite = HAPHandlePairingPairVerifyWrite }
};

static const HAPUInt8Characteristic pairingPairingFeaturesCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_PairingPairingFeatures,
    .characteristicType = &kHAPCharacteristicType_PairingFeatures,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingFeatures,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 0,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandlePairingPairingFeaturesRead, .handleWrite = NULL }
};

static const HAPTLV8Characteristic pairingPairingPairingsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairingPairings,
    .characteristicType = &kHAPCharacteristicType_PairingPairings,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingPairings,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandlePairingPairingPairingsRead,
                   .handleWrite = HAPHandlePairingPairingPairingsWrite }
};

static const HAPDataCharacteristic dreamdeskServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_WindowServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic dreamdeskNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_WindowName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

static const HAPIntCharacteristic dreamdeskCurrentPositionCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowCurrentPosition,
    .characteristicType = &kHAPCharacteristicType_CurrentPosition,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentPosition,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                     .units = kHAPCharacteristicUnits_None,
                     .constraints = { .minimumValue = 0,
                                      .maximumValue = 100,
                                      .stepValue = 1 },
    .callbacks = { .handleRead = HandleCurrentPositionRead, .handleWrite = NULL }
};

static const HAPIntCharacteristic dreamdeskTargetPositionCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowTargetPosition,
    .characteristicType = &kHAPCharacteristicType_TargetPosition,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetPosition,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                     .units = kHAPCharacteristicUnits_None,
                     .constraints = { .minimumValue = 0,
                                      .maximumValue = 100,
                                      .stepValue = 1 },
    .callbacks = { .handleRead = HandleTargetPositionRead, .handleWrite = HandleTargetPositionWrite }
};

static const HAPIntCharacteristic dreamdeskPositionStateCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_WindowPositionState,
    .characteristicType = &kHAPCharacteristicType_PositionState,
    .debugDescription = kHAPCharacteristicDebugDescription_PositionState,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 2,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandlePositionStateRead, .handleWrite = NULL }
};

static const HAPDataCharacteristic temperatureSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_TemperatureSensorSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic temperatureSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_TemperatureSensorName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

static const HAPFloatCharacteristic temperatureSensorCurrentTemperatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_TemperatureSensorCurrentTemperature,
    .characteristicType = &kHAPCharacteristicType_CurrentTemperature,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTemperature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 100,
                                     .stepValue = .1 },
    .callbacks = { .handleRead = HandleCurrentTemperatureRead, .handleWrite = NULL }
};

static const HAPIntCharacteristic temperatureSensorTemperatureDisplayUnitsCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_TemperatureSensorTemperatureDisplayUnits,
    .characteristicType = &kHAPCharacteristicType_TemperatureDisplayUnits,
    .debugDescription = kHAPCharacteristicDebugDescription_TemperatureDisplayUnits,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 1,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandleTemperatureDisplayUnitsRead, .handleWrite = NULL }
};

static const HAPDataCharacteristic humiditySensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HumiditySensorSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic humiditySensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HumiditySensorName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

static const HAPFloatCharacteristic humiditySensorCurrentRelativeHumidityCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_HumiditySensorCurrentRelativeHumidity,
    .characteristicType = &kHAPCharacteristicType_CurrentRelativeHumidity,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentRelativeHumidity,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 100,
                                     .stepValue = .1 },
    .callbacks = { .handleRead = HandleCurrentHumidityRead, .handleWrite = NULL }
};

static const HAPDataCharacteristic carbonDioxideSensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_CarbonDioxideSensorSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPIntCharacteristic carbonDioxideDetectedCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_CarbonDioxideSensorDetected,
    .characteristicType = &kHAPCharacteristicType_CarbonDioxideDetected,
    .debugDescription = kHAPCharacteristicDebugDescription_CarbonDioxideDetected,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 1,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandleCarbonDioxideDetectedRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic carbonDioxideSensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_CarbonDioxideSensorName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

static const HAPBoolCharacteristic carbonDioxideStatusActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_CarbonDioxideSensorStatusActive,
    .characteristicType = &kHAPCharacteristicType_StatusActive,
    .debugDescription = kHAPCharacteristicDebugDescription_StatusActive,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HandleCarbonDioxideStatusActiveRead, .handleWrite = NULL }
};

static const HAPFloatCharacteristic carbonDioxideLevelCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_CarbonDioxideSensorLevel,
    .characteristicType = &kHAPCharacteristicType_CarbonDioxideLevel,
    .debugDescription = kHAPCharacteristicDebugDescription_CarbonDioxideLevel,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 100000,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandleCarbonDioxideLevelRead, .handleWrite = NULL }
};

static const HAPFloatCharacteristic carbonDioxidePeakLevelCharacteristic = {
    .format = kHAPCharacteristicFormat_Float,
    .iid = kIID_CarbonDioxideSensorPeakLevel,
    .characteristicType = &kHAPCharacteristicType_CarbonDioxidePeakLevel,
    .debugDescription = kHAPCharacteristicDebugDescription_CarbonDioxidePeakLevel,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 100000,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandleCarbonDioxidePeakLevelRead, .handleWrite = NULL }
};

static const HAPDataCharacteristic airQualitySensorServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_AirQualitySensorSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic airQualitySensorNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AirQualitySensorName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

static const HAPIntCharacteristic airQualitySensorAirQualityCharacteristic = {
    .format = kHAPCharacteristicFormat_Int,
    .iid = kIID_AirQualitySensorAirQuality,
    .characteristicType = &kHAPCharacteristicType_AirQuality,
    .debugDescription = kHAPCharacteristicDebugDescription_AirQuality,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
                    .constraints = { .minimumValue = 0,
                                     .maximumValue = 5,
                                     .stepValue = 1 },
    .callbacks = { .handleRead = HandleAirQualityRead, .handleWrite = NULL }
};

void home_task(void *arg);
