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
//#ifndef DB_H
//#define DB_H

#include "HAP.h"

#define HOMEKIT_STACK_SIZE         (6144)

/**
 * Total number of services and characteristics contained in the accessory.
 */
#define kAttributeCount ((size_t) 21)

void home_task(void *arg);

/**
 * HomeKit Accessory Information service.
 */
extern const HAPService accessoryInformationService;

/**
 * Characteristics to expose accessory information and configuration (associated with Accessory Information service).
 */
extern const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic;
extern const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic;
extern const HAPStringCharacteristic accessoryInformationModelCharacteristic;
extern const HAPStringCharacteristic accessoryInformationNameCharacteristic;
extern const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic;
extern const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic;

/**
 * HAP Protocol Information service.
 */
extern const HAPService hapProtocolInformationService;

/**
 * Pairing service.
 */
extern const HAPService pairingService;

/**
 * Light Bulb service.
 */
extern const HAPService lightBulbService;

/**
 * The 'On' characteristic of the Light Bulb service.
 */
extern const HAPBoolCharacteristic lightBulbOnCharacteristic;

extern const HAPIntCharacteristic lightBulbBrightnessCharacteristic;

/**
 * Identify routine. Used to locate the accessory.
 */
HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServerRef* server,
        const HAPAccessoryIdentifyRequest* request,
        void* _Nullable context);

/**
 * Handle read request to the 'On' characteristic of the Light Bulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCurrentPositionRead(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicReadRequest* request,
        int* value,
        void* _Nullable context);

/**
 * Handle write request to the 'On' characteristic of the Light Bulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCurrentPositionWrite(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicWriteRequest* request,
        int value,
        void* _Nullable context);

HAP_RESULT_USE_CHECK
HAPError HandleTargetPositionRead(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicReadRequest* request,
        int* value,
        void* _Nullable context);

/**
 * Handle write request to the 'On' characteristic of the Light Bulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleTargetPositionWrite(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicWriteRequest* request,
        int value,
        void* _Nullable context);

HAP_RESULT_USE_CHECK
HAPError HandlePositionStateRead(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicReadRequest* request,
        int* value,
        void* _Nullable context);

/**
 * Handle write request to the 'On' characteristic of the Light Bulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandlePositionStateWrite(
        HAPAccessoryServerRef* server,
        const HAPIntCharacteristicWriteRequest* request,
        int value,
        void* _Nullable context);

/**
 * Initialize the application.
 */
void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Deinitialize the application.
 */
void AppRelease(void);

/**
 * Start the accessory server for the app.
 */
void AppAccessoryServerStart(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context);

void AccessoryServerHandleSessionAccept(HAPAccessoryServerRef* server, HAPSessionRef* session, void* _Nullable context);

void AccessoryServerHandleSessionInvalidate(
        HAPAccessoryServerRef* server,
        HAPSessionRef* session,
        void* _Nullable context);

/**
 * Restore platform specific factory settings.
 */
void RestorePlatformFactorySettings(void);

/**
 * Returns pointer to accessory information
 */
const HAPAccessory* AppGetAccessoryInfo();