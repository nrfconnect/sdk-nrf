.. _migration_3.2:

Migration guide for |NCS| v3.2.0 (Working draft)
################################################

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

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.

nRF54H20 IronSide SE binaries
-----------------------------

.. toggle::

   * The nRF54H20 IronSide SE binaries have been updated to version v23.0.1+16.
     Starting from the |NCS| v3.2.0, you should always upgrade your nRF54H20 IronSide SE binaries to the latest version.

     For more information, see:

     * :ref:`abi_compatibility` for details about the SoC binaries.
     * :ref:`ug_nrf54h20_ironside_se_update` for instructions on updating the SoC binaries.

nRF54H20 power management
-------------------------

.. toggle::

   * The Kconfig options :kconfig:option:`PM_S2RAM` and :kconfig:option:`PM_S2RAM_CUSTOM_MARKING` have been reworked to be managed automatically based on the suspend-to-ram ``power-states`` in the devicetree.
     Any occurrences of ``CONFIG_PM_S2RAM=y`` and ``CONFIG_PM_S2RAM_CUSTOM_MARKING=y`` must be removed.
     Any occurrence of ``CONFIG_PM_S2RAM=n`` or when the code requires S2RAM state to be disabled (the default value of :kconfig:option:`PM_S2RAM` has changed from ``n`` to ``y``) must be replaced by disabling the ``s2ram`` power state in the devicetree.

     .. code-block:: dts

        &s2ram {
                status = "disabled";
        };

Samples and applications
========================

This section describes the changes related to samples and applications.

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

    * The :file:`zap-generated` directory now includes the :file:`CodeDrivenCallback.h` and :file:`CodeDrivenInitShutdown.cpp` files.
      The Matter build system gradually moves to the new approach of data model handling and does not auto-generate some bits of a code responsible for command handling anymore.
      Invoking the ``west zap-generate`` command removes command handler implementations from the existing :file:`IMClusterCommandHandler.cpp` file.
      To fix this, you must now manually create the :file:`CodeDrivenCallback.h` and :file:`CodeDrivenInitShutdown.cpp` files, and implement the ``MatterClusterServerInitCallback`` and ``MatterClusterServerShutdownCallback`` callbacks to handle server initialization, shutdown, and Matter cluster commands.
      Ensure that these callbacks contain your application's command handling logic as required.

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
          * :c:func:`pdn_default_apn_get` with :c:func:`lte_lc_pdn_ctx_default_apn_get`

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

* The options have changed for the ``CONFIG_MEMFAULT_NCS_DEVICE_ID_*`` choice, which sets the Memfault Device Serial:

  * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_HW_ID` (new, and default) - Use ``hw_id`` provided device ID, which is also what nRF Cloud uses for device identity. See the :ref:`lib_hw_id` library for options for device ID source.
  * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_STATIC` - Used to set a custom build-time defined static device ID, primarily useful for testing.
  * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME` - Use a runtime-applied device ID, commonly used when the serial number of the device is written into settings at manufacturing time, for example.
  * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_IMEI` (deprecated) - Use the LTE modem IMEI as the device ID.
  * :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID_NET_MAC` (deprecated) - Use the network interface MAC address as the device ID.
