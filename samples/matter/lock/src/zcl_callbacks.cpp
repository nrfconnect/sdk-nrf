/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "bolt_lock_manager.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <lib/support/CodeUtils.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::DoorLock;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	VerifyOrReturn(attributePath.mClusterId == DoorLock::Id &&
		       attributePath.mAttributeId == DoorLock::Attributes::LockState::Id);

	switch (*value) {
	case to_underlying(DlLockState::kLocked):
		GetAppTask().PostEvent(AppEvent(AppEvent::Lock, BoltLockManager::OperationSource::kRemote));
		break;
	case to_underlying(DlLockState::kUnlocked):
		GetAppTask().PostEvent(AppEvent(AppEvent::Unlock, BoltLockManager::OperationSource::kRemote));
		break;
	default:
		break;
	}
}

bool emberAfPluginDoorLockOnDoorLockCommand(chip::EndpointId endpointId, const Optional<ByteSpan> &pinCode,
					    DlOperationError &err)
{
	return true;
}

bool emberAfPluginDoorLockOnDoorUnlockCommand(chip::EndpointId endpointId, const Optional<ByteSpan> &pinCode,
					      DlOperationError &err)
{
	return true;
}

void emberAfDoorLockClusterInitCallback(EndpointId endpoint)
{
	DoorLockServer::Instance().InitServer(endpoint);
	GetAppTask().UpdateClusterState(BoltLockMgr().GetState(), BoltLockManager::OperationSource::kUnspecified);
}
