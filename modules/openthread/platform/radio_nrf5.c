/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction
 *   for radio communication utilizing nRF IEEE802.15.4 radio driver.
 *
 */

#include <openthread/error.h>
#define LOG_MODULE_NAME net_otPlat_radio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>

#include <openthread/ip6.h>
#include <openthread-system.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>
#include <openthread/message.h>
#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif

#include "nrf_802154.h"
#include "nrf_802154_const.h"

/******************************************************/
/* TODO: To remove once L2 layer dependency is removed */
#include <zephyr/net/ieee802154_radio.h>
/******************************************************/

/* Init, process and other functions required by platform.c */

void platformRadioInit(void)
{
}

void platformRadioProcess(otInstance *aInstance)
{
}

uint16_t platformRadioChannelGet(otInstance *aInstance)
{
}

#if defined(CONFIG_OPENTHREAD_DIAG)
void platformRadioChannelSet(uint8_t aChannel)
{
}
#endif

/* Radio configuration */

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
}

const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
}


void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
}

void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanId)
{
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
}

void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
}

otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
}

otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
}

void otPlatRadioSetRxOnWhenIdle(otInstance *aInstance, bool aEnable)
{
}

void otPlatRadioSetMacKey(otInstance *aInstance, uint8_t aKeyIdMode, uint8_t aKeyId,
			  const otMacKeyMaterial *aPrevKey, const otMacKeyMaterial *aCurrKey,
			  const otMacKeyMaterial *aNextKey, otRadioKeyType aKeyType)
{
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
}

void otPlatRadioSetMacFrameCounterIfLarger(otInstance *aInstance, uint32_t aMacFrameCounter)
{
}

/* Radio operations */

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
}

uint32_t otPlatRadioGetBusSpeed(otInstance *aInstance)
{
}

uint32_t otPlatRadioGetBusLatency(otInstance *aInstance)
{
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
}

otError otPlatRadioEnable(otInstance *aInstance)
{
}

otError otPlatRadioDisable(otInstance *aInstance)
{
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
}

otError otPlatRadioSleep(otInstance *aInstance)
{
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
}

otError otPlatRadioReceiveAt(otInstance *aInstance, uint8_t aChannel, uint32_t aStart,
			     uint32_t aDuration)
{
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
}

uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
}

otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
}

otError otPlatRadioEnableCsl(otInstance *aInstance, uint32_t aCslPeriod, otShortAddress aShortAddr,
			     const otExtAddress *aExtAddr)
{
}

otError otPlatRadioResetCsl(otInstance *aInstance)
{
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
}

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
}

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel,
					      int8_t aMaxPower)
{
}

otError otPlatRadioSetRegion(otInstance *aInstance, uint16_t aRegionCode)
{
}

otError otPlatRadioGetRegion(otInstance *aInstance, uint16_t *aRegionCode)
{
}

otError otPlatRadioConfigureEnhAckProbing(otInstance *aInstance, otLinkMetrics aLinkMetrics,
					  otShortAddress aShortAddress,
					  const otExtAddress *aExtAddress)
{
}

otError otPlatRadioAddCalibratedPower(otInstance *aInstance, uint8_t aChannel, int16_t aActualPower,
				      const uint8_t *aRawPowerSetting,
				      uint16_t aRawPowerSettingLength)
{
}

otError otPlatRadioClearCalibratedPowers(otInstance *aInstance)
{
}

otError otPlatRadioSetChannelTargetPower(otInstance *aInstance, uint8_t aChannel,
					 int16_t aTargetPower)
{
}

/* Platform related */

#if defined(CONFIG_IEEE802154_CARRIER_FUNCTIONS)
otError platformRadioTransmitCarrier(otInstance *aInstance, bool aEnable)
{
}

otError platformRadioTransmitModulatedCarrier(otInstance *aInstance, bool aEnable,
					      const uint8_t *aData)
{
}

#endif /* CONFIG_IEEE802154_CARRIER_FUNCTIONS */
