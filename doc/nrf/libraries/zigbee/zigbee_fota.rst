.. _lib_zigbee_fota:

Zigbee FOTA
###########

.. contents::
   :local:
   :depth: 2

The Zigbee firmware over-the-air (Zigbee FOTA) library provides Zigbee endpoint definition, which implements clusters responsible for transferring a firmware file through the Zigbee network.
The received data is passed as an upgrade candidate through the :ref:`lib_dfu_multi_image` library API to the :ref:`lib_dfu_target`.

The library uses the endpoint to:

* Discover Zigbee OTA server by sending Match Descriptor Request broadcast messages.
* Query the discovered server periodically to check if there is a suitable image for the update.
* Transfer the new firmware using the ZCL OTA cluster commands.
* Allow the OTA server to manage the upgrade process using the ZCL OTA control commands.

When used, the Zigbee FOTA library replaces the :ref:`lib_fota_download` library in |NCS|.

.. note::
    The Zigbee FOTA functionality is available for the :ref:`zigbee_light_switch_sample` sample.

OTA upgrade entities
********************

The following entities participate in the Zigbee OTA Upgrade process:

* OTA Upgrade Client – Runs on the target, that is the device that is being upgraded, and is responsible for downloading and installing the new firmware.
* OTA Upgrade Server – Provides the firmware image.
  The OTA Upgrade Server can be either a standalone third-party device or it can be instantiated on an nRF52840 DK using nRF Util's `DFU over Zigbee`_ procedure.

OTA upgrade process
*******************

The OTA Upgrade Client queries for OTA Upgrade Servers with the intervals defined by the :kconfig:option:`CONFIG_ZIGBEE_FOTA_SERVER_DISOVERY_INTERVAL_HRS` Kconfig option until at least one server is found.
Once found, the client starts to query the server for images.
The interval between queries for the available Zigbee FOTA images is defined by the :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_QUERY_INTERVAL_MIN` Kconfig option.
After querying the OTA Upgrade Server for available images and receiving information about the image, the library begins the OTA upgrade process.
The library uses :ref:`nrfxlib:zboss`'s ZCL API to download the image.

After the OTA Upgrade Client downloads the Zigbee OTA image header, the stack verifies the following mandatory fields:

* Manufacturer ID - Defined by the :kconfig:option:`CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID` Kconfig option.
* Image type - Defined by the :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_TYPE` Kconfig option; it may be different than the MCUboot image type value.
* Hardware version - Defined by the :kconfig:option:`CONFIG_ZIGBEE_FOTA_HW_VERSION` Kconfig option.
* Firmware version - Defined by the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option; see :ref:`ug_zigbee_configuring_components_ota` for details.

If all values are accepted, the OTA Upgrade Client downloads the first fragment of the firmware image.

Once the download is started, all received data fragments are passed to the :ref:`lib_dfu_multi_image` library.
The library takes care of where the upgrade candidate is stored, depending on the image type that is being downloaded.

When the download is completed, the download client sends an appropriate event.
At this point, the received firmware is tagged as an upgrade candidate and the OTA server is queried for an update time.

Once the OTA server triggers the update process, the library sends a :c:enumerator:`ZIGBEE_FOTA_EVT_FINISHED` callback event.
When the consumer of the library receives this event, it should issue a reboot command to apply the upgrade.

.. _lib_zigbee_fota_options:

Configuration
*************

To enable the Zigbee FOTA library, set the :kconfig:option:`CONFIG_ZIGBEE_FOTA` Kconfig option.

To configure the Zigbee FOTA library, use the following options:

* :kconfig:option:`CONFIG_ZIGBEE_FOTA_HW_VERSION`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_DATA_BLOCK_SIZE`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_ENDPOINT`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_PROGRESS_EVT`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_TYPE`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_COMMENT`
* :kconfig:option:`CONFIG_ENABLE_ZIGBEE_FOTA_MIN_HW_VERSION`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION`
* :kconfig:option:`CONFIG_ENABLE_ZIGBEE_FOTA_MAX_HW_VERSION`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_SERVER_DISOVERY_INTERVAL_HRS`
* :kconfig:option:`CONFIG_ZIGBEE_FOTA_IMAGE_QUERY_INTERVAL_MIN`

For detailed steps about configuring the library in a Zigbee sample or application, see :ref:`ug_zigbee_configuring_components_ota`.

.. _lib_zigbee_fota_limitations:

Limitations
***********

The Zigbee FOTA library has the following limitations:

* The endpoint definition in the library includes the endpoint ID, defined with :kconfig:option:`CONFIG_ZIGBEE_FOTA_ENDPOINT`.
  When using the Zigbee FOTA library, this endpoint ID cannot be used for other endpoints.
* The Zigbee FOTA upgrades are currently only supported on the nRF52840 DK (PCA10056) and nRF5340 DK (PCA10095).
* The Zigbee FOTA library does not currently support bootloader upgrades.

Additionally, the following limitations apply on the nRF5340 SoCs:

* It is required to use external flash to enable the Zigbee FOTA library.
* By default, only the full upgrades (to both application and network core) are allowed.
  Disable the :kconfig:option:`CONFIG_NRF53_ENFORCE_IMAGE_VERSION_EQUALITY` Kconfig option to build update images without inter-dependencies so that they can be applied independently.
* It is impossible to enable :kconfig:option:`CONFIG_BOOT_SWAP_USING_MOVE` Kconfig option.
  As a result, the fallback recovery is not available and any valid upgrade will overwrite the previous image.
  The call to the :c:func:`boot_write_img_confirmed()` will have no effect.
* The current DFU limitations and dependencies are enforced by the :kconfig:option:`CONFIG_NRF53_MULTI_IMAGE_UPDATE` Kconfig option.
* The version of the network core image is always set to the same value as the application core image.
  Its value can be configured using the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.
* The MCUboot header is not stored inside the network core flash memory.
  As a result, it is impossible to read the version of the currently running network core image.

API documentation
*****************

| Header file: :file:`include/zigbee/zigbee_fota.h`
| Source files: :file:`subsys/zigbee/lib/zigbee_fota/src/`

.. doxygengroup:: zigbee_fota
   :project: nrf
   :members:
