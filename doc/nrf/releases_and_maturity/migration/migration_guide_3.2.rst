.. _migration_3.2:

Migration guide for |NCS| v3.2.0
################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.1.0 to |NCS| v3.2.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.2_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

nRF54L
======

This section describes the changes specific to the nRF54L series SoCs and DKs support in the |NCS|.

nRF54L pin cross power-domain use
---------------------------------

.. toggle::

   * You must enable the Constant Latency sub-power mode from the application to allow cross power-domain pin mapping.
     For details, see the :ref:`ug_nrf54l_pinmap` page.

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.

nRF54H20 IronSide SE binaries
-----------------------------

.. toggle::

   * The nRF54H20 IronSide SE binaries have been updated to version v23.1.1+20.
     Starting from the |NCS| v3.2.0, you should always upgrade your nRF54H20 IronSide SE binaries to the latest version.

     For more information, see:

     * :ref:`abi_compatibility` for details about the SoC binaries.
     * :ref:`ug_nrf54h20_ironside_se_update` for instructions on updating the SoC binaries.

nRF54H20 power management
-------------------------

.. toggle::

   * The Kconfig options :kconfig:option:`CONFIG_PM_S2RAM` and :kconfig:option:`CONFIG_PM_S2RAM_CUSTOM_MARKING` have been reworked to be managed automatically based on the suspend-to-ram ``power-states`` in the devicetree.
     Any occurrences of ``CONFIG_PM_S2RAM=y`` and ``CONFIG_PM_S2RAM_CUSTOM_MARKING=y`` must be removed.

     The default value of :kconfig:option:`CONFIG_PM_S2RAM` has changed from ``n`` to ``y``.
     If your application explicitly disables S2RAM or relies on S2RAM being disabled, remove any instance of ``CONFIG_PM_S2RAM=n`` and instead disable the ``s2ram`` power state in the devicetree.

     To disable the ``s2ram`` power state, see the following DTS example:

     .. code-block:: dts

        &s2ram {
                status = "disabled";
        };

Samples and applications
========================

This section describes the changes related to samples and applications.

nRF Desktop
-----------

.. toggle::

   * The configuration of the ``nrf54h20dk/nrf54h20/cpuapp`` board target has been updated in the nRF Desktop application to migrate from the SUIT solution to the IronSide SE solution.
     To migrate the configuration for your nRF Desktop based application, complete the following steps:

     1. Remove the Kconfig options related to the SUIT solution in your project configuration files (for example, :file:`prj.conf`).
     #. Configure the MCUboot bootloader in your sysbuild configuration file:

        * :kconfig:option:`SB_CONFIG_BOOTLOADER_MCUBOOT` to ``y``.
        * :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP` to ``y``.
        * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519` to ``y``.
        * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE` to ``y``.
        * :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` to the path of the private key file which is used to sign the DFU package.
        * :kconfig:option:`SB_CONFIG_MCUBOOT_IMAGES_ROM_END_OFFSET_AUTO` to ``ipc_radio;ipc_radio_secondary_app``.
          In the case of a merged image slot, space for the MCUboot trailer is only reserved in the radio slots.

        .. note::
           The nRF Desktop configuration for the ``nrf54h20dk/nrf54h20/cpuapp`` board target uses the MCUboot bootloader in the direct-xip mode.
           You can configure other MCUboot bootloader modes (for example, the swap mode) in the nRF Desktop application, but they are not used as part of the nRF Desktop configuration for the ``nrf54h20dk/nrf54h20/cpuapp`` board target and are not covered by this migration guide.
           The nRF Desktop application also configures the MCUboot bootloader to use the merged image slot that combines both application and radio core images.
           The merged image slot is the default configuration for the direct-xip mode.
           Other ways of partitioning the application and radio core images are not covered by this migration guide.

     #. Add the configuration files for the MCUboot bootloader image to your application configuration.
        See the :file:`nrf/applications/nrf_desktop/configuration/nrf54h20dk_nrf54h20_cpuapp/images/mcuboot` directory for an example.

        Note the following Kconfig options that are specific to the nRF54H configuration of the MCUboot bootloader:

        * :kconfig:option:`CONFIG_SOC_EARLY_RESET_HOOK` to ``y`` - This option is required to support the Suspend to RAM (S2RAM) functionality in the application.
          With this Kconfig option, the MCUboot bootloader can detect a wake-up from S2RAM and redirect the execution to the application’s resume routine.
        * :kconfig:option:`CONFIG_POWER_DOMAIN` to ``n`` - This option is required to disable the power domain management in the bootloader and simplify its configuration.
        * :kconfig:option:`CONFIG_NRF_SECURITY`, :kconfig:option:`CONFIG_MULTITHREADING` and :kconfig:option:`CONFIG_PSA_SSF_CRYPTO_CLIENT` to ``y`` - These options are required to support the hardware cryptography in the MCUboot bootloader and its dependencies.

     #. Since the MCUboot bootloader in the direct-xip mode uses a merged image slot for the ``nrf54h20dk/nrf54h20/cpuapp`` board target, define the custom memory layout in DTS (the ``partitions`` DTS node) and ensure that this DTS customization is propagated to every image that is built as part of the nRF Desktop application.
        See the :file:`nrf/applications/nrf_desktop/configuration/nrf54h20dk_nrf54h20_cpuapp/memory_map.dtsi` file for an example of the memory layout file and the :file:`nrf/applications/nrf_desktop/configuration/nrf54h20dk_nrf54h20_cpuapp/images/mcuboot/app.overlay` file for an example integration of the custom memory layout into the MCUboot bootloader image.
        Apart from the MCUboot bootloader image, include the custom memory layout in the following images:

        * The ``nrf_desktop`` image (the default application image)
        * The ``ipc_radio`` image
        * The ``uicr`` image

        To verify that the custom memory layout is propagated to all the images, use the :file:`<build_dir>/<image_name>/zephyr/zephyr.dts` file and validate it for each image.

        .. note::
           The |NCS| v3.2.0 introduces a new image ``uicr`` for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.
           Include the custom memory layout in the ``uicr`` image as well to prevent runtime issues.
           See the :file:`nrf/applications/nrf_desktop/configuration/nrf54h20dk_nrf54h20_cpuapp/images/uicr/app.overlay` file for an example integration of the custom memory layout into the ``uicr`` image.

        Assign the secondary image partition to the ``secondary_app_partition`` DTS label in the DTS configuration of your primary image:

        * For the ``nrf_desktop`` (primary) image and the ``mcuboot_secondary_app`` (secondary) image, use the following DTS configuration:

          .. code-block::

             secondary_app_partition: &cpuapp_slot1_partition {};

        * For the ``ipc_radio`` (primary) image and the ``ipc_radio_secondary_app`` (secondary) image, use the following DTS configuration:

          .. code-block::

             secondary_app_partition: &cpurad_slot1_partition {};

     #. Optionally, you can enable the following Kconfig options to improve the HID report rate over USB by suspending Bluetooth when USB is connected:

        * :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE`
        * :kconfig:option:`CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB`

     #. Remove the usage of the following Kconfig options related to the S2RAM functionality:

        * :kconfig:option:`CONFIG_PM_S2RAM`
        * :kconfig:option:`CONFIG_PM_S2RAM_CUSTOM_MARKING`

        .. note::
           The S2RAM functionality is now enabled by default for the ``nrf54h20dk/nrf54h20/cpuapp`` board target when the :kconfig:option:`CONFIG_PM` Kconfig option is enabled.
           The setting of the :kconfig:option:`CONFIG_PM_S2RAM` Kconfig option is now controlled by the devicetree (DTS) description.

     #. Remove the ``pwmleds`` DTS node from the application configuration to prevent the :kconfig:option:`CONFIG_LED_PWM` and :kconfig:option:`CONFIG_CAF_LEDS_PWM` Kconfig options from being enabled for the ``nrf54h20dk/nrf54h20/cpuapp`` board target.

        .. code-block::

           / {
             /delete-node/ pwmleds;

             aliases {
               /delete-property/ pwm-led0;
             };
           };

        .. note::
           The nRF Desktop configuration for the ``nrf54h20dk/nrf54h20/cpuapp`` board target relies on GPIO LEDs - the PWM LEDs are not used.

     #. Disable Link Time Optimization (LTO) as a workaround for linker warnings about the type mismatch (``-Wlto-type-mismatch``) that are caused by the :file:`device_deps.c` file (used by the Zephyr power domain driver):

        * :kconfig:option:`CONFIG_LTO`
        * :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION`

     #. Replace the ``interface-name`` property with the ``label`` property in all DTS nodes that set the ``compatible`` property to ``zephyr,hid-device``.
        See the :file:`nrf/applications/nrf_desktop/configuration/nrf54h20dk_nrf54h20_cpuapp/app.overlay` file for an example.

     For more information regarding differences between SUIT and IronSide SE solutions, see the :ref:`migration_3.1_54h_suit_ironside` document.

Matter
------

.. toggle::

   * For the Matter samples and applications:

     * All Matter over Wi-Fi samples and applications now store a portion of the application code related to the nRF70 Series Wi-Fi firmware in external flash memory by default.

       There are two consequences of this change:

       * The partition map has been changed.
         You cannot perform DFU between the previous |NCS| versions and v3.2.0 unless you disable storing of the Wi-Fi firmware patch in external memory.
       * The application code size is reduced, but the programming process may take longer when performing the full erase, because the entire external flash memory is erased before programming the Wi-Fi firmware patch.

       When using the ``west flash`` command, the default behavior is to erase the entire external memory before programming the Wi-Fi firmware patch.
       To reduce programming time, you can add the ``--ext-erase-mode=ranges`` option to erase only the necessary memory ranges:

       .. code-block:: console

          west flash --ext-erase-mode=ranges

       The longer programming time also occurs when using the :guilabel:`Erase and Flash to Board` option in the |nRFVSC|.
       To speed up the process in the |nRFVSC|, use the :guilabel:`Flash` button instead of :guilabel:`Erase and Flash to Board` in the `Actions View`_.

       To disable storing the Wi-Fi firmware patch in external memory and revert to the previous approach, complete the following steps:

       1. Remove the Wi-Fi firmware patch partition from the partition list.
       #. Set the :kconfig:option:`SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE` Kconfig option to ``n``.
       #. Set the :kconfig:option:`SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH` Kconfig option to ``n``.
       #. Set the :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` Kconfig option to ``2``.

     * The Kconfig options ``CONFIG_CHIP_SPI_NOR`` and ``CONFIG_CHIP_QSPI_NOR`` have been removed.
       Instead, use the standard options :kconfig:option:`CONFIG_SPI_NOR` and :kconfig:option:`CONFIG_NORDIC_QSPI_NOR`.
       The configuration system will now automatically enable these options when the corresponding SPI or QSPI device is specified in the devicetree.
       This change ensures more consistent configuration by relying on the standard Kconfig options when external NOR flash devices are present.

    * All Matter over Wi-Fi samples and applications now enable the :kconfig:option:`CONFIG_CHIP_WIFI` and :kconfig:option:`CONFIG_WIFI_NRF70` Kconfig options, depending on the board used.
      Previously, :kconfig:option:`CONFIG_CHIP_WIFI` was enabled in the Matter stack configuration if the nRF7002 DK or nRF7002 EK was used, which caused issues when building the application with custom boards.

      To build your custom board with Wi-Fi support, set both the :kconfig:option:`CONFIG_CHIP_WIFI` and :kconfig:option:`CONFIG_WIFI_NRF70` Kconfig options to ``y``.

    * The Matter build system is transitioning to a code-driven approach for Data Model and Cluster configuration handling.
      This approach assumes gradual replacement of the configuration based on the ZAP files and the ZAP-generated code, and handling the configuration in the source code.
      This change has the following impacts:

      * The :file:`zap-generated` directory now includes the :file:`CodeDrivenCallback.h` and :file:`CodeDrivenInitShutdown.cpp` files.
        Invoking the ``west zap-generate`` command removes command handler implementations from the existing :file:`IMClusterCommandHandler.cpp` file.
        To fix this, you must now manually create the :file:`CodeDrivenCallback.h` and :file:`CodeDrivenInitShutdown.cpp` files, and implement the ``MatterClusterServerInitCallback`` and ``MatterClusterServerShutdownCallback`` callbacks to handle server initialization, shutdown, and Matter cluster commands.
        Ensure that these callbacks contain your application's command handling logic.
      * The code-driven approach is not yet fully implemented for all available clusters, but the coverage will be increasing and it is used for the newly created clusters.
        The following clusters are already ported using the code-driven approach:

        * Access Control
        * Administrator Commissioning
        * Basic Information
        * Binding
        * Boolean State
        * Descriptor
        * Diagnostic Logs
        * Ethernet Network Diagnostics
        * Fixed Label
        * General Commissioning
        * General Diagnostics
        * Group Key Management
        * Groupcast
        * Identify
        * Localization Configuration
        * OTA Software Update Provider
        * Operational Credentials
        * Push AV Stream Transport
        * Software Diagnostics
        * Time Format Localization
        * User Label
        * Wi-Fi Network Diagnostics

        For the full list of clusters and their migration status, see the `Matter Clusters Code-Driven support`_ file.

      * By default, all the mandatory attributes are enabled for the cluster.
        To enable an optional attribute or set an optional feature in the feature map, you must do that in the source code by calling dedicated methods.
        For example, to enable the Positioning feature and the ``CountdownTime`` optional attribute for the Closure Control cluster, perform the following operations:

        * Implement a delegate class for the Closure Control cluster.
          See the :file:`samples/matter/closure/src/closure_control_endpoint.h` file for an example.
        * Enable the attribute and set the feature on cluster initialization.
          See the ``ClosureControlEndpoint::Init`` function in the :file:`samples/matter/closure/src/closure_control_endpoint.cpp` file for an example.

      * To enable an optional cluster, you must register it in the source code by calling a dedicated method.
        For example, to enable the Identify cluster, implement the following code:

        .. code-block:: C++

           #include <data-model-providers/codegen/CodegenDataModelProvider.h>
           #include <app/clusters/identify-server/IdentifyCluster.h>

           chip::app::RegisteredServerCluster<chip::app::Clusters::IdentifyCluster> mIdentifyCluster;
           chip::app::CodegenDataModelProvider::Instance().Registry().Register(mIdentifyCluster.Registration());

      * To enable a cluster extension for the cluster that is already implemented using the code-driven approach, you must inherit from this cluster delegate class and implement the methods to handle the customized part.
        You also have to unregister the original cluster delegate and register the customized one.
        For example, to enable a cluster extension for the Basic Information cluster, perform the following operations:

        * Inherit from the ``BasicInformationCluster`` class and override the methods to handle the customized part.
          See the :file:`samples/matter/manufacturer_specific/src/basic_information_extension.h` file for an example.
        * Implement the body of overridden methods to handle the custom attributes, commands, and events.
          See the :file:`samples/matter/manufacturer_specific/src/basic_information_extension.cpp` file for an example.
        * Unregister the original cluster delegate and register the customized one.
          See the ``AppTask::StartApp`` function in the :file:`samples/matter/manufacturer_specific/src/app_task.cpp` file for an example.

    * :ref:`matter_lock_sample` sample:

      * The :kconfig:option:`CONFIG_BT_FIXED_PASSKEY` Kconfig option has been deprecated, replace it with the :kconfig:option:`CONFIG_BT_APP_PASSKEY` Kconfig option.
        Now, if you want to use a fixed passkey for the Matter Lock NUS service, register the :c:member:`bt_conn_auth_cb.app_passkey` callback in the :c:struct:`bt_conn_auth_cb` structure.

        For example:

       .. code-block:: c

          static uint32_t AuthAppPasskey(struct bt_conn *conn)
          {
              return 123456;
          }

          static struct bt_conn_auth_cb sConnAuthCallbacks = {
              .app_passkey = AuthAppPasskey,
          };

Serial LTE modem
----------------

.. toggle::

   * The Serial LTE modem application has been removed.
     Instead, use `Serial Modem`_, an |NCS| add-on application.

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lte_lc_readme` library:

     * The type of the :c:member:`lte_lc_evt.modem_evt` field has been changed to :c:struct:`lte_lc_modem_evt`.
       The modem event type can be determined from the :c:member:`lte_lc_modem_evt.type` field.
       Applications using modem events need to be updated to read the event type from ``modem_evt.type`` instead of ``modem_evt``.

     * Modem events ``LTE_LC_MODEM_EVT_CE_LEVEL_0``, ``LTE_LC_MODEM_EVT_CE_LEVEL_1``, ``LTE_LC_MODEM_EVT_CE_LEVEL_2`` and ``LTE_LC_MODEM_EVT_CE_LEVEL_3`` have been replaced by event :c:enumerator:`LTE_LC_MODEM_EVT_CE_LEVEL`.
       The CE level can be read from :c:member:`lte_lc_modem_evt.ce_level`.

     * Changed the order of the :c:enumerator:`LTE_LC_MODEM_EVT_SEARCH_DONE` modem event, and registration and cell related events.
       When the modem has completed the network selection, the registration and cell related events (:c:enumerator:`LTE_LC_EVT_NW_REG_STATUS`, :c:enumerator:`LTE_LC_EVT_CELL_UPDATE`, :c:enumerator:`LTE_LC_EVT_LTE_MODE_UPDATE` and :c:enumerator:`LTE_LC_EVT_PSM_UPDATE`) are sent first, followed by the :c:enumerator:`LTE_LC_MODEM_EVT_SEARCH_DONE` modem event.
       If the application depends on the order of the events, it may need to be updated.

   * :ref:`nrf_security_readme` library:

      * The ``CONFIG_CRACEN_PROVISION_PROT_RAM_INV_DATA`` Kconfig option has been renamed to :kconfig:option:`CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT`.

Trusted Firmware-M
==================

.. toggle::

   * Trusted Firmware-M changed how data is stored and read in the Protected Storage partition.
     As a consequence, the applications that build with TF-M (``*/ns`` board targets) and want to perform a firmware upgrade to this |NCS| release will not be able to read the existing Protected Storage data with the default configuration.
     To enable reading the Protected Storage data from a previous release, make sure that the application enables the :kconfig:option:`CONFIG_TFM_PS_SUPPORT_FORMAT_TRANSITION` Kconfig option.

.. _migration_3.2_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

MCUboot
-------

.. toggle::

   * The default C library for MCUboot has changed to picolibc.
     Picolibc is recommended over the minimal C library as it is a fully developed and supported C library designed for application usage.
     If you have not explicitly specified the C library in your sysbuild project for MCUboot using either a :file:`sysbuild/mcuboot/prj.conf` file or :file:`sysbuild/mcuboot.conf` file, picolibc will be used by default.
     To set picolibc in your project, use the :kconfig:option:`CONFIG_PICOLIBC` Kconfig option.
     If you need to use the minimal C library (which is not recommended outside of testing scenarios), use the :kconfig:option:`CONFIG_MINIMAL_LIBC` Kconfig option.
   * MCUboot image IDs are no longer taken from sysbuild Kconfig options and are instead automatically assigned.
     See :ref:`sysbuild_assigned_images_ids` for details on how to get the values from the sysbuild cache as these are no longer available in the sysbuild Kconfig tree.
     Application Kconfig values for image IDs remain present and their functionality is the same as in |NCS| v3.1.0.

Libraries
=========

This section describes the changes related to libraries.

PDN library
-----------

.. toggle::

   * The :ref:`pdn_readme` library has been deprecated in favor of the PDN management functionality provided by the :ref:`lte_lc_readme` library and will be removed in a future |NCS| release.

     To migrate your application to use LTE link control PDN management instead of the PDN library, complete the following steps:

     #. Kconfig options:

        * Replace:

           * The ``CONFIG_PDN`` Kconfig option with the :kconfig:option:`CONFIG_LTE_LC_PDN_MODULE` Kconfig option, which is available when the :kconfig:option:`CONFIG_LTE_LINK_CONTROL` Kconfig option is enabled.


           * The ``CONFIG_PDN_DEFAULTS_OVERRIDE`` and related PDN Kconfig options with the corresponding LTE link control PDN defaults options:

             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_APN`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV4`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV6`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_IPV4V6`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_FAM_NONIP`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_NONE`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_PAP`
             * :kconfig:option:`CONFIG_LTE_LC_PDN_DEFAULT_AUTH_CHAP`

        * Remove:

           * The ``CONFIG_HEAP_MEM_POOL_ADD_SIZE_PDN`` Kconfig option.
             Instead, use the :kconfig:option:`CONFIG_HEAP_MEM_POOL_ADD_SIZE_LTE_LINK_CONTROL` Kconfig option.

     #. Replace header files:

        * Remove:

          .. code-block:: C

             #include <modem/pdn.h>

        * Add:

          .. code-block:: C

             #include <modem/lte_lc.h>

     #. Replace PDN API calls:

        * Replace the PDP context management APIs:

          * :c:func:`pdn_ctx_create` with :c:func:`lte_lc_pdn_ctx_create`
          * :c:func:`pdn_ctx_configure` with :c:func:`lte_lc_pdn_ctx_configure`
          * :c:func:`pdn_ctx_auth_set` with :c:func:`lte_lc_pdn_ctx_auth_set`
          * :c:func:`pdn_ctx_destroy` with :c:func:`lte_lc_pdn_ctx_destroy`

        * Replace the PDN connection control APIs:

          * :c:func:`pdn_activate` with :c:func:`lte_lc_pdn_activate`
          * :c:func:`pdn_deactivate` with :c:func:`lte_lc_pdn_deactivate`

        * Replace the PDN information APIs:

          * :c:func:`pdn_id_get` with :c:func:`lte_lc_pdn_id_get`
          * :c:func:`pdn_dynamic_info_get` with :c:func:`lte_lc_pdn_dynamic_info_get`
          * :c:func:`pdn_default_apn_get` with :c:func:`lte_lc_pdn_default_ctx_apn_get`

        * Replace the ESM error string helper:

          * :c:func:`pdn_esm_strerror` with :c:func:`lte_lc_pdn_esm_strerror`

        * Replace the default-context callback registration:

          * :c:func:`pdn_default_ctx_cb_reg` and :c:func:`pdn_default_ctx_cb_dereg` with :c:func:`lte_lc_register_handler` in combination with :c:func:`lte_lc_pdn_default_ctx_events_enable` and :c:func:`lte_lc_pdn_default_ctx_events_disable`.
            The PDN events are handled using the :c:member:`lte_lc_evt.pdn` member as documented in the :ref:`lte_lc_pdn` page.

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Memfault integration
--------------------

.. toggle::

   * The options have changed for the ``CONFIG_MEMFAULT_NCS_DEVICE_ID_*`` choice, which sets the Memfault Device Serial:

     * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID` (new, and default) - Use ``hw_id`` provided device ID, which is also what nRF Cloud uses for device identity.
       See the :ref:`lib_hw_id` library for options for device ID source.
     * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_STATIC` - Used to set a custom build-time defined static device ID, primarily useful for testing.
     * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME` - Use a runtime-applied device ID, commonly used when the serial number of the device is written into settings at manufacturing time, for example.
     * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI` (deprecated) - Use the LTE modem IMEI as the device ID.
     * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC` (deprecated) - Use the network interface MAC address as the device ID.

Drivers
=======

This section provides detailed lists of changes by drivers.

nrfx
----

.. toggle::

   * nrfx version has been updated to 4.0.
     For nrfx changes see `nrfx 4.0 migration note`_.

     Additionally, to migrate a Zephyr build system application that uses nrfx API directly, complete the following steps:

     #. Kconfig options:

        * Replace:

            * Use :kconfig:option:`CONFIG_NRFX_GPPI` instead of the following:

               * ``CONFIG_NRFX_DPPI``
               * ``CONFIG_NRFX_DPPI0``
               * ``CONFIG_NRFX_DPPI00``
               * ``CONFIG_NRFX_DPPI10``
               * ``CONFIG_NRFX_DPPI20``
               * ``CONFIG_NRFX_DPPI30``
               * ``CONFIG_NRFX_DPPI020``
               * ``CONFIG_NRFX_DPPI120``
               * ``CONFIG_NRFX_DPPI130``
               * ``CONFIG_NRFX_DPPI131``
               * ``CONFIG_NRFX_DPPI132``
               * ``CONFIG_NRFX_DPPI133``
               * ``CONFIG_NRFX_DPPI134``
               * ``CONFIG_NRFX_DPPI135``
               * ``CONFIG_NRFX_DPPI136``
               * ``CONFIG_NRFX_PPI``

            * Use :kconfig:option:`CONFIG_NRFX_I2S` instead of the following:

               * ``CONFIG_NRFX_I2S0``
               * ``CONFIG_NRFX_I2S20``

            * Use :kconfig:option:`CONFIG_NRFX_PDM` instead of the following:

               * ``CONFIG_NRFX_PDM0``
               * ``CONFIG_NRFX_PDM20``
               * ``CONFIG_NRFX_PDM21``

            * Use :kconfig:option:`CONFIG_NRFX_PWM` instead of the following:

               * ``CONFIG_NRFX_PWM0``
               * ``CONFIG_NRFX_PWM1``
               * ``CONFIG_NRFX_PWM2``
               * ``CONFIG_NRFX_PWM3``
               * ``CONFIG_NRFX_PWM20``
               * ``CONFIG_NRFX_PWM21``
               * ``CONFIG_NRFX_PWM22``
               * ``CONFIG_NRFX_PWM120``
               * ``CONFIG_NRFX_PWM130``
               * ``CONFIG_NRFX_PWM131``
               * ``CONFIG_NRFX_PWM132``
               * ``CONFIG_NRFX_PWM133``

            * Use :kconfig:option:`CONFIG_NRFX_QDEC` instead of the following:

               * ``CONFIG_NRFX_QDEC0``
               * ``CONFIG_NRFX_QDEC1``
               * ``CONFIG_NRFX_QDEC20``
               * ``CONFIG_NRFX_QDEC21``
               * ``CONFIG_NRFX_QDEC130``
               * ``CONFIG_NRFX_QDEC131``

            * Use :kconfig:option:`CONFIG_NRFX_SPIM` instead of the following:

               * ``CONFIG_NRFX_SPIM0``
               * ``CONFIG_NRFX_SPIM1``
               * ``CONFIG_NRFX_SPIM2``
               * ``CONFIG_NRFX_SPIM3``
               * ``CONFIG_NRFX_SPIM4``
               * ``CONFIG_NRFX_SPIM00``
               * ``CONFIG_NRFX_SPIM01``
               * ``CONFIG_NRFX_SPIM20``
               * ``CONFIG_NRFX_SPIM21``
               * ``CONFIG_NRFX_SPIM22``
               * ``CONFIG_NRFX_SPIM23``
               * ``CONFIG_NRFX_SPIM24``
               * ``CONFIG_NRFX_SPIM30``
               * ``CONFIG_NRFX_SPIM120``
               * ``CONFIG_NRFX_SPIM121``
               * ``CONFIG_NRFX_SPIM130``
               * ``CONFIG_NRFX_SPIM131``
               * ``CONFIG_NRFX_SPIM132``
               * ``CONFIG_NRFX_SPIM133``
               * ``CONFIG_NRFX_SPIM134``
               * ``CONFIG_NRFX_SPIM135``
               * ``CONFIG_NRFX_SPIM136``
               * ``CONFIG_NRFX_SPIM137``

            * Use :kconfig:option:`CONFIG_NRFX_SPIS` instead of the following:

               * ``CONFIG_NRFX_SPIS0``
               * ``CONFIG_NRFX_SPIS1``
               * ``CONFIG_NRFX_SPIS2``
               * ``CONFIG_NRFX_SPIS3``
               * ``CONFIG_NRFX_SPIS00``
               * ``CONFIG_NRFX_SPIS01``
               * ``CONFIG_NRFX_SPIS20``
               * ``CONFIG_NRFX_SPIS21``
               * ``CONFIG_NRFX_SPIS22``
               * ``CONFIG_NRFX_SPIS23``
               * ``CONFIG_NRFX_SPIS24``
               * ``CONFIG_NRFX_SPIS30``
               * ``CONFIG_NRFX_SPIS120``
               * ``CONFIG_NRFX_SPIS130``
               * ``CONFIG_NRFX_SPIS131``
               * ``CONFIG_NRFX_SPIS132``
               * ``CONFIG_NRFX_SPIS133``
               * ``CONFIG_NRFX_SPIS134``
               * ``CONFIG_NRFX_SPIS135``
               * ``CONFIG_NRFX_SPIS136``
               * ``CONFIG_NRFX_SPIS137``

            * Use :kconfig:option:`CONFIG_NRFX_TIMER` instead of the following:

               * ``CONFIG_NRFX_TIMER0``
               * ``CONFIG_NRFX_TIMER1``
               * ``CONFIG_NRFX_TIMER2``
               * ``CONFIG_NRFX_TIMER3``
               * ``CONFIG_NRFX_TIMER4``
               * ``CONFIG_NRFX_TIMER00``
               * ``CONFIG_NRFX_TIMER10``
               * ``CONFIG_NRFX_TIMER20``
               * ``CONFIG_NRFX_TIMER21``
               * ``CONFIG_NRFX_TIMER22``
               * ``CONFIG_NRFX_TIMER23``
               * ``CONFIG_NRFX_TIMER24``
               * ``CONFIG_NRFX_TIMER020``
               * ``CONFIG_NRFX_TIMER021``
               * ``CONFIG_NRFX_TIMER022``
               * ``CONFIG_NRFX_TIMER120``
               * ``CONFIG_NRFX_TIMER121``
               * ``CONFIG_NRFX_TIMER130``
               * ``CONFIG_NRFX_TIMER131``
               * ``CONFIG_NRFX_TIMER132``
               * ``CONFIG_NRFX_TIMER133``
               * ``CONFIG_NRFX_TIMER134``
               * ``CONFIG_NRFX_TIMER135``
               * ``CONFIG_NRFX_TIMER136``
               * ``CONFIG_NRFX_TIMER137``

            * Use :kconfig:option:`CONFIG_NRFX_TWIM` instead of the following:

               * ``CONFIG_NRFX_TWIM0``
               * ``CONFIG_NRFX_TWIM1``
               * ``CONFIG_NRFX_TWIM2``
               * ``CONFIG_NRFX_TWIM3``
               * ``CONFIG_NRFX_TWIM20``
               * ``CONFIG_NRFX_TWIM21``
               * ``CONFIG_NRFX_TWIM22``
               * ``CONFIG_NRFX_TWIM23``
               * ``CONFIG_NRFX_TWIM24``
               * ``CONFIG_NRFX_TWIM30``
               * ``CONFIG_NRFX_TWIM120``
               * ``CONFIG_NRFX_TWIM130``
               * ``CONFIG_NRFX_TWIM131``
               * ``CONFIG_NRFX_TWIM132``
               * ``CONFIG_NRFX_TWIM133``
               * ``CONFIG_NRFX_TWIM134``
               * ``CONFIG_NRFX_TWIM135``
               * ``CONFIG_NRFX_TWIM136``
               * ``CONFIG_NRFX_TWIM137``

            * Use :kconfig:option:`CONFIG_NRFX_TWIS` instead of the following:

               * ``CONFIG_NRFX_TWIS0``
               * ``CONFIG_NRFX_TWIS1``
               * ``CONFIG_NRFX_TWIS2``
               * ``CONFIG_NRFX_TWIS3``
               * ``CONFIG_NRFX_TWIS20``
               * ``CONFIG_NRFX_TWIS21``
               * ``CONFIG_NRFX_TWIS22``
               * ``CONFIG_NRFX_TWIS23``
               * ``CONFIG_NRFX_TWIS24``
               * ``CONFIG_NRFX_TWIS30``
               * ``CONFIG_NRFX_TWIS120``
               * ``CONFIG_NRFX_TWIS130``
               * ``CONFIG_NRFX_TWIS131``
               * ``CONFIG_NRFX_TWIS132``
               * ``CONFIG_NRFX_TWIS133``
               * ``CONFIG_NRFX_TWIS134``
               * ``CONFIG_NRFX_TWIS135``
               * ``CONFIG_NRFX_TWIS136``
               * ``CONFIG_NRFX_TWIS137``

            * Use :kconfig:option:`CONFIG_NRFX_UARTE` instead of the following:

               * ``CONFIG_NRFX_UARTE0``
               * ``CONFIG_NRFX_UARTE1``
               * ``CONFIG_NRFX_UARTE2``
               * ``CONFIG_NRFX_UARTE3``
               * ``CONFIG_NRFX_UARTE00``
               * ``CONFIG_NRFX_UARTE20``
               * ``CONFIG_NRFX_UARTE21``
               * ``CONFIG_NRFX_UARTE22``
               * ``CONFIG_NRFX_UARTE23``
               * ``CONFIG_NRFX_UARTE24``
               * ``CONFIG_NRFX_UARTE30``
               * ``CONFIG_NRFX_UARTE120``
               * ``CONFIG_NRFX_UARTE130``
               * ``CONFIG_NRFX_UARTE131``
               * ``CONFIG_NRFX_UARTE132``
               * ``CONFIG_NRFX_UARTE133``
               * ``CONFIG_NRFX_UARTE134``
               * ``CONFIG_NRFX_UARTE135``
               * ``CONFIG_NRFX_UARTE136``
               * ``CONFIG_NRFX_UARTE137``

            * Use :kconfig:option:`CONFIG_NRFX_WDT` instead of the following:

               * ``CONFIG_NRFX_WDT0``
               * ``CONFIG_NRFX_WDT1``
               * ``CONFIG_NRFX_WDT30``
               * ``CONFIG_NRFX_WDT31``
               * ``CONFIG_NRFX_WDT010``
               * ``CONFIG_NRFX_WDT011``
               * ``CONFIG_NRFX_WDT130``
               * ``CONFIG_NRFX_WDT131``
               * ``CONFIG_NRFX_WDT132``

        * Remove:

            * ``CONFIG_NRFX_PPIB``
            * ``CONFIG_NRFX_PPIB00``
            * ``CONFIG_NRFX_PPIB01``
            * ``CONFIG_NRFX_PPIB10``
            * ``CONFIG_NRFX_PPIB11``
            * ``CONFIG_NRFX_PPIB20``
            * ``CONFIG_NRFX_PPIB21``
            * ``CONFIG_NRFX_PPIB22``
            * ``CONFIG_NRFX_PPIB30``
