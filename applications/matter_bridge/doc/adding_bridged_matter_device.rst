.. _matter_bridge_app_extending_matter_device:

Adding support for a new Matter device type
###########################################

The Matter Bridge application supports bridging only a few Matter device types due to practical reasons.
However, you can select any of the available :ref:`Matter device types <ug_matter_device_types>` and add support to it in the application.

You will need to implement the ``Matter Bridged Device`` and ``Bridged Device Data Provider`` roles based on the :ref:`Matter Bridge architecture <ug_matter_overview_bridge_ncs_implementation>` for the newly added Matter device type.
The Matter Bridge application supports :ref:`simulated and Bluetooth LE <matter_bridge_app_bridged_support>` bridged device configurations.
In this guide, the simulated provider example is presented, but the process is similar for the Bluetooth LE provider as well.

The following steps show how to add support for a new Matter device type, using  the Pressure Sensor device type as an example.

1. Enable the ``Pressure Measurement`` cluster for the endpoint ``2`` in the :file:`src/bridge.zap` file and re-generate the files located in the :file:`src/zap-generated` directory.

   To learn how to modify the :file:`.zap` file and re-generate the :file:`zap-generated` directory, see the :ref:`ug_matter_creating_accessory_edit_zap` section in the :ref:`ug_matter_creating_accessory` user guide.
#. Implement the ``Matter Bridged Device`` role.

   a. Create the :file:`pressure_sensor.cpp` and :file:`pressure_sensor.h` files in the :file:`src/bridged_device_types` directory.
   #. Open the :file:`nrf/samples/matter/common/bridge/matter_bridged_device.h` header file and find the :c:struct:`MatterBridgedDevice` class constructor.
      Note the constructor signature, it will be used in the child class implemented in the next steps.
   #. Add a new :c:struct:`PressureSensorDevice` class inheriting :c:struct:`MatterBridgedDevice`, and implement its constructor in the :file:`pressure_sensor.cpp` and :file:`pressure_sensor.h` files.

      - :file:`pressure_sensor.h`

         .. code-block:: C++

            #pragma once

            #include "matter_bridged_device.h"

            class PressureSensorDevice : public MatterBridgedDevice {
            public:

            PressureSensorDevice(const char *nodeLabel);
            static constexpr uint16_t kPressureSensorDeviceTypeId = 0x0305;

            };

      - :file:`pressure_sensor.cpp`

         .. code-block:: C++

            #include "pressure_sensor.h"

            PressureSensorDevice::PressureSensorDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel) {}

   #. Declare all clusters that are mandatory for the Pressure Sensor device type, according to the Matter device library specification, and fill the appropriate :c:struct:`MatterBridgedDevice` class fields in the :c:struct:`PressureSensorDevice` class constructor.

      The Pressure Sensor device requires the ``Descriptor``, ``Bridged Device Basic Information`` and ``Identify`` clusters, which can be declared using helper macros from the :file:`nrf/samples/matter/common/bridge/matter_bridged_device.h` header file, and the ``Pressure Measurement`` cluster, which has to be defined in the application.
      Edit the :file:`pressure_sensor.cpp` file as follows:

      - Add:

         .. code-block:: C++

            namespace
            {
            DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs);
            BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs);
            IDENTIFY_CLUSTER_ATTRIBUTES(identifyAttrs);
            }; /* namespace */
            using namespace ::chip;
            using namespace ::chip::app;

            DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(pressureSensorAttrs)
            DECLARE_DYNAMIC_ATTRIBUTE(Clusters::PressureMeasurement::Attributes::MeasuredValue::Id, INT16S, 2, 0),
               DECLARE_DYNAMIC_ATTRIBUTE(Clusters::PressureMeasurement::Attributes::MinMeasuredValue::Id, INT16S, 2,
                           0),
               DECLARE_DYNAMIC_ATTRIBUTE(Clusters::PressureMeasurement::Attributes::MaxMeasuredValue::Id, INT16S, 2,
                           0),
               DECLARE_DYNAMIC_ATTRIBUTE(Clusters::PressureMeasurement::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
               DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

            DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedPressureClusters)
            DECLARE_DYNAMIC_CLUSTER(Clusters::PressureMeasurement::Id, pressureSensorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
               DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
               DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
               DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, ZAP_CLUSTER_MASK(SERVER), sIdentifyIncomingCommands,
                           nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

            DECLARE_DYNAMIC_ENDPOINT(bridgedPressureEndpoint, bridgedPressureClusters);

            static constexpr uint8_t kBridgedPressureEndpointVersion = 2;

            static constexpr EmberAfDeviceType kBridgedPressureDeviceTypes[] = {
               { static_cast<chip::DeviceTypeId>(PressureSensorDevice::kPressureSensorDeviceTypeId),
               kBridgedPressureEndpointVersion },
               { static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
               MatterBridgedDevice::kDefaultDynamicEndpointVersion }
            };

            static constexpr uint8_t kPressureDataVersionSize = ArraySize(bridgedPressureClusters);

      - Modify the constructor:

         .. code-block:: C++

            PressureSensorDevice::PressureSensorDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
            {
                  mDataVersionSize = kPressureDataVersionSize;
                  mEp = &bridgedPressureEndpoint;
                  mDeviceTypeList = kBridgedPressureDeviceTypes;
                  mDeviceTypeListSize = ARRAY_SIZE(kBridgedPressureDeviceTypes);
                  mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
            }

   #. Open the :file:`nrf/samples/matter/common/bridge/matter_bridged_device.h` header file again to see which methods of the :c:struct:`MatterBridgedDevice` class are purely virtual (assigned with ``=0``) and have to be overridden by the :c:struct:`PressureSensorDevice` class.
   #. Edit the :c:struct:`PressureSensorDevice` class in the :file:`pressure_sensor.h` header file to declare the required methods as follows:

      .. code-block:: C++

        uint16_t GetDeviceType() const override;

        CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
                    uint16_t maxReadLength) override;
        CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;
        CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
                        size_t dataSize) override;

   #. Implement the body of the :c:func:`GetDeviceType` method so that it can return the device type ID for the Pressure Sensor device type, which is equal to ``0x0305``.
      To check the device type ID for specific type of device, see Matter Device Library Specification.

      Edit the :file:`pressure_sensor.cpp` file as follows:

      .. code-block:: C++

         uint16_t PressureSensorDevice::GetDeviceType() const {
            return PressureSensorDevice::kPressureSensorDeviceTypeId;
         }

   #. Implement the body of the :c:func:`HandleRead` method to handle reading data operations for all supported attributes.

      The read operations for the ``Descriptor``, ``Bridged Device Basic Information`` and ``Identify`` clusters, which are common to all devices, are handled in a common bridge module.
      The read operations for the ``Pressure Measurement`` cluster are the only ones to that need to be handled in the application.

      To provide support for reading attributes for the Pressure Sensor device, edit the :file:`pressure_sensor.h` and :file:`pressure_sensor.cpp` files as follows:

      - :file:`pressure_sensor.h`, :c:struct:`PressureSensorDevice` class

         .. code-block:: C++

            int16_t GetMeasuredValue() { return mMeasuredValue; }
            int16_t GetMinMeasuredValue() { return 95; }
            int16_t GetMaxMeasuredValue() { return 101; }
            uint16_t GetPressureMeasurementClusterRevision() { return 3; }
            uint32_t GetPressureMeasurementFeatureMap() { return 0; }

            CHIP_ERROR HandleReadPressureMeasurement(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
            uint16_t mMeasuredValue = 0;

      - :file:`pressure_sensor.cpp`

         .. code-block:: C++

            CHIP_ERROR PressureSensorDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength) {
               switch (clusterId) {
               case Clusters::PressureMeasurement::Id:
                  return HandleReadPressureMeasurement(attributeId, buffer, maxReadLength);
               default:
                  return CHIP_ERROR_INVALID_ARGUMENT;
               }
            }
            CHIP_ERROR PressureSensorDevice::HandleReadPressureMeasurement(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength) {
               switch (attributeId) {
               case Clusters::PressureMeasurement::Attributes::MeasuredValue::Id: {
                  int16_t value = GetMeasuredValue();
                  return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
               }
               case Clusters::PressureMeasurement::Attributes::MinMeasuredValue::Id: {
                  int16_t value = GetMinMeasuredValue();
                  return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
               }
               case Clusters::PressureMeasurement::Attributes::MaxMeasuredValue::Id: {
                  int16_t value = GetMaxMeasuredValue();
                  return CopyAttribute(&value, sizeof(value), buffer, maxReadLength);
               }
               case Clusters::PressureMeasurement::Attributes::ClusterRevision::Id: {
                  uint16_t clusterRevision = GetPressureMeasurementClusterRevision();
                  return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
               }
               case Clusters::PressureMeasurement::Attributes::FeatureMap::Id: {
                  uint32_t featureMap = GetPressureMeasurementFeatureMap();
                  return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
               }
               default:
                  return CHIP_ERROR_INVALID_ARGUMENT;
               }
            }

   #. Implement the body of the :c:func:`HandleWrite` method, which handles write data operations for all supported attributes.
      In this case, there is no attribute supporting write operations, so edit the :file:`pressure_sensor.cpp` file as follows:

      .. code-block:: C++

         CHIP_ERROR PressureSensorDevice::HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) {
            return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
         }

   #. Implement the body of the :c:func:`HandleAttributeChange` method.
      This will be called by the ``Bridge Manager`` to notify that data was changed by the ``Bridged Device Data Provider`` and the local state should be updated.

      Edit the :file:`pressure_sensor.h` and :file:`pressure_sensor.cpp` files as follows:

      - :file:`pressure_sensor.h`

         .. code-block:: C++

            void SetMeasuredValue(int16_t value) { mMeasuredValue = value; }

      - :file:`pressure_sensor.cpp`

         .. code-block:: C++

            CHIP_ERROR PressureSensorDevice::HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data, size_t dataSize)
            {
               CHIP_ERROR err = CHIP_NO_ERROR;
               if (!data) {
                  return CHIP_ERROR_INVALID_ARGUMENT;
               }
               switch (clusterId) {
               case Clusters::BridgedDeviceBasicInformation::Id:
                  return HandleWriteDeviceBasicInformation(clusterId, attributeId, data, dataSize);
               case Clusters::Identify::Id:
                  return HandleWriteIdentify(attributeId, data, dataSize);
               case Clusters::PressureMeasurement::Id: {
                  switch (attributeId) {
                  case Clusters::PressureMeasurement::Attributes::MeasuredValue::Id: {
                     int16_t value;

                     err = CopyAttribute(data, dataSize, &value, sizeof(value));

                     if (err != CHIP_NO_ERROR) {
                        return err;
                     }

                     SetMeasuredValue(value);

                     break;
                  }
                  default:
                     return CHIP_ERROR_INVALID_ARGUMENT;
                  }
                  break;
               }
               default:
                  return CHIP_ERROR_INVALID_ARGUMENT;
               }

               return err;
            }

#. Implement the ``Bridged Device Data Provider`` role.

   a. Create the :file:`simulated_pressure_sensor_data_provider.cpp` and :file:`simulated_pressure_sensor_data_provider.h` files in the :file:`src/simulated_providers` directory.
   #. Open the :file:`nrf/samples/matter/common/bridge/bridged_device_data_provider.h` header file and find the :c:struct:`BridgedDeviceDataProvider` class constructor.
      Note the constructor signature, it will be used in the child class implemented in the next steps.
   #. Add a new :c:struct:`SimulatedPressureSensorDataProvider` class inheriting :c:struct:`BridgedDeviceDataProvider`, and implement its constructor in the :file:`simulated_pressure_sensor_data_provider.h` header file.

      .. code-block:: C++

         #pragma once

         #include "bridged_device_data_provider.h"

         #include <zephyr/kernel.h>

         class SimulatedPressureSensorDataProvider : public BridgedDeviceDataProvider {
         public:
            SimulatedPressureSensorDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : BridgedDeviceDataProvider(updateCallback, commandCallback) {}
            ~SimulatedPressureSensorDataProvider() {}
         };

   #. Open the :file:`nrf/samples/matter/common/bridge/bridged_device_data_provider.h` header file again to see which methods of the :c:struct:`BridgedDeviceDataProvider` class are purely virtual (assigned with ``=0``) and have to be overridden by the :c:struct:`SimulatedPressureSensorDataProvider` class.
   #. Edit the :c:struct:`SimulatedPressureSensorDataProvider` class in the :file:`simulated_pressure_sensor_data_provider.h` header file to declare the required methods as follows:

      .. code-block:: C++

         void Init() override;
         void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data, size_t dataSize) override;
         CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

   #. Implement the body of the :c:func:`Init` method so that it can prepare the data provider for further operation.
      In this case, the pressure measurements will be simulated by changing data in a random manner and updating it at fixed time intervals.

      To initialize the timer and perform measurement updates, edit the :file:`simulated_pressure_sensor_data_provider.h` and :file:`simulated_pressure_sensor_data_provider.cpp` files as follows:

      - :file:`simulated_pressure_sensor_data_provider.h`, :c:struct:`SimulatedPressureSensorDataProvider` class

         .. code-block:: C++

            static void NotifyAttributeChange(intptr_t context);

            static constexpr uint16_t kMeasurementsIntervalMs = 10000;
            static constexpr int16_t kMinRandomPressure = 95;
            static constexpr int16_t kMaxRandomPressure = 101;

            static void TimerTimeoutCallback(k_timer *timer);
            k_timer mTimer;
            int16_t mPressure = 0;

      - :file:`simulated_pressure_sensor_data_provider.cpp`

         .. code-block:: C++

            #include "simulated_pressure_sensor_data_provider.h"

            using namespace ::chip;
            using namespace ::chip::app;

            void SimulatedPressureSensorDataProvider::Init()
            {
               k_timer_init(&mTimer, SimulatedPressureSensorDataProvider::TimerTimeoutCallback, nullptr);
               k_timer_user_data_set(&mTimer, this);
               k_timer_start(&mTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
            }

            void SimulatedPressureSensorDataProvider::TimerTimeoutCallback(k_timer *timer)
            {
               if (!timer || !timer->user_data) {
                  return;
               }
               SimulatedPressureSensorDataProvider *provider = reinterpret_cast<SimulatedPressureSensorDataProvider *>(timer->user_data);
               /* Get some random data to emulate sensor measurements. */
               provider->mPressure = chip::Crypto::GetRandU16() % (kMaxRandomPressure - kMinRandomPressure) + kMinRandomPressure;
               DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
            }

            void SimulatedPressureSensorDataProvider::NotifyAttributeChange(intptr_t context)
            {
               SimulatedPressureSensorDataProvider *provider = reinterpret_cast<SimulatedPressureSensorDataProvider *>(context);
               provider->NotifyUpdateState(Clusters::PressureMeasurement::Id,
                           Clusters::PressureMeasurement::Attributes::MeasuredValue::Id,
                           &provider->mPressure, sizeof(provider->mPressure));
            }

   #. Implement the body of the :c:func:`NotifyUpdateState` method that shall be called after every data change related to the Pressure Sensor device.
      It is used to inform the ``Bridge Manager`` and Matter Data Model that an attribute value should be updated.

      To make the method invoke the appropriate callback, edit the :file:`simulated_pressure_sensor_data_provider.cpp` file as follows:

      .. code-block:: C++

         void SimulatedPressureSensorDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
                                    void *data, size_t dataSize)
         {
            if (mUpdateAttributeCallback) {
               mUpdateAttributeCallback(*this, Clusters::PressureMeasurement::Id,
                        Clusters::PressureMeasurement::Attributes::MeasuredValue::Id, data,
                        dataSize);
            }
         }

   #. Implement the body of the :c:func:`UpdateState` method.
      This will be called by the ``Bridge Manager`` to inform that data in Matter Data Model was changed and request propagating this information to the end device.

      In this case, there is no attribute supporting write operations and sending data to end device is not required, so edit the :file:`simulated_pressure_sensor_data_provider.cpp` file as follows:

      .. code-block:: C++

         CHIP_ERROR SimulatedPressureSensorDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
                                    uint8_t *buffer)
         {
            return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
         }

#. Add the ``PressureSensorDevice`` and ``SimulatedPressureSensorDataProvider`` implementations created in previous steps to the compilation process.
   To do that, edit the :file:`CMakeLists.txt` file as follows:

   .. code-block:: cmake

      target_sources(app PRIVATE
        src/bridged_device_types/pressure_sensor.cpp
        src/simulated_providers/simulated_pressure_sensor_data_provider.cpp
      )

#. Provide allocators for ``PressureSensorDevice`` and ``SimulatedPressureSensorDataProvider``  object creation.
   The Matter Bridge application uses a :c:struct:`SimulatedBridgedDeviceFactory` factory module that creates paired ``Matter Bridged Device`` and ``Bridged Device Data Provider`` objects matching a specific Matter device type ID.

   To add support for creating the ``PressureSensorDevice`` and ``SimulatedPressureSensorDataProvider`` objects when the Pressure Sensor device type ID is used, edit the :file:`src/simulated_providers/simulated_bridged_device_factory.h` and :file:`src/simulated_providers/simulated_bridged_device_factory.cpp` files as follows:

   - :file:`src/simulated_providers/simulated_bridged_device_factory.h`

      .. code-block:: C++

         #include "pressure_sensor.h"
         #include "simulated_pressure_sensor_data_provider.h"

   - :file:`src/simulated_providers/simulated_bridged_device_factory.h`, :c:func:`GetBridgedDeviceFactory` method

      .. code-block:: C++

         { PressureSensorDevice::kPressureSensorDeviceTypeId,
         [checkLabel](const char *nodeLabel) -> MatterBridgedDevice * {
            if (!checkLabel(nodeLabel)) {
               return nullptr;
            }
            return chip::Platform::New<PressureSensorDevice>(nodeLabel);
         } },

   - :file:`src/simulated_providers/simulated_bridged_device_factory.h`, :c:func:`GetDataProviderFactory` method

      .. code-block:: C++

         { PressureSensorDevice::kPressureSensorDeviceTypeId,
         [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
            return chip::Platform::New<SimulatedPressureSensorDataProvider>(updateClb, commandClb);
         } },

5. Compile the target and test it following the steps from the :ref:`Matter Bridge application testing <matter_bridge_testing>` section.
