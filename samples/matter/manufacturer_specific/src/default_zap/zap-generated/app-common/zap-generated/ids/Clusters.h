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

#pragma once

#include <app/util/basic-types.h>

namespace chip
{
namespace app
{
	namespace Clusters
	{

		namespace Identify
		{
			static constexpr ClusterId Id = 0x00000003;
		} // namespace Identify
		namespace Groups
		{
			static constexpr ClusterId Id = 0x00000004;
		} // namespace Groups
		namespace OnOff
		{
			static constexpr ClusterId Id = 0x00000006;
		} // namespace OnOff
		namespace LevelControl
		{
			static constexpr ClusterId Id = 0x00000008;
		} // namespace LevelControl
		namespace PulseWidthModulation
		{
			static constexpr ClusterId Id = 0x0000001C;
		} // namespace PulseWidthModulation
		namespace Descriptor
		{
			static constexpr ClusterId Id = 0x0000001D;
		} // namespace Descriptor
		namespace Binding
		{
			static constexpr ClusterId Id = 0x0000001E;
		} // namespace Binding
		namespace AccessControl
		{
			static constexpr ClusterId Id = 0x0000001F;
		} // namespace AccessControl
		namespace Actions
		{
			static constexpr ClusterId Id = 0x00000025;
		} // namespace Actions
		namespace BasicInformation
		{
			static constexpr ClusterId Id = 0x00000028;
		} // namespace BasicInformation
		namespace OtaSoftwareUpdateProvider
		{
			static constexpr ClusterId Id = 0x00000029;
		} // namespace OtaSoftwareUpdateProvider
		namespace OtaSoftwareUpdateRequestor
		{
			static constexpr ClusterId Id = 0x0000002A;
		} // namespace OtaSoftwareUpdateRequestor
		namespace LocalizationConfiguration
		{
			static constexpr ClusterId Id = 0x0000002B;
		} // namespace LocalizationConfiguration
		namespace TimeFormatLocalization
		{
			static constexpr ClusterId Id = 0x0000002C;
		} // namespace TimeFormatLocalization
		namespace UnitLocalization
		{
			static constexpr ClusterId Id = 0x0000002D;
		} // namespace UnitLocalization
		namespace PowerSourceConfiguration
		{
			static constexpr ClusterId Id = 0x0000002E;
		} // namespace PowerSourceConfiguration
		namespace PowerSource
		{
			static constexpr ClusterId Id = 0x0000002F;
		} // namespace PowerSource
		namespace GeneralCommissioning
		{
			static constexpr ClusterId Id = 0x00000030;
		} // namespace GeneralCommissioning
		namespace NetworkCommissioning
		{
			static constexpr ClusterId Id = 0x00000031;
		} // namespace NetworkCommissioning
		namespace DiagnosticLogs
		{
			static constexpr ClusterId Id = 0x00000032;
		} // namespace DiagnosticLogs
		namespace GeneralDiagnostics
		{
			static constexpr ClusterId Id = 0x00000033;
		} // namespace GeneralDiagnostics
		namespace SoftwareDiagnostics
		{
			static constexpr ClusterId Id = 0x00000034;
		} // namespace SoftwareDiagnostics
		namespace ThreadNetworkDiagnostics
		{
			static constexpr ClusterId Id = 0x00000035;
		} // namespace ThreadNetworkDiagnostics
		namespace WiFiNetworkDiagnostics
		{
			static constexpr ClusterId Id = 0x00000036;
		} // namespace WiFiNetworkDiagnostics
		namespace EthernetNetworkDiagnostics
		{
			static constexpr ClusterId Id = 0x00000037;
		} // namespace EthernetNetworkDiagnostics
		namespace TimeSynchronization
		{
			static constexpr ClusterId Id = 0x00000038;
		} // namespace TimeSynchronization
		namespace BridgedDeviceBasicInformation
		{
			static constexpr ClusterId Id = 0x00000039;
		} // namespace BridgedDeviceBasicInformation
		namespace Switch
		{
			static constexpr ClusterId Id = 0x0000003B;
		} // namespace Switch
		namespace AdministratorCommissioning
		{
			static constexpr ClusterId Id = 0x0000003C;
		} // namespace AdministratorCommissioning
		namespace OperationalCredentials
		{
			static constexpr ClusterId Id = 0x0000003E;
		} // namespace OperationalCredentials
		namespace GroupKeyManagement
		{
			static constexpr ClusterId Id = 0x0000003F;
		} // namespace GroupKeyManagement
		namespace FixedLabel
		{
			static constexpr ClusterId Id = 0x00000040;
		} // namespace FixedLabel
		namespace UserLabel
		{
			static constexpr ClusterId Id = 0x00000041;
		} // namespace UserLabel
		namespace ProxyConfiguration
		{
			static constexpr ClusterId Id = 0x00000042;
		} // namespace ProxyConfiguration
		namespace ProxyDiscovery
		{
			static constexpr ClusterId Id = 0x00000043;
		} // namespace ProxyDiscovery
		namespace ProxyValid
		{
			static constexpr ClusterId Id = 0x00000044;
		} // namespace ProxyValid
		namespace BooleanState
		{
			static constexpr ClusterId Id = 0x00000045;
		} // namespace BooleanState
		namespace IcdManagement
		{
			static constexpr ClusterId Id = 0x00000046;
		} // namespace IcdManagement
		namespace Timer
		{
			static constexpr ClusterId Id = 0x00000047;
		} // namespace Timer
		namespace OvenCavityOperationalState
		{
			static constexpr ClusterId Id = 0x00000048;
		} // namespace OvenCavityOperationalState
		namespace OvenMode
		{
			static constexpr ClusterId Id = 0x00000049;
		} // namespace OvenMode
		namespace LaundryDryerControls
		{
			static constexpr ClusterId Id = 0x0000004A;
		} // namespace LaundryDryerControls
		namespace ModeSelect
		{
			static constexpr ClusterId Id = 0x00000050;
		} // namespace ModeSelect
		namespace LaundryWasherMode
		{
			static constexpr ClusterId Id = 0x00000051;
		} // namespace LaundryWasherMode
		namespace RefrigeratorAndTemperatureControlledCabinetMode
		{
			static constexpr ClusterId Id = 0x00000052;
		} // namespace RefrigeratorAndTemperatureControlledCabinetMode
		namespace LaundryWasherControls
		{
			static constexpr ClusterId Id = 0x00000053;
		} // namespace LaundryWasherControls
		namespace RvcRunMode
		{
			static constexpr ClusterId Id = 0x00000054;
		} // namespace RvcRunMode
		namespace RvcCleanMode
		{
			static constexpr ClusterId Id = 0x00000055;
		} // namespace RvcCleanMode
		namespace TemperatureControl
		{
			static constexpr ClusterId Id = 0x00000056;
		} // namespace TemperatureControl
		namespace RefrigeratorAlarm
		{
			static constexpr ClusterId Id = 0x00000057;
		} // namespace RefrigeratorAlarm
		namespace DishwasherMode
		{
			static constexpr ClusterId Id = 0x00000059;
		} // namespace DishwasherMode
		namespace AirQuality
		{
			static constexpr ClusterId Id = 0x0000005B;
		} // namespace AirQuality
		namespace SmokeCoAlarm
		{
			static constexpr ClusterId Id = 0x0000005C;
		} // namespace SmokeCoAlarm
		namespace DishwasherAlarm
		{
			static constexpr ClusterId Id = 0x0000005D;
		} // namespace DishwasherAlarm
		namespace MicrowaveOvenMode
		{
			static constexpr ClusterId Id = 0x0000005E;
		} // namespace MicrowaveOvenMode
		namespace MicrowaveOvenControl
		{
			static constexpr ClusterId Id = 0x0000005F;
		} // namespace MicrowaveOvenControl
		namespace OperationalState
		{
			static constexpr ClusterId Id = 0x00000060;
		} // namespace OperationalState
		namespace RvcOperationalState
		{
			static constexpr ClusterId Id = 0x00000061;
		} // namespace RvcOperationalState
		namespace ScenesManagement
		{
			static constexpr ClusterId Id = 0x00000062;
		} // namespace ScenesManagement
		namespace HepaFilterMonitoring
		{
			static constexpr ClusterId Id = 0x00000071;
		} // namespace HepaFilterMonitoring
		namespace ActivatedCarbonFilterMonitoring
		{
			static constexpr ClusterId Id = 0x00000072;
		} // namespace ActivatedCarbonFilterMonitoring
		namespace BooleanStateConfiguration
		{
			static constexpr ClusterId Id = 0x00000080;
		} // namespace BooleanStateConfiguration
		namespace ValveConfigurationAndControl
		{
			static constexpr ClusterId Id = 0x00000081;
		} // namespace ValveConfigurationAndControl
		namespace ElectricalPowerMeasurement
		{
			static constexpr ClusterId Id = 0x00000090;
		} // namespace ElectricalPowerMeasurement
		namespace ElectricalEnergyMeasurement
		{
			static constexpr ClusterId Id = 0x00000091;
		} // namespace ElectricalEnergyMeasurement
		namespace WaterHeaterManagement
		{
			static constexpr ClusterId Id = 0x00000094;
		} // namespace WaterHeaterManagement
		namespace DemandResponseLoadControl
		{
			static constexpr ClusterId Id = 0x00000096;
		} // namespace DemandResponseLoadControl
		namespace Messages
		{
			static constexpr ClusterId Id = 0x00000097;
		} // namespace Messages
		namespace DeviceEnergyManagement
		{
			static constexpr ClusterId Id = 0x00000098;
		} // namespace DeviceEnergyManagement
		namespace EnergyEvse
		{
			static constexpr ClusterId Id = 0x00000099;
		} // namespace EnergyEvse
		namespace EnergyPreference
		{
			static constexpr ClusterId Id = 0x0000009B;
		} // namespace EnergyPreference
		namespace PowerTopology
		{
			static constexpr ClusterId Id = 0x0000009C;
		} // namespace PowerTopology
		namespace EnergyEvseMode
		{
			static constexpr ClusterId Id = 0x0000009D;
		} // namespace EnergyEvseMode
		namespace WaterHeaterMode
		{
			static constexpr ClusterId Id = 0x0000009E;
		} // namespace WaterHeaterMode
		namespace DeviceEnergyManagementMode
		{
			static constexpr ClusterId Id = 0x0000009F;
		} // namespace DeviceEnergyManagementMode
		namespace DoorLock
		{
			static constexpr ClusterId Id = 0x00000101;
		} // namespace DoorLock
		namespace WindowCovering
		{
			static constexpr ClusterId Id = 0x00000102;
		} // namespace WindowCovering
		namespace ServiceArea
		{
			static constexpr ClusterId Id = 0x00000150;
		} // namespace ServiceArea
		namespace PumpConfigurationAndControl
		{
			static constexpr ClusterId Id = 0x00000200;
		} // namespace PumpConfigurationAndControl
		namespace Thermostat
		{
			static constexpr ClusterId Id = 0x00000201;
		} // namespace Thermostat
		namespace FanControl
		{
			static constexpr ClusterId Id = 0x00000202;
		} // namespace FanControl
		namespace ThermostatUserInterfaceConfiguration
		{
			static constexpr ClusterId Id = 0x00000204;
		} // namespace ThermostatUserInterfaceConfiguration
		namespace ColorControl
		{
			static constexpr ClusterId Id = 0x00000300;
		} // namespace ColorControl
		namespace BallastConfiguration
		{
			static constexpr ClusterId Id = 0x00000301;
		} // namespace BallastConfiguration
		namespace IlluminanceMeasurement
		{
			static constexpr ClusterId Id = 0x00000400;
		} // namespace IlluminanceMeasurement
		namespace TemperatureMeasurement
		{
			static constexpr ClusterId Id = 0x00000402;
		} // namespace TemperatureMeasurement
		namespace PressureMeasurement
		{
			static constexpr ClusterId Id = 0x00000403;
		} // namespace PressureMeasurement
		namespace FlowMeasurement
		{
			static constexpr ClusterId Id = 0x00000404;
		} // namespace FlowMeasurement
		namespace RelativeHumidityMeasurement
		{
			static constexpr ClusterId Id = 0x00000405;
		} // namespace RelativeHumidityMeasurement
		namespace OccupancySensing
		{
			static constexpr ClusterId Id = 0x00000406;
		} // namespace OccupancySensing
		namespace CarbonMonoxideConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000040C;
		} // namespace CarbonMonoxideConcentrationMeasurement
		namespace CarbonDioxideConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000040D;
		} // namespace CarbonDioxideConcentrationMeasurement
		namespace NitrogenDioxideConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x00000413;
		} // namespace NitrogenDioxideConcentrationMeasurement
		namespace OzoneConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x00000415;
		} // namespace OzoneConcentrationMeasurement
		namespace Pm25ConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042A;
		} // namespace Pm25ConcentrationMeasurement
		namespace FormaldehydeConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042B;
		} // namespace FormaldehydeConcentrationMeasurement
		namespace Pm1ConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042C;
		} // namespace Pm1ConcentrationMeasurement
		namespace Pm10ConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042D;
		} // namespace Pm10ConcentrationMeasurement
		namespace TotalVolatileOrganicCompoundsConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042E;
		} // namespace TotalVolatileOrganicCompoundsConcentrationMeasurement
		namespace RadonConcentrationMeasurement
		{
			static constexpr ClusterId Id = 0x0000042F;
		} // namespace RadonConcentrationMeasurement
		namespace WiFiNetworkManagement
		{
			static constexpr ClusterId Id = 0x00000451;
		} // namespace WiFiNetworkManagement
		namespace ThreadBorderRouterManagement
		{
			static constexpr ClusterId Id = 0x00000452;
		} // namespace ThreadBorderRouterManagement
		namespace ThreadNetworkDirectory
		{
			static constexpr ClusterId Id = 0x00000453;
		} // namespace ThreadNetworkDirectory
		namespace WakeOnLan
		{
			static constexpr ClusterId Id = 0x00000503;
		} // namespace WakeOnLan
		namespace Channel
		{
			static constexpr ClusterId Id = 0x00000504;
		} // namespace Channel
		namespace TargetNavigator
		{
			static constexpr ClusterId Id = 0x00000505;
		} // namespace TargetNavigator
		namespace MediaPlayback
		{
			static constexpr ClusterId Id = 0x00000506;
		} // namespace MediaPlayback
		namespace MediaInput
		{
			static constexpr ClusterId Id = 0x00000507;
		} // namespace MediaInput
		namespace LowPower
		{
			static constexpr ClusterId Id = 0x00000508;
		} // namespace LowPower
		namespace KeypadInput
		{
			static constexpr ClusterId Id = 0x00000509;
		} // namespace KeypadInput
		namespace ContentLauncher
		{
			static constexpr ClusterId Id = 0x0000050A;
		} // namespace ContentLauncher
		namespace AudioOutput
		{
			static constexpr ClusterId Id = 0x0000050B;
		} // namespace AudioOutput
		namespace ApplicationLauncher
		{
			static constexpr ClusterId Id = 0x0000050C;
		} // namespace ApplicationLauncher
		namespace ApplicationBasic
		{
			static constexpr ClusterId Id = 0x0000050D;
		} // namespace ApplicationBasic
		namespace AccountLogin
		{
			static constexpr ClusterId Id = 0x0000050E;
		} // namespace AccountLogin
		namespace ContentControl
		{
			static constexpr ClusterId Id = 0x0000050F;
		} // namespace ContentControl
		namespace ContentAppObserver
		{
			static constexpr ClusterId Id = 0x00000510;
		} // namespace ContentAppObserver
		namespace WebRTCTransportProvider
		{
			static constexpr ClusterId Id = 0x00000553;
		} // namespace WebRTCTransportProvider
		namespace Chime
		{
			static constexpr ClusterId Id = 0x00000556;
		} // namespace Chime
		namespace EcosystemInformation
		{
			static constexpr ClusterId Id = 0x00000750;
		} // namespace EcosystemInformation
		namespace CommissionerControl
		{
			static constexpr ClusterId Id = 0x00000751;
		} // namespace CommissionerControl
		namespace NordicDevKit
		{
			static constexpr ClusterId Id = 0xFFF1FC01;
		} // namespace NordicDevKit
		namespace UnitTesting
		{
			static constexpr ClusterId Id = 0xFFF1FC05;
		} // namespace UnitTesting
		namespace FaultInjection
		{
			static constexpr ClusterId Id = 0xFFF1FC06;
		} // namespace FaultInjection
		namespace SampleMei
		{
			static constexpr ClusterId Id = 0xFFF1FC20;
		} // namespace SampleMei

	} // namespace Clusters
} // namespace app
} // namespace chip
