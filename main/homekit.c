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
#include "homekit.h"
#include "dreamdesk.h"
#include "sensors.h"
#include "math.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformAccessorySetup+Init.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"
#include "HAPPlatformKeyValueStore+Init.h"
#include "HAPPlatformMFiHWAuth+Init.h"
#include "HAPPlatformMFiTokenAuth+Init.h"
#include "HAPPlatformRunLoop+Init.h"
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"

#define kAppKeyValueStoreDomain_Configuration           ((HAPPlatformKeyValueStoreDomain) 0x00)
#define kAppKeyValueStoreKey_Configuration_State        ((HAPPlatformKeyValueStoreDomain) 0x00)

bool requestedFactoryReset = false;
bool clearPairings = false;

static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPPlatformKeyValueStore factoryKeyValueStore;
    HAPAccessoryServerOptions hapAccessoryServerOptions;
    HAPPlatform hapPlatform;
    HAPAccessoryServerCallbacks hapAccessoryServerCallbacks;
    HAPPlatformTCPStreamManager tcpStreamManager;
    HAPPlatformMFiHWAuth mfiHWAuth;
    HAPPlatformMFiTokenAuth mfiTokenAuth;
} platform;

HAPAccessoryServerRef accessoryServer;
AccessoryConfigurationT accessoryConfiguration;

const HAPService accessoryInformationService = {
    .iid = kIID_AccessoryInformation,
    .serviceType = &kHAPServiceType_AccessoryInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessoryInformationFirmwareRevisionCharacteristic,
                                                            &accessoryInformationIdentifyCharacteristic,
                                                            &accessoryInformationManufacturerCharacteristic,
                                                            &accessoryInformationModelCharacteristic,
                                                            &accessoryInformationNameCharacteristic,
                                                            &accessoryInformationSerialNumberCharacteristic,
                                                            NULL }
};

const HAPService hapProtocolInformationService = {
    .iid = kIID_HAPProtocolInformation,
    .serviceType = &kHAPServiceType_HAPProtocolInformation,
    .debugDescription = kHAPServiceDebugDescription_HAPProtocolInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = true } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &hapProtocolInformationServiceSignatureCharacteristic,
                                                            &hapProtocolInformationVersionCharacteristic,
                                                            NULL }
};

const HAPService pairingService = {
    .iid = kIID_Pairing,
    .serviceType = &kHAPServiceType_Pairing,
    .debugDescription = kHAPServiceDebugDescription_Pairing,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &pairingPairSetupCharacteristic,
                                                            &pairingPairVerifyCharacteristic,
                                                            &pairingPairingFeaturesCharacteristic,
                                                            &pairingPairingPairingsCharacteristic,
                                                            NULL }
};

const HAPService dreamdeskService = {
    .iid = kIID_Window,
    .serviceType = &kHAPServiceType_WindowCovering,
    .debugDescription = kHAPServiceDebugDescription_WindowCovering,
    .name = "Dreamdesk Remote Control",
    .properties = { .primaryService = true, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &dreamdeskServiceSignatureCharacteristic,
                                                            &dreamdeskNameCharacteristic,
                                                            &dreamdeskCurrentPositionCharacteristic,
                                                            &dreamdeskTargetPositionCharacteristic,
                                                            &dreamdeskPositionStateCharacteristic,
                                                            NULL }
};

const HAPService temperatureSensorService = {
    .iid = kIID_TemperatureSensor,
    .serviceType = &kHAPServiceType_TemperatureSensor,
    .debugDescription = kHAPServiceDebugDescription_TemperatureSensor,
    .name = "Dreamdesk Temperature",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &temperatureSensorServiceSignatureCharacteristic,
                                                            &temperatureSensorNameCharacteristic,
                                                            &temperatureSensorCurrentTemperatureCharacteristic,
                                                            &temperatureSensorTemperatureDisplayUnitsCharacteristic,
                                                            NULL }
};

const HAPService humiditySensorService = {
    .iid = kIID_HumiditySensor,
    .serviceType = &kHAPServiceType_HumiditySensor,
    .debugDescription = kHAPServiceDebugDescription_HumiditySensor,
    .name = "Dreamdesk Humidity",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &humiditySensorServiceSignatureCharacteristic,
                                                            &humiditySensorNameCharacteristic,
                                                            &humiditySensorCurrentRelativeHumidityCharacteristic,
                                                            NULL }
};

const HAPService carbonDioxideSensorService = {
    .iid = kIID_CarbonDioxideSensor,
    .serviceType = &kHAPServiceType_CarbonDioxideSensor,
    .debugDescription = kHAPServiceDebugDescription_CarbonDioxideSensor,
    .name = "Dreamdesk CO₂",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &carbonDioxideSensorServiceSignatureCharacteristic,
                                                            &carbonDioxideDetectedCharacteristic,
                                                            &carbonDioxideSensorNameCharacteristic,
                                                            &carbonDioxideStatusActiveCharacteristic,
                                                            &carbonDioxideLevelCharacteristic,
                                                            &carbonDioxidePeakLevelCharacteristic,
                                                            NULL }
};

const HAPService airQualitySensorService = {
    .iid = kIID_AirQualitySensor,
    .serviceType = &kHAPServiceType_AirQualitySensor,
    .debugDescription = kHAPServiceDebugDescription_AirQualitySensor,
    .name = "Dreamdesk Air Quality",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &airQualitySensorServiceSignatureCharacteristic,
                                                            &airQualitySensorNameCharacteristic,
                                                            &airQualitySensorAirQualityCharacteristic,
                                                            NULL }
};

const HAPAccessory accessory = { .aid = 0x01,
                                  .category = kHAPAccessoryCategory_WindowCoverings,
                                  .name = "Dreamdesk",
                                  .manufacturer = "ma.lwa.re",
                                  .model = "Dreamdesk v2.5",
                                  .serialNumber = "DEADBEEFBABE",
                                  .firmwareVersion = "2.5",
                                  .hardwareVersion = "2.5",
                                  .services = (const HAPService* const[]) {
                                      &accessoryInformationService, &hapProtocolInformationService,
                                      &pairingService, &dreamdeskService,
                                      #if defined(SENSORS_ON)
                                      &temperatureSensorService, &humiditySensorService,
                                      &carbonDioxideSensorService, &airQualitySensorService,
                                      #endif
                                      NULL
                                  },
                                  .callbacks = { .identify = IdentifyAccessory }
};

const HAPAccessory* AppGetAccessoryInfo() {
    return &accessory;
}

void LoadAccessoryState() {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    bool found;
    size_t numBytes;

    HAPError err = HAPPlatformKeyValueStoreGet(
        accessoryConfiguration.keyValueStore,
        kAppKeyValueStoreDomain_Configuration,
        kAppKeyValueStoreKey_Configuration_State,
        &accessoryConfiguration.state,
        sizeof accessoryConfiguration.state,
        &numBytes,
        &found);

    if(err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    if(!found || numBytes != sizeof accessoryConfiguration.state) {

        if(found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    }
}

void SaveAccessoryState() {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err = HAPPlatformKeyValueStoreSet(
        accessoryConfiguration.keyValueStore,
        kAppKeyValueStoreDomain_Configuration,
        kAppKeyValueStoreKey_Configuration_State,
        &accessoryConfiguration.state,
        sizeof accessoryConfiguration.state);
    if(err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!context);

    switch(HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

HAP_RESULT_USE_CHECK HAPError IdentifyAccessory(HAPAccessoryServerRef* server HAP_UNUSED,
                                                const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
                                                void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCurrentPositionRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                        int* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.current_position = accessoryConfiguration.state.target_position;
    *value = accessoryConfiguration.state.current_position;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleTargetPositionRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                       const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                       int* value, void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.target_position;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleTargetPositionWrite(HAPAccessoryServerRef* server,
                                                        const HAPIntCharacteristicWriteRequest* request,
                                                        int value, void* _Nullable context HAP_UNUSED) {
    if(accessoryConfiguration.state.target_position != value) {
        accessoryConfiguration.state.target_position = value;
        desk_set_target_percentage(value);
        accessoryConfiguration.state.current_position = value;

        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandlePositionStateRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                      const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                      int* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.position_state = 0x02;
    *value = accessoryConfiguration.state.position_state;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCurrentTemperatureRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                           const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
                                                           float* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.current_temperature = round(get_current_temperature() * 10.0) / 10.0;
    *value = accessoryConfiguration.state.current_temperature;
    HAPLogInfo(&kHAPLog_Default, "%s: %f", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleTemperatureDisplayUnitsRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                                const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                                int* value, void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.display_units;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCurrentHumidityRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                        const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
                                                        float* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.current_relative_humidity = round(get_current_relative_humidity());
    *value = accessoryConfiguration.state.current_relative_humidity;
    HAPLogInfo(&kHAPLog_Default, "%s: %f", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideDetectedRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                              const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                              int* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.co2_detected = get_air_quality() == POOR ? 0x01 : 0x00;
    *value = accessoryConfiguration.state.co2_detected;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideStatusActiveRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                                  const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
                                                                  bool* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.co2_active = true;
    *value = accessoryConfiguration.state.co2_active;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxideLevelRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                          const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
                                                          float* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.co2_level = round(get_co2_level());
    *value = accessoryConfiguration.state.co2_level;
    HAPLogInfo(&kHAPLog_Default, "%s: %f", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleAirQualityRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                   const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                                   int* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.air_quality = get_air_quality();
    *value = accessoryConfiguration.state.air_quality;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK HAPError HandleCarbonDioxidePeakLevelRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                                               const HAPFloatCharacteristicReadRequest* request HAP_UNUSED,
                                                               float* value, void* _Nullable context HAP_UNUSED) {
    accessoryConfiguration.state.co2_peak_level = round(get_co2_peak_level());
    *value = accessoryConfiguration.state.co2_peak_level;
    HAPLogInfo(&kHAPLog_Default, "%s: %f", __func__, *value);
    return kHAPError_None;
}

void AccessoryNotification(const HAPAccessory* accessory, const HAPService* service,
                           const HAPCharacteristic* characteristic, void* ctx) {
    HAPLogInfo(&kHAPLog_Default, "Accessory Notification");
    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void HandleUpdatedState(HAPAccessoryServerRef* _Nonnull server, void* _Nullable context) {

    if(HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && requestedFactoryReset) {
        HAPPrecondition(server);
        HAPLogInfo(&kHAPLog_Default, "A factory reset has been requested.");

        HAPError err = HAPPlatformKeyValueStorePurgeDomain(&platform.keyValueStore, ((HAPPlatformKeyValueStoreDomain) 0x00));

        if(err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        err = HAPRestoreFactorySettings(&platform.keyValueStore);

        if(err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        requestedFactoryReset = false;

        AppCreate(server, &platform.keyValueStore);
        AppAccessoryServerStart();
    } else if (HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && clearPairings) {
        HAPError err = HAPRemoveAllPairings(&platform.keyValueStore);

        if(err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        AppAccessoryServerStart();
    } else {
        AccessoryServerHandleUpdatedState(server, context);
    }
}

void InitializePlatform() {
    HAPPlatformKeyValueStoreCreate(&platform.keyValueStore, &(const HAPPlatformKeyValueStoreOptions) {
        .part_name = "nvs",
        .namespace_prefix = "hap",
        .read_only = false
    });
    platform.hapPlatform.keyValueStore = &platform.keyValueStore;

    HAPPlatformKeyValueStoreCreate(&platform.factoryKeyValueStore, &(const HAPPlatformKeyValueStoreOptions) {
        .part_name = "fctry",
        .namespace_prefix = "hap",
        .read_only = true
    });

    static HAPPlatformAccessorySetup accessorySetup;
    HAPPlatformAccessorySetupCreate(&accessorySetup, &(const HAPPlatformAccessorySetupOptions) {
        .keyValueStore = &platform.factoryKeyValueStore });
    platform.hapPlatform.accessorySetup = &accessorySetup;

    HAPPlatformTCPStreamManagerCreate(&platform.tcpStreamManager, &(const HAPPlatformTCPStreamManagerOptions) {
        .port = 0,
        .maxConcurrentTCPStreams = 9
    });

    static HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformServiceDiscoveryCreate(&serviceDiscovery, &(const HAPPlatformServiceDiscoveryOptions) {0});
    platform.hapPlatform.ip.serviceDiscovery = &serviceDiscovery;

    HAPPlatformMFiTokenAuthCreate(&platform.mfiTokenAuth, &(const HAPPlatformMFiTokenAuthOptions)
                                  { .keyValueStore = &platform.keyValueStore });

    HAPPlatformRunLoopCreate(&(const HAPPlatformRunLoopOptions) { .keyValueStore = &platform.keyValueStore });

    platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    platform.hapPlatform.authentication.mfiTokenAuth =
            HAPPlatformMFiTokenAuthIsProvisioned(&platform.mfiTokenAuth) ? &platform.mfiTokenAuth : NULL;

   platform.hapAccessoryServerCallbacks.handleUpdatedState = HandleUpdatedState;
}

void DeinitializePlatform() {
    HAPPlatformTCPStreamManagerRelease(&platform.tcpStreamManager);
    HAPPlatformRunLoopRelease();
}

void InitializeIP() {
    static HAPIPSession ipSessions[kHAPIPSessionStorage_MinimumNumElements];
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumInboundBufferSize];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumOutboundBufferSize];
    static HAPIPEventNotificationRef ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];

    for(size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }

    static HAPIPReadContextRef ipReadContexts[kAttributeCount];
    static HAPIPWriteContextRef ipWriteContexts[kAttributeCount];
    static uint8_t ipScratchBuffer[kHAPIPSession_MinimumScratchBufferSize];
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer }
    };

    platform.hapAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    platform.hapAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;
    platform.hapPlatform.ip.tcpStreamManager = &platform.tcpStreamManager;
}

void home_task(void *arg) {
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    InitializePlatform();
    InitializeIP();

    HAPAccessoryServerCreate(&accessoryServer, &platform.hapAccessoryServerOptions,
                             &platform.hapPlatform, &platform.hapAccessoryServerCallbacks, NULL);
    AppCreate(&accessoryServer, &platform.keyValueStore);

    AppAccessoryServerStart();
    HAPPlatformRunLoopRun();

    HAPAccessoryServerRelease(&accessoryServer);
    DeinitializePlatform();
}
