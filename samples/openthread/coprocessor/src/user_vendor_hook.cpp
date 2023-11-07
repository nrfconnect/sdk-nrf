/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *   This file shows how to implement the NCP vendor hook.
 */
#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK
#include "nrf_802154_radio_wrapper.h"
#include <zephyr/logging/log.h>
#include <ncp_base.hpp>
#include <ncp_hdlc.hpp>
#include <common/new.hpp>

#define VENDOR_SPINEL_PROP_VENDOR_NAME SPINEL_PROP_VENDOR__BEGIN
#define VENDOR_SPINEL_PROP_AUTO_ACK_ENABLED SPINEL_PROP_VENDOR__BEGIN + 1
#define VENDOR_SPINEL_PROP_HW_CAPABILITIES SPINEL_PROP_VENDOR__BEGIN + 2

LOG_MODULE_REGISTER(ncp_sample_vendor_hook, CONFIG_OT_COPROCESSOR_LOG_LEVEL);

namespace ot {
namespace Ncp {

otError NcpBase::VendorCommandHandler(uint8_t aHeader, unsigned int aCommand)
{
    otError error = OT_ERROR_NONE;

    LOG_DBG("VendorCommandHandler");

    switch (aCommand)
    {
        // TODO: Implement your command handlers here.

    default:
        error = PrepareLastStatusResponse(aHeader, SPINEL_STATUS_INVALID_COMMAND);
    }

    return error;
}

void NcpBase::VendorHandleFrameRemovedFromNcpBuffer(Spinel::Buffer::FrameTag aFrameTag)
{
    // This method is a callback which mirrors `NcpBase::HandleFrameRemovedFromNcpBuffer()`.
    // It is called when a spinel frame is sent and removed from NCP buffer.
    //
    // (a) This can be used to track and verify that a vendor spinel frame response is
    //     delivered to the host (tracking the frame using its tag).
    //
    // (b) It indicates that NCP buffer space is now available (since a spinel frame is
    //     removed). This can be used to implement reliability mechanisms to re-send
    //     a failed spinel command response (or an async spinel frame) transmission
    //     (failed earlier due to NCP buffer being full).

    OT_UNUSED_VARIABLE(aFrameTag);
}

otError NcpBase::VendorGetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
    case VENDOR_SPINEL_PROP_VENDOR_NAME:
        LOG_DBG("Got NAME get property request");
        error = mEncoder.WriteUtf8("Nordic Seminconductor");
        break;
    case VENDOR_SPINEL_PROP_AUTO_ACK_ENABLED:
        LOG_DBG("Got VENDOR_SPINEL_PROP_AUTO_ACK_ENABLED get property request");
        error = mEncoder.WriteUint8(nrf_802154_radio_wrapper_auto_ack_get());
        break;
    case VENDOR_SPINEL_PROP_HW_CAPABILITIES:
        LOG_DBG("Got VENDOR_SPINEL_PROP_HW_CAPABILITIES get property request");
        error = mEncoder.WriteUint16(nrf_802154_radio_wrapper_hw_capabilities_get());
        break;

        // TODO: Implement your get properties handlers here.
    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

    return error;
}

otError NcpBase::VendorSetPropertyHandler(spinel_prop_key_t aPropKey)
{
    otError error = OT_ERROR_NONE;

    switch (aPropKey)
    {
    case VENDOR_SPINEL_PROP_AUTO_ACK_ENABLED: {
            LOG_DBG("Got VENDOR_SPINEL_PROP_AUTO_ACK_ENABLED set property request");

            uint8_t mode = 0;
            SuccessOrExit(error = mDecoder.ReadUint8(mode));
            nrf_802154_radio_wrapper_auto_ack_set(mode);
        }
        break;

        // TODO: Implement your set properties handlers here.
    default:
        error = OT_ERROR_NOT_FOUND;
        break;
    }

exit:
    return error;
}

} // namespace Ncp
} // namespace ot

//------------------------------------------------------------------
// When OPENTHREAD_ENABLE_NCP_VENDOR_HOOK is enabled, vendor code is
// expected to provide the `otNcpInit()` function. The reason behind
// this is to enable vendor code to define its own sub-class of
// `NcpBase` or `NcpHdlc`/`NcpSpi`.
//
// Example below show how to add a vendor sub-class over `NcpHdlc`.

class NcpVendorUart : public ot::Ncp::NcpHdlc
{

    static int SendHdlc(const uint8_t *aBuf, uint16_t aBufLength)
    {
        return mSendCallback(aBuf, aBufLength);
    }

public:
    NcpVendorUart(ot::Instance *aInstance, otNcpHdlcSendCallback aSendCallback)
        : ot::Ncp::NcpHdlc(aInstance, &NcpVendorUart::SendHdlc)
    {
        OT_ASSERT(aSendCallback);
        mSendCallback = aSendCallback;
    }

    // Add public/private methods or member variables
private:
    static otNcpHdlcSendCallback mSendCallback;
};

otNcpHdlcSendCallback NcpVendorUart::mSendCallback;

static OT_DEFINE_ALIGNED_VAR(sNcpVendorRaw, sizeof(NcpVendorUart), uint64_t);


extern "C" void otNcpHdlcInit(otInstance *aInstance, otNcpHdlcSendCallback aSendCallback)
{
    NcpVendorUart *ncpVendor = NULL;
    ot::Instance *instance = static_cast<ot::Instance *>(aInstance);

    ncpVendor = new (&sNcpVendorRaw) NcpVendorUart(instance, aSendCallback);
}
#endif
