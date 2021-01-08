/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include "LightBulbPublishService.h"

#include "AppTask.h"

#include <logging/log.h>
#include <zephyr.h>

#include <openthread/message.h>
#include <openthread/udp.h>
#include <platform/OpenThread/OpenThreadUtils.h>

#include <inet/InetConfig.h>
#include <platform/CHIPDeviceLayer.h>
#include <transport/TransportMgr.h>
#include <transport/raw/UDP.h>

LOG_MODULE_DECLARE(app);

// Pointer to the LightBulbPublishService instance needed by static
// PublishTimerHandler method
static LightBulbPublishService* sLightBulbPublishService;

const char PUBLISH_SERVICE_PAYLOAD[] = "chip-light-bulb";
const char PUBLISH_DEST_ADDR[] = "ff03::1:12345";

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::Transport;
using namespace ::chip::DeviceLayer;

void LightBulbPublishService::Init()
{
    // Set pointer to the current instance
    sLightBulbPublishService = this;
    mIsServiceStarted = false;

    // Initialize timer
    k_timer_init(&mPublishTimer, &PublishTimerHandler, nullptr);
    k_timer_user_data_set(&mPublishTimer, this);

    // Initialize UDP endpoint
    UdpListenParameters udpParams(&chip::DeviceLayer::InetLayer);
    udpParams.SetAddressType(kIPAddressType_IPv6);
    udpParams.SetInterfaceId(INET_NULL_INTERFACEID);
    udpParams.SetListenPort(SERVICE_UDP_PORT);
    if (mUdp.Init(udpParams))
    {
        LOG_ERR("LightBulbPublishService initialization failed");
    }

    mMessageId = 0;
    IPAddress::FromString(PUBLISH_DEST_ADDR, mPublishDestAddr);
}

void LightBulbPublishService::Start(uint32_t aTimeoutInMs, uint32_t aIntervalInMs)
{
    if (!mIsServiceStarted)
    {
        if (aTimeoutInMs >= aIntervalInMs)
        {
            LOG_INF("Started Publish service");

            mPublishInterval = aIntervalInMs;
            // Calculate number of publish iterations to do considering given timeout and interval values.
            mPublishIterations = aTimeoutInMs / aIntervalInMs;
            mIsServiceStarted = true;

            k_timer_start(&mPublishTimer, K_MSEC(aIntervalInMs), K_NO_WAIT);
        }
        else
        {
            LOG_ERR("Publish service cannot be started: service timeout is greater than interval");
        }
    }
    else
    {
        LOG_INF("Publish service is already running");
    }
}

void LightBulbPublishService::Stop()
{
    if (mIsServiceStarted)
    {
        LOG_INF("Stopped Publish service");

        k_timer_stop(&mPublishTimer);

        mIsServiceStarted = false;
        mMessageId = 0;
    }
    else
    {
        LOG_INF("Publish service is not running");
    }
}

void LightBulbPublishService::Publish()
{
    if (!chip::DeviceLayer::ConnectivityMgrImpl().IsThreadAttached())
    {
        return;
    }

    const uint16_t payloadLength = sizeof(PUBLISH_SERVICE_PAYLOAD);

    chip::System::PacketBufferHandle buffer = chip::System::PacketBuffer::NewWithAvailableSize(payloadLength);
    if (!buffer.IsNull())
    {
        memcpy(buffer->Start(), PUBLISH_SERVICE_PAYLOAD, payloadLength);
        buffer->SetDataLength(payloadLength);

        // Subsequent messages are assumed to have increasing IDs.
        PacketHeader header;
        header.SetMessageId(mMessageId++);

        if (mUdp.SendMessage(header, Transport::PeerAddress::UDP(mPublishDestAddr), std::move(buffer)))
        {
            LOG_INF("LightBulbPublishService message send successfully");
        }
        else
        {
            LOG_INF("LightBulbPublishService message sending failed");
        }
    }
}

void LightBulbPublishService::PublishTimerHandler(k_timer *aTimer)
{
    if (sLightBulbPublishService->mPublishIterations > 0)
    {
        GetAppTask().PostEvent(AppEvent{ AppEvent::PublishLightBulbService });
        k_timer_start(&(sLightBulbPublishService->mPublishTimer), K_MSEC(sLightBulbPublishService->mPublishInterval), K_NO_WAIT);
        sLightBulbPublishService->mPublishIterations--;
    }
    else
    {
        sLightBulbPublishService->Stop();
    }
}
