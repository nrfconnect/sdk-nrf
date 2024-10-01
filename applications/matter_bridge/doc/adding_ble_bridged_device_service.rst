.. _matter_bridge_app_extending_ble_service:

Adding support for a new Bluetooth LE service
#############################################

The Matter Bridge application supports bridging Bluetooth LE devices using :ref:`LED Button Service <lbs_readme>` and :zephyr:code-sample:`ble_peripheral_esp` Bluetooth services.
You can also add support for a proprietary Bluetooth LE service, if required by your use case.
The functionality of the added Bluetooth LE service has to be represented by one or more device types available in the :ref:`Matter Data Model <ug_matter_device_types>`.
For example, the :ref:`LED Button Service <lbs_readme>` is represented by the Matter On/Off Light and Matter Generic Switch device types.

You can bridge the new Bluetooth LE service with one or more Matter device types supported by the Matter Bridge application, or add support for a new Matter device type.
To learn how to add support for new Matter device type in the Matter Bridge application, see the :ref:`matter_bridge_app_extending_matter_device` page.

You will need to implement the ``Bridged Device Data Provider`` role based on the :ref:`Matter Bridge architecture <ug_matter_overview_bridge_ncs_implementation>` for the newly added Bluetooth LE service.
The following steps show how to add support for a new Bluetooth LE service called ``My Bt Service``.

1. Include the header file containing the Bluetooth LE service declaration of the :c:struct:`bt_uuid` type in the :file:`app_task.cpp` file.

   .. code-block:: C++

      #include "my_bt_service.h"

#. Update the :c:var:`sUuidServices` array in the :file:`app_task.cpp` file to include the Bluetooth LE service UUID.

   .. code-block:: C++

      static bt_uuid *sUuidMyBtService = BT_UUID_MY_BT_SERVICE;
      static bt_uuid *sUuidServices[] = { sUuidLbs, sUuidEs, sUuidMyBtService};

#. Implement the ``Bridged Device Data Provider`` role.

   a. Create the :file:`my_bt_service_data_provider.cpp` and :file:`my_bt_service_data_provider.h` files for your Bluetooth LE Data Provider in the :file:`src/ble_providers` directory.
   #. Open the :file:`nrf/samples/matter/common/src/bridge/ble_bridged_device.h` header file and find the :c:struct:`BLEBridgedDeviceProvider` class constructor.
      Note the constructor signature, it will be used in the child class implemented in the next steps.
   #. Add a new :c:struct:`MyBtServiceDataProvider` class inheriting :c:struct:`BLEBridgedDeviceProvider`, and implement its constructor in the :file:`my_bt_service_data_provider.h` file.

      .. code-block:: C++

        #include "ble_bridged_device.h"
        #include "ble_connectivity_manager.h"
        #include "bridged_device_data_provider.h"

        #include "my_bt_service.h"

        class MyBtServiceDataProvider : public Nrf::BLEBridgedDeviceProvider {
        public:
            explicit MyBtServiceDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : Nrf::BLEBridgedDeviceProvider(updateCallback, commandCallback) {}

        };

   #. Open the :file:`nrf/samples/matter/common/src/bridge/ble_bridged_device.h` header file again to see which methods of :c:struct:`BLEBridgedDeviceProvider` class are purely virtual (assigned with ``=0``) and have to be overridden by the :c:struct:`MyBtServiceDataProvider` class.

      Note that :c:struct:`BLEBridgedDeviceProvider` inherits from the :c:struct:`BridgedDeviceDataProvider` class, so the :c:struct:`MyBtServiceDataProvider` class has to implement the purely virtual methods of :c:struct:`BridgedDeviceDataProvider` as well.
   #. Edit the :c:struct:`MyBtServiceDataProvider` class in the :file:`my_bt_service_data_provider.h` header file to declare the required methods as follows:

      .. code-block:: C++

        void Init() override;
        void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
                    size_t dataSize) override;
        CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;
        bt_uuid *GetServiceUuid() override;
        int ParseDiscoveredData(bt_gatt_dm *discoveredData) override;

   #. Include the necessary header files and namespaces in the :file:`my_bt_service_data_provider.cpp` file:

      .. code-block:: C++

         #include "my_bt_service_data_provider.h"

         #include <bluetooth/gatt_dm.h>
         #include <zephyr/bluetooth/conn.h>
         #include <zephyr/bluetooth/gatt.h>

         using namespace ::chip;
         using namespace ::chip::app;
         using namespace Nrf;

   #. Implement the body of the :c:func:`Init` method so that it can prepare the data provider for further operation.
      If there are no additional actions to be done before starting the provider, it can be implemented in the :file:`my_bt_service_data_provider.cpp` file as empty.

      .. code-block:: C++

        void MyBtServiceDataProvider::Init()
        {
            /* Do nothing in this case */
        }

   #. Implement the body of the :c:func:`NotifyUpdateState` method that shall be called after every data change related to the Matter devices bridged to the Bluetooth LE device using ``My Bt Service``.
      It is used to inform the ``Bridge Manager`` and Matter Data Model that an attribute value should be updated.

      To make the method invoke the appropriate callback, edit the :file:`my_bt_service_data_provider.cpp` file as follows:

      .. code-block:: C++

        void MyBtServiceDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
                            size_t dataSize)
        {
            if (mUpdateAttributeCallback) {
                mUpdateAttributeCallback(*this, clusterId, attributeId, data, dataSize);
            }
        }

   #. Implement the body of the :c:func:`UpdateState` method.
      This will be called by the ``Bridge Manager`` to inform that data in Matter Data Model was changed and request propagating this information to the Bluetooth LE end device.

      The content of this method depends on the supported Matter device types and the Bluetooth characteristics supported by the specific Bluetooth LE profile.
      If the profile supports write operations, the implementation should analyze the Matter :c:var:`clusterId` and :c:var:`attributeId` variables, and perform a Bluetooth GATT write operation to the corresponding Bluetooth characteristic.
      Otherwise, the method can be left empty.

      To handle write operations to the Bluetooth LE device, edit the :file:`my_bt_service_data_provider.h` and :file:`my_bt_service_data_provider.cpp` files using the following code snippets:

      - :file:`my_bt_service_data_provider.h`, :c:struct:`MyBtServiceDataProvider` class

         .. code-block:: C++

            static void NotifyAttributeChange(intptr_t context);
            static void GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params);
            bt_gatt_write_params mGattWriteParams{};

      - :file:`my_bt_service_data_provider.cpp`

         .. code-block:: C++

            CHIP_ERROR MyBtServiceDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) {
               /* Set all mGattWriteParams fields and copy data from the input to mGattWriteParams buffer. */
               /* ... */
               mGattWriteParams.func = MyBtServiceDataProvider::GattWriteCallback;

               int err = bt_gatt_write(mDevice.mConn, &mGattWriteParams);
               if (err) {
                  return CHIP_ERROR_INTERNAL;
               }
            }

            void MyBtServiceDataProvider::GattWriteCallback(bt_conn *conn, uint8_t err, bt_gatt_write_params *params)
            {
               if (!params) {
                  return;
               }
               MyBtServiceDataProvider *provider = static_cast<MyBtServiceDataProvider *>(
                  BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));
               if (!provider) {
                  return;
               }

               /* Save data received in GATT write response. */
               /* ... */
               DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
            }

            void MyBtServiceDataProvider::NotifyAttributeChange(intptr_t context)
            {
               MyBtServiceDataProvider *provider = reinterpret_cast<MyBtServiceDataProvider *>(context);
               /* Invoke provider->NotifyUpdateState() method to inform the `Bridge Manager` that write operation suceeded and Matter Data Model state can be updated. */
               /* ... */
            }

   #. Implement the body of the :c:func:`GetServiceUuid` method.
      This shall return the UUID of the ``My Bt Service`` Bluetooth LE service.
      To do this, edit the :file:`my_bt_service_data_provider.cpp` file as follows:

      .. code-block:: C++

         static bt_uuid *sServiceUuid = BT_UUID_MY_BT_SERVICE;
         bt_uuid *MyBtServiceDataProvider::GetServiceUuid()
         {
            return sServiceUuid;
         }

   #. Implement the body of the :c:func:`ParseDiscoveredData` method.
      This should parse the input data and save the required Bluetooth characteristic handles for further use.

      The Bluetooth LE service can support different sets of characteristics, so the method content will depend on this set.
      Additionally, the Bluetooth LE service might support subscriptions through the GATT CCC characteristic.
      In that case, the method implementation should establish a subscription session with the Bluetooth LE end device.

      For example, to handle a single characteristic that additionally supports subscriptions, edit the :file:`my_bt_service_data_provider.h` and :file:`my_bt_service_data_provider.cpp` files as follows:

      - :file:`my_bt_service_data_provider.h`, :c:struct:`MyBtServiceDataProvider` class

         .. code-block:: C++

            uint16_t mCharacteristicHandle;
            uint16_t mCccHandle;
            bt_gatt_subscribe_params mGattSubscribeParams{};

      - :file:`my_bt_service_data_provider.cpp`

         .. code-block:: C++

            static bt_uuid *sUuidChar = BT_UUID_MY_BT_SERVICE_CHARACTERISTIC;
            static bt_uuid *sUuidCcc = BT_UUID_GATT_CCC;
            uint8_t MyBtServiceDataProvider::GattNotifyCallback(bt_conn *conn, bt_gatt_subscribe_params *params, const void *data,
                              uint16_t length)
            {
               MyBtServiceDataProvider *provider = static_cast<MyBtServiceDataProvider *>(
                  BLEConnectivityManager::Instance().FindBLEProvider(*bt_conn_get_dst(conn)));
               VerifyOrExit(data, );
               VerifyOrExit(provider, );

               /* Save data received in GATT write response. */
               /* ... */
               DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));

            exit:
               return BT_GATT_ITER_CONTINUE;
            }

            int MyBtServiceDataProvider::ParseDiscoveredData(bt_gatt_dm *discoveredData)
            {
               const bt_gatt_dm_attr *gatt_chrc;
               const bt_gatt_dm_attr *gatt_desc;
               gatt_chrc = bt_gatt_dm_char_by_uuid(discoveredData, sUuidChar);
               if (!gatt_chrc) {
                  return -EINVAL;
               }

               gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidChar);
               if (!gatt_desc) {
                  return -EINVAL;
               }
               mCharacteristicHandle = gatt_desc->handle;

               gatt_desc = bt_gatt_dm_desc_by_uuid(discoveredData, gatt_chrc, sUuidCcc);
               if (!gatt_desc) {
                  return -EINVAL;
               }
               mCccHandle = gatt_desc->handle;

               VerifyOrReturn(mDevice.mConn, LOG_ERR("Invalid connection object"));

               /* Configure subscription for the button characteristic */
               mGattSubscribeParams.ccc_handle = mCccHandle;
               mGattSubscribeParams.value_handle = mCharacteristicHandle;
               mGattSubscribeParams.value = BT_GATT_CCC_NOTIFY;
               mGattSubscribeParams.notify = MyBtServiceDataProvider::GattNotifyCallback;
               mGattSubscribeParams.subscribe = nullptr;
               mGattSubscribeParams.write = nullptr;
               return bt_gatt_subscribe(mDevice.mConn, &mGattSubscribeParams);
            }

#. Add the ``MyBtServiceDataProvider`` implementation created in a previous step to the compilation process.
   To do that, edit the :file:`CMakeLists.txt` file as follows:

   .. code-block:: cmake

      target_sources(app PRIVATE
        src/ble_providers/my_bt_service_data_provider.cpp
      )

#. Provide an allocator for ``MyBtServiceDataProvider`` object creation.
   The Matter Bridge application uses a :c:struct:`BleBridgedDeviceFactory` factory module that creates paired ``Matter Bridged Device`` and ``Bridged Device Data Provider`` objects matching a specific Matter device type ID.
   To add support for creating the ``MyBtServiceDataProvider`` object, edit the :file:`src/ble_providers/ble_bridged_device_factory.h` and :file:`src/ble_providers/ble_bridged_device_factory.cpp` files as follows:

   - :file:`ble_bridged_device_factory.h`

      .. code-block:: C++

         #include "my_bt_service_data_provider.h"

   - :file:`ble_bridged_device_factory.cpp`, :c:func:`GetDataProviderFactory`

      .. code-block:: C++

		   { ServiceUuid::MyBtService, [](UpdateAttributeCallback updateClb, InvokeCommandCallback commandClb) {
			   return chip::Platform::New<MyBtServiceDataProvider>(updateClb, commandClb);
		   } },

#. Provide mapping between the ``My Bt Service`` UUID and corresponding Matter device types in the helper methods.

   a. Add the ``MyBtService`` UUID in the :c:enum:`ServiceUuid` declaration, in the :file:`src/ble_providers/ble_bridged_device_factory.h` header file.
   #. Perform proper mapping of Bluetooth UUID and Matter device types in the :c:func:`MatterDeviceTypeToBleService` and :c:func:`BleServiceToMatterDeviceType` methods, in the :file:`src/ble_providers/ble_bridged_device_factory.cpp` file.

#. Compile the target and test it following the steps from the :ref:`Matter Bridge application testing <matter_bridge_testing>` section.
