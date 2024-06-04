/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP

#include <cinttypes>
#include <cstdint>

#include <app-common/zap-generated/callback.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandHandler.h>
#include <app/InteractionModelEngine.h>
#include <app/util/util.h>
#include <lib/core/CHIPSafeCasts.h>
#include <lib/support/TypeTraits.h>

namespace chip
{
namespace app
{

	// Cluster specific command parsing

	namespace Clusters
	{

		namespace AdministratorCommissioning
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::OpenCommissioningWindow::Id: {
						Commands::OpenCommissioningWindow::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfAdministratorCommissioningClusterOpenCommissioningWindowCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::OpenBasicCommissioningWindow::Id: {
						Commands::OpenBasicCommissioningWindow::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfAdministratorCommissioningClusterOpenBasicCommissioningWindowCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::RevokeCommissioning::Id: {
						Commands::RevokeCommissioning::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfAdministratorCommissioningClusterRevokeCommissioningCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace AdministratorCommissioning

		namespace DiagnosticLogs
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::RetrieveLogsRequest::Id: {
						Commands::RetrieveLogsRequest::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfDiagnosticLogsClusterRetrieveLogsRequestCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace DiagnosticLogs

		namespace GeneralCommissioning
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::ArmFailSafe::Id: {
						Commands::ArmFailSafe::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGeneralCommissioningClusterArmFailSafeCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::SetRegulatoryConfig::Id: {
						Commands::SetRegulatoryConfig::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGeneralCommissioningClusterSetRegulatoryConfigCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::CommissioningComplete::Id: {
						Commands::CommissioningComplete::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGeneralCommissioningClusterCommissioningCompleteCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace GeneralCommissioning

		namespace GeneralDiagnostics
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::TestEventTrigger::Id: {
						Commands::TestEventTrigger::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGeneralDiagnosticsClusterTestEventTriggerCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::TimeSnapshot::Id: {
						Commands::TimeSnapshot::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGeneralDiagnosticsClusterTimeSnapshotCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace GeneralDiagnostics

		namespace GroupKeyManagement
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::KeySetWrite::Id: {
						Commands::KeySetWrite::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGroupKeyManagementClusterKeySetWriteCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::KeySetRead::Id: {
						Commands::KeySetRead::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfGroupKeyManagementClusterKeySetReadCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::KeySetRemove::Id: {
						Commands::KeySetRemove::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGroupKeyManagementClusterKeySetRemoveCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::KeySetReadAllIndices::Id: {
						Commands::KeySetReadAllIndices::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfGroupKeyManagementClusterKeySetReadAllIndicesCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace GroupKeyManagement

		namespace Identify
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::Identify::Id: {
						Commands::Identify::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfIdentifyClusterIdentifyCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace Identify

		namespace OtaSoftwareUpdateRequestor
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::AnnounceOTAProvider::Id: {
						Commands::AnnounceOTAProvider::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOtaSoftwareUpdateRequestorClusterAnnounceOTAProviderCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace OtaSoftwareUpdateRequestor

		namespace OperationalCredentials
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::AttestationRequest::Id: {
						Commands::AttestationRequest::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterAttestationRequestCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::CertificateChainRequest::Id: {
						Commands::CertificateChainRequest::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterCertificateChainRequestCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::CSRRequest::Id: {
						Commands::CSRRequest::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterCSRRequestCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::AddNOC::Id: {
						Commands::AddNOC::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfOperationalCredentialsClusterAddNOCCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::UpdateNOC::Id: {
						Commands::UpdateNOC::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterUpdateNOCCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::UpdateFabricLabel::Id: {
						Commands::UpdateFabricLabel::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterUpdateFabricLabelCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::RemoveFabric::Id: {
						Commands::RemoveFabric::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterRemoveFabricCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::AddTrustedRootCertificate::Id: {
						Commands::AddTrustedRootCertificate::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfOperationalCredentialsClusterAddTrustedRootCertificateCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace OperationalCredentials

		namespace ThreadNetworkDiagnostics
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::ResetCounts::Id: {
						Commands::ResetCounts::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfThreadNetworkDiagnosticsClusterResetCountsCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace ThreadNetworkDiagnostics

		namespace WindowCovering
		{

			void DispatchServerCommand(CommandHandler *apCommandObj,
						   const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aDataTlv)
			{
				CHIP_ERROR TLVError = CHIP_NO_ERROR;
				bool wasHandled = false;
				{
					switch (aCommandPath.mCommandId) {
					case Commands::UpOrOpen::Id: {
						Commands::UpOrOpen::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfWindowCoveringClusterUpOrOpenCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::DownOrClose::Id: {
						Commands::DownOrClose::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfWindowCoveringClusterDownOrCloseCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::StopMotion::Id: {
						Commands::StopMotion::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled = emberAfWindowCoveringClusterStopMotionCallback(
								apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::GoToLiftPercentage::Id: {
						Commands::GoToLiftPercentage::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfWindowCoveringClusterGoToLiftPercentageCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					case Commands::GoToTiltPercentage::Id: {
						Commands::GoToTiltPercentage::DecodableType commandData;
						TLVError = DataModel::Decode(aDataTlv, commandData);
						if (TLVError == CHIP_NO_ERROR) {
							wasHandled =
								emberAfWindowCoveringClusterGoToTiltPercentageCallback(
									apCommandObj, aCommandPath, commandData);
						}
						break;
					}
					default: {
						// Unrecognized command ID, error status will apply.
						apCommandObj->AddStatus(
							aCommandPath,
							Protocols::InteractionModel::Status::UnsupportedCommand);
						ChipLogError(Zcl,
							     "Unknown command " ChipLogFormatMEI
							     " for cluster " ChipLogFormatMEI,
							     ChipLogValueMEI(aCommandPath.mCommandId),
							     ChipLogValueMEI(aCommandPath.mClusterId));
						return;
					}
					}
				}

				if (CHIP_NO_ERROR != TLVError || !wasHandled) {
					apCommandObj->AddStatus(aCommandPath,
								Protocols::InteractionModel::Status::InvalidCommand);
					ChipLogProgress(Zcl, "Failed to dispatch command, TLVError=%" CHIP_ERROR_FORMAT,
							TLVError.Format());
				}
			}

		} // namespace WindowCovering

	} // namespace Clusters

	void DispatchSingleClusterCommand(const ConcreteCommandPath &aCommandPath, TLV::TLVReader &aReader,
					  CommandHandler *apCommandObj)
	{
		switch (aCommandPath.mClusterId) {
		case Clusters::AdministratorCommissioning::Id:
			Clusters::AdministratorCommissioning::DispatchServerCommand(apCommandObj, aCommandPath,
										    aReader);
			break;
		case Clusters::DiagnosticLogs::Id:
			Clusters::DiagnosticLogs::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::GeneralCommissioning::Id:
			Clusters::GeneralCommissioning::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::GeneralDiagnostics::Id:
			Clusters::GeneralDiagnostics::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::GroupKeyManagement::Id:
			Clusters::GroupKeyManagement::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::Identify::Id:
			Clusters::Identify::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::OtaSoftwareUpdateRequestor::Id:
			Clusters::OtaSoftwareUpdateRequestor::DispatchServerCommand(apCommandObj, aCommandPath,
										    aReader);
			break;
		case Clusters::OperationalCredentials::Id:
			Clusters::OperationalCredentials::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::ThreadNetworkDiagnostics::Id:
			Clusters::ThreadNetworkDiagnostics::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		case Clusters::WindowCovering::Id:
			Clusters::WindowCovering::DispatchServerCommand(apCommandObj, aCommandPath, aReader);
			break;
		default:
			ChipLogError(Zcl, "Unknown cluster " ChipLogFormatMEI,
				     ChipLogValueMEI(aCommandPath.mClusterId));
			apCommandObj->AddStatus(aCommandPath, Protocols::InteractionModel::Status::UnsupportedCluster);
			break;
		}
	}

} // namespace app
} // namespace chip
