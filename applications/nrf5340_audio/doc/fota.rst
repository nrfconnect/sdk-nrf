.. _nrf53_audio_app_fota:

Configuring and testing FOTA upgrades for nRF5340 Audio applications
####################################################################

.. contents::
   :local:
   :depth: 2

The nRF5340 Audio applications all support FOTA upgrades, and the application implementation is based on the procedure described in :ref:`ug_nrf53_developing_ble_fota`.

Requirements for FOTA
*********************

If the application is running on the nRF5340 Audio DK, you need an external flash shield to upgrade both the application and network core at the same time.
See `Requirements for external flash memory DFU`_ in the nRF5340 Audio DK Hardware documentation for more information.

.. _nrf53_audio_app_configuration_configure_fota:

Configuring FOTA upgrades
*************************

The nRF5340 Audio applications can be built with a :ref:`FOTA configuration <nrf53_audio_app_building_config_files>` that includes the required features and applications to perform firmware upgrades over Bluetooth LE.

The FOTA configuration requires that an external flash be available and that the required DTS overlay files use the external flash shield specified in the `Requirements for FOTA`_ above.
With the external flash connected, it is possible to upgrade both the application core and the network core at the same time.

See :ref:`multi-image DFU <ug_nrf5340_multi_image_dfu>` for more information about the FOTA process on the nRF5340 SoC.

.. caution::
   Using the single-image upgrade strategy carries risk of the device being in a state where the application core firmware and the network core firmware are no longer compatible, which can result in a bricked device.
   For devices where FOTA is the only DFU method available, multi-image upgrades are recommended to ensure compatibility between the cores.
   Make sure to evaluate the risks for your device when selecting the FOTA method.

.. caution::
   The application is provided with a memory partition configuration in :file:`pm_static_fota.yml`, which is required to perform FOTA using an external flash.
   The partition configuration is an example that can be changed between versions, and can be incompatible with existing devices.
   For this reason, always create a partition configuration that suits your application.
   See :ref:`partition_manager` for more information on how to create a partition configuration.

Updating the SoftDevice
=======================

Both FOTA upgrade methods support updating the SoftDevice on the network core.
However, the current default build options for the SoftDevice create a binary that is too large to run on the network core together with a bootloader.
To reduce the size of the SoftDevice binary, you can disable unused features in the SoftDevice.
See :ref:`softdevice_controller` documentation for more information.

Entering the DFU mode
=====================

The |NCS| uses :ref:`SMP server and mcumgr <zephyr:device_mgmt>` as the DFU backend.
The SMP server service is separated from CIS and BIS services, and is only advertised when the application is in the DFU mode.
To enter the DFU mode, press **BTN 4** on the nRF5340 Audio DK during startup.

To identify the devices before the DFU takes place, the DFU mode advertising names mention the device type directly.
The names follow the pattern in which the device role is inserted between the device name and the ``_DFU`` suffix.
For example:

* Gateway: ``NRF5340_AUDIO_GW_DFU``
* Left Headset: ``NRF5340_AUDIO_HL_DFU``
* Right Headset: ``NRF5340_AUDIO_HR_DFU``

The first part of these names is based on :kconfig:option:`CONFIG_BT_DEVICE_NAME`.

.. note::
   When performing DFU for the nRF5340 Audio applications, there will be one or more error prints related to opening flash area ID 1.
   This is due to restrictions in the DFU system, and the error print is expected.
   The DFU process should still complete successfully.

.. _nrf53_audio_unicast_client_app_testing_steps_fota:

Testing FOTA upgrades
=====================

To test FOTA for the nRF5340 Audio application, ensure the application is in the DFU mode, and then follow the testing steps in the FOTA over Bluetooth Low Energy section of :ref:`ug_nrf53_developing_ble_fota` (you can skip the configuration steps).
