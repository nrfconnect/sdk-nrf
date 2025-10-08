:orphan:

.. _migration_2.8:

Migration guide for |NCS| v2.8.0
################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.7.0 to |NCS| v2.8.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.8_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Build and configuration system
==============================

.. toggle::

   * Sysbuild now handles the following MCUboot image ID assignments:

     * MCUboot updates (using b0) are automatically assigned to MCUboot.
       The :kconfig:option:`SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES` Kconfig option must not be incremented to include this image.
     * Applications and MCUboot must now use the MCUboot assigned image ID Kconfig values to refer to image IDs instead of hardcoding them.
     * Applications interacting with the device using MCUboot serial recovery MCUmgr must use the image IDs assigned to them, as well as MCUboot or MCUmgr hooks.
     * Depending upon enabled images, some image IDs might differ in |NCS| 2.8 and higher than from previous releases.


Nordic Secure Immutable Bootloader (NSIB, B0, or B0n)
=====================================================

.. toggle::

   * Custom printing has been dropped in favor of using the logging subsystem, with output printed out to the default logging device.
     The ``CONFIG_SECURE_BOOT_DEBUG`` Kconfig option has been removed.
     To disable logging in b0 or b0n, set the :kconfig:option:`CONFIG_LOG` option to ``n``.
     To send logs over RTT instead of UART, apply the following settings:

     * Enable the :kconfig:option:`CONFIG_USE_SEGGER_RTT` and :kconfig:option:`CONFIG_RTT_CONSOLE` Kconfig options.
     * Disable the :kconfig:option:`CONFIG_UART_CONSOLE` and :kconfig:option:`CONFIG_SERIAL` Kconfig options.

nRF70 Series
============

.. toggle::

   * The nRF70 Series support is now part of Zephyr upstream and it requires the following changes:

     * The nRF70 Series driver namespace has been renamed from ``NRF700X`` to ``NRF70``.
       For example, ``CONFIG_NRF700X_RAW_DATA_RX`` to ``CONIFG_NRF70_RAW_DATA_RX``.
       Update your application configurations to use the new namespace.

     * The nRF70 Series driver now uses per-module kernel heap with a higher default.
       If a sample or an application uses the kernel heap but uses less than the default size, a build warning is displayed.
       Use the :kconfig:option:`CONFIG_HEAP_MEM_POOL_IGNORE_MIN` Kconfig option and enable it to suppress the warning.

   * The WPA supplicant is also now part of Zephyr upstream and it requires the following changes:

     * The WPA supplicant namespace has been renamed from ``WPA_SUPP`` to ``WIFI_NM_WPA_SUPPLICANT``.
       For example, ``CONFIG_WPA_SUPP=y`` to ``CONFIG_WIFI_NM_WPA_SUPPLICANT=y``.
       Update your application configurations to use the new namespace.

     * You need to reconcile the application heap and kernel heap usage appropriately to accommodate this switch from application to kernel heap.

   * The SR co-existence feature should now be explicitly enabled using the :kconfig:option:`CONFIG_NRF70_SR_COEX` Kconfig option.
     The RF switch feature should be enabled using the :kconfig:option:`CONFIG_NRF70_SR_COEX_RF_SWITCH` Kconfig option.

nRF54L Series
=============

.. toggle::

   * Use the :ref:`ZMS (Zephyr Memory Storage) <zephyr:zms_api>` storage system for all devices with RRAM memory technology.
     See the :ref:`zms_memory_storage` page for more details on how to enable ZMS for an nRF54L Series.

.. _migration_2.8_nrf54h:

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.
For more information on changes related to samples and applications usage on the nRF54H20 DK, see :ref:`migration_2.8_required_nrf54h`.

DK compatibility
----------------

.. toggle::

  * The |NCS| v2.8.0 is compatible only with the following versions of the nRF54H20 DK, PCA10175:

      * Engineering B - versions ranging from v0.8.0 to 0.8.2
      * Engineering C - v0.8.3 and later revisions

      Check the version number on your DK's sticker to verify its compatibility with the |NCS|.

Dependencies
------------

The following required dependencies for the nRF54H20 SoC and DK have been updated.

nRF54H20 BICR
+++++++++++++

.. toggle::

  * The nRF54H20 BICR has been updated (from the one supporting |NCS| v2.7.0).

    .. note::
       BICR update is not required if migrating from |NCS| v2.7.99-cs1 or v2.7.99-cs2.

    To update the BICR of your development kit while in Root of Trust, do the following:

    1. Download the `nRF54H20 DK BICR binary file`_.
    #. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

       .. note::
          On MacOS, connecting the DK might repeatedly trigger a popup displaying the message ``Disk Not Ejected Properly``.
          To disable this, run ``JLinkExe``, then run ``MSDDisable`` in the J-Link Commander interface.

    #. List all the connected development kits to see their serial number (matching the one on the DK's sticker)::

          nrfutil device list

    #. Move the BICR HEX file to a folder of your choice, then program the BICR by running nRF Util from that folder using the following command::

          nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware <path_to_bicr.hex> --core Application --serial-number <serial_number>

nRF54H20 SoC binaries
+++++++++++++++++++++

.. toggle::

  * The *nRF54H20 SoC binaries* bundle has been updated to version 0.7.0.

    .. caution::
       If migrating from |NCS| v2.7.0, before proceeding with the SoC binaries update, you must first update the BICR as described in the previous section.

    To update the SoC binaries bundle of your development kit while in Root of Trust, do the following:

    1. Download the nRF54H20 SoC binaries v0.7.0:

       * `nRF54H20 SoC binaries v0.7.0 for EngC DKs`_, compatible with the nRF54H20 DK v0.8.3 and later revisions
       * `nRF54H20 SoC binaries v0.7.0 for EngB DKs`_, compatible with the nRF54H20 DKs ranging from v0.8.0 to v0.8.2.

       .. note::
          On MacOS, ensure that the ZIP file is not unpacked automatically upon download.

    #. Purge the device as follows::

          nrfutil device recover --core Application --serial-number <serial_number>
          nrfutil device recover --core Network --serial-number <serial_number>

    #. Run ``west update``.
    #. Move the correct :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using one of the following commands, depending on your DK:

       * For Engineering B::

            nrfutil device x-suit-dfu --firmware nrf54h20_soc_binaries_v0.7.0_<revision>.zip --serial-number <serial_number>

       * For Engineering C::

            nrfutil device x-suit-dfu --firmware nrf54h20_soc_binaries_v0.7.0_<revision>.zip --serial-number <serial_number> --update-candidate-info-address 0x0e1ef340

nrfutil device
++++++++++++++

.. toggle::

  * ``nrfutil device`` has been updated to version 2.7.2.

    Install the nRF Util ``device`` command version 2.7.2 as follows::

       nrfutil install device=2.7.2 --force

    For more information, consult the `nRF Util`_ documentation.

nrfutil-trace
+++++++++++++

.. toggle::

  * ``nrfutil-trace`` has been updated to version 2.11.0.

    Install the nRF Util ``trace`` command version 2.11.0 as follows::

       nrfutil install trace=2.11.0 --force

    For more information, consult the `nRF Util`_ documentation.

nrf-regtool
+++++++++++

.. toggle::

  * ``nrf-regtool`` has been updated to version 8.0.0.

    1. Open nRF Connect for Desktop, navigate to the Toolchain Manager, select the v2.8 toolchain, and click the :guilabel:`Open terminal` button.
    #. In the terminal window, install ``nrf-regtool`` version 8.0.0 as follows::

          pip install nrf-regtool==8.0.0


SEGGER J-Link
+++++++++++++

.. toggle::

  * A new version of SEGGER J-Link is supported: `SEGGER J-Link` version 7.94i.

    .. note::
       On Windows, to update to the new J-link version, including the USB Driver for J-Link, you must manually install J-Link v7.94i from the command line, using the ``-InstUSBDriver=1`` parameter:

      1. Navigate to the download location of the J-Link executable and run one of the following commands:

          * From the Command Prompt::

               JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

          * From PowerShell::

               .\JLink_Windows_V794i_x86_64.exe -InstUSBDriver=1

      #. In the :guilabel:`Choose optional components` window, select :guilabel:`update existing installation`.
      #. Add the J-Link executable to the system path on Linux and MacOS, or to the environment variables on Windows, to run it from anywhere on the system.

  * The STM logging feature for the nRF54H20 SoC was tested using the J-Trace PRO V2 Cortex-M, with firmware compiled on ``Mar 28 2024 15:14:04``.
    Using this feature also requires ``nrfutil-trace`` version 2.10.0 or later.

nRF Connect Device Manager
++++++++++++++++++++++++++

.. toggle::

  * The nRF54H20 SUIT DFU feature now requires `nRF Connect Device Manager`_ version v2.2.2 or higher.

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem (SLM)
----------------------

.. toggle::

   * The handling of Release Assistance Indication (RAI) socket options has been updated in the ``#XSOCKETOPT`` command.
     The individual RAI-related socket options have been consolidated into a single ``SO_RAI`` option.
     You must modify your application to use the new ``SO_RAI`` option with the corresponding value to specify the RAI behavior.
     The changes are as follows:

     The ``SO_RAI_NO_DATA``, ``SO_RAI_LAST``, ``SO_RAI_ONE_RESP``, ``SO_RAI_ONGOING``, and ``SO_RAI_WAIT_MORE`` options have been replaced by the ``SO_RAI`` option with values from ``1`` to ``5``.

     Replace the following commands in your application code if they were used previously:

     * ``AT#XSOCKETOPT=1,50,`` with ``AT#XSOCKETOPT=1,61,1`` to indicate ``RAI_NO_DATA``.
     * ``AT#XSOCKETOPT=1,51,`` with ``AT#XSOCKETOPT=1,61,2`` to indicate ``RAI_LAST``.
     * ``AT#XSOCKETOPT=1,52,`` with ``AT#XSOCKETOPT=1,61,3`` to indicate ``RAI_ONE_RESP``.
     * ``AT#XSOCKETOPT=1,53,`` with ``AT#XSOCKETOPT=1,61,4`` to indicate ``RAI_ONGOING``.
     * ``AT#XSOCKETOPT=1,54,`` with ``AT#XSOCKETOPT=1,61,5`` to indicate ``RAI_WAIT_MORE``.

.. _migration_2.8_required_nrf54h:

nRF54H20
--------

.. toggle::

  * When using the nRF54H20 DK Engineering B (from v0.8.0 to 0.8.2), you must build samples and applications using the board revision 0.8.0 with the ``<board>@<revision>`` syntax.
    For example, ``nrf54h20dk@0.8.0/nrf54h20/cpuapp`` when building for the application core, or ``nrf54h20dk@0.8.0/nrf54h20/cpurad`` when building for the radio core.

  * When using SUIT DFU on the nRF54H20 SoC, the manifest sequence number is no longer configured through a :ref:`sysbuild <configuring_sysbuild>` Kconfig option.
    The values are now read from the :file:`VERSION` file, used for :ref:`zephyr:app-version-details` in Zephyr and the |NCS|.
    This change to the :ref:`sysbuild <configuring_sysbuild>` Kconfig option requires the following updates in the SUIT templates for your project:

       * Remove from all templates:

         .. code-block:: YAML

            suit-manifest-sequence-number: {{ sysbuild['config']['SB_CONFIG_SUIT_ENVELOPE_SEQUENCE_NUM'] }}

       * Add the line that corresponds to the manifest name, that is ``APP_ROOT_SEQ_NUM`` for the application root manifest:

         .. code-block:: YAML

            suit-manifest-sequence-number: {{ APP_ROOT_SEQ_NUM }}

    If the value of the sequence number was changed in your application, append the following line to the :file:`VERSION` file:

         .. code-block:: sh

            APP_ROOT_SEQ_NUM = <N>

    For the list of all variables, set through the :file:`VERSION`, refer to the ``ug_nrf54h20_suit_customize_dfu``.

  * When using MCU Manager, the ``Confirm`` command is now needed to trigger a device firmware update.
  * The build command to enable DFU from the external flash is now the following::

      west build ./ -b nrf54h20dk/nrf54h20/cpuapp -T sample.suit.smp_transfer.cache_push.extflash.bt

  * For updating using the SUIT Device Manager application, you can also use the following zip file: :file:`<main_application_build_directory>/zephyr/dfu_suit_recovery.zip`.
  * Some Kconfig options and SUIT manifests have been modified, changing names and configurations.
    Ensure the compatibility of your application with these changes.

Libraries
=========

This section describes the changes related to libraries.

AT command parser
-----------------

.. toggle::

   * The :c:func:`at_parser_cmd_type_get` has been renamed to :c:func:`at_parser_at_cmd_type_get`.

nRF Cloud
---------

.. toggle::

   * The :kconfig:option:`CONFIG_NRF_CLOUD_COAP_DOWNLOADS` Kconfig option has been enabled by default for nRF Cloud CoAP projects using the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA_POLL` or :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` Kconfig option.
     Set the :kconfig:option:`CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE` Kconfig option to at least ``80`` for P-GPS and ``192`` for FOTA.

nRF Security
------------

.. toggle::

   * The ``CONFIG_CRACEN_LOAD_KMU_SEED`` Kconfig option was renamed to :kconfig:option:`CONFIG_CRACEN_IKG_SEED_LOAD`.
   * The ``CONFIG_MBEDTLS_CIPHER_MODE_CFB`` and ``CONFIG_MBEDTLS_CIPHER_MODE_OFB`` Kconfig options have been removed.
     Use other cipher modes instead.

LTE link control library
------------------------

.. toggle::

   * For applications using :ref:`lte_lc_readme`:

     * Remove all instances of the :c:func:`lte_lc_init` function.
     * Replace the use of the :c:func:`lte_lc_deinit` function with the :c:func:`lte_lc_power_off` function.
     * Replace the use of the :c:func:`lte_lc_init_and_connect` function with the :c:func:`lte_lc_connect` function.
     * Replace the use of the :c:func:`lte_lc_init_and_connect_async` function with the :c:func:`lte_lc_connect_async` function.
     * Replace the use of the :c:macro:`LTE_LC_ON_CFUN` macro with the :c:macro:`NRF_MODEM_LIB_ON_CFUN` macro.
     * Remove the use of the ``CONFIG_LTE_NETWORK_USE_FALLBACK`` Kconfig option.
       Use the :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT` or :kconfig:option:`CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS` Kconfig option instead.
       In addition, you can control the priority between LTE-M and NB-IoT using the :kconfig:option:`CONFIG_LTE_MODE_PREFERENCE` Kconfig option.

     * The library has been reorganized into modules that are enabled via their respective Kconfig options.
       This change requires the following updates:

      * If your application uses:

         * :c:func:`lte_lc_conn_eval_params_get`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_CONN_EVAL_MODULE` Kconfig option.

      * If your application uses:

         * :c:enumerator:`LTE_LC_EVT_EDRX_UPDATE`
         * :c:func:`lte_lc_ptw_set`
         * :c:func:`lte_lc_edrx_param_set`
         * :c:func:`lte_lc_edrx_req`
         * :c:func:`lte_lc_edrx_get`
         * :kconfig:option:`CONFIG_LTE_EDRX_REQ`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_EDRX_MODULE` Kconfig option.

      * If your application uses:

         * :c:enumerator:`LTE_LC_EVT_NEIGHBOR_CELL_MEAS`
         * :c:func:`lte_lc_neighbor_cell_measurement_cancel`
         * :c:func:`lte_lc_neighbor_cell_measurement`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_NEIGHBOR_CELL_MEAS_MODULE` Kconfig option.

      * If your application uses:

         * :c:func:`lte_lc_periodic_search_request`
         * :c:func:`lte_lc_periodic_search_clear`
         * :c:func:`lte_lc_periodic_search_get`
         * :c:func:`lte_lc_periodic_search_set`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_PERIODIC_SEARCH_MODULE` Kconfig option.

      * If your application uses:

         * :c:enumerator:`LTE_LC_EVT_PSM_UPDATE`
         * :c:func:`lte_lc_psm_param_set`
         * :c:func:`lte_lc_psm_param_set_seconds`
         * :c:func:`lte_lc_psm_req`
         * :c:func:`lte_lc_psm_get`
         * :c:func:`lte_lc_proprietary_psm_req`
         * :kconfig:option:`CONFIG_LTE_PSM_REQ`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_PSM_MODULE` Kconfig option.

      * If your application uses:

         * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING`
         * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_ENTER`
         * :c:enumerator:`LTE_LC_EVT_MODEM_SLEEP_EXIT`
         * :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_MODEM_SLEEP_MODULE` Kconfig option.

      * If your application uses:

         * :c:enumerator:`LTE_LC_EVT_TAU_PRE_WARNING`
         * :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS`

         You must use the new :kconfig:option:`CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE` Kconfig option.

LwM2M carrier library
---------------------

.. toggle::

   The bootstrap from smartcard feature is no longer enabled by default in the library and the ``CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD`` Kconfig option has been removed.
   To continue using this functionality, the :ref:`lib_uicc_lwm2m` library must be included in the project by enabling the :kconfig:option:`CONFIG_UICC_LWM2M` Kconfig option.

Wi-Fi®
------

.. toggle::

   * For Wi-Fi credentials library:

     * Syntax for ``add`` command has been modified to support ``getopt`` model.
       For example, the following command with old syntax:
       ``wifi_cred add SSID WPA2-PSK password`` should be replaced with the following command with new syntax:
       ``wifi_cred add -s SSID -k 1 -p password``.
       ``wifi_cred add --help`` command will provide more information on the new syntax.

.. _migration_2.8_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Devicetree
==========

.. toggle::

   The ``nordic,owned-memory`` and ``nordic,owned-partitions`` bindings have been updated, making these properties deprecated:

     * ``owner-id``
     * ``perm-read``
     * ``perm-write``
     * ``perm-execute``
     * ``perm-secure``
     * ``non-secure-callable``

   It is recommended to use the ``nordic,access`` property instead.
   The board files and sample overlays in the |NCS| are already updated to use it.
   See :file:`ncs/zephyr/dts/bindings/reserved-memory/nordic,owned-memory.yaml` for more details.

   If both of the new and deprecated properties are set on the same devicetree node, then only ``nordic,access`` will take effect.
   Therefore, it may not be possible to override the default permissions of an existing memory node using the old properties.

   Example before:

   .. code-block:: devicetree

      &cpuapp_ram0x_region {
         compatible = "nordic,owned-memory";
         owner-id = <2>;
         perm-read;
         perm-write;
         perm-execute;
         perm-secure;
      };

   Example after:

   .. code-block:: devicetree

      &cpuapp_ram0x_region {
         compatible = "nordic,owned-memory";
         nordic,access = <NRF_OWNER_ID_APPLICATION NRF_PERM_RWXS>;
      };


Snippets
========

This section describes the changes related to snippets.

.. toggle::

   The existing snippet ``nrf70-debug`` has been removed and divided into three sub-snippets as below:

   * ``nrf70-driver-debug`` - To enable the nRF70 driver debug logs.
   * ``nrf70-driver-verbose-logs`` - To enable the nRF70 driver, firmware interface, and BUS interface debug logs.
   * ``wpa-supplicant-debug`` - To enable supplicant logs.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Bluetooth® LE
-------------

.. toggle::

   *  To use the Zephyr Bluetooth LE Controller, use the :ref:`bt-ll-sw-split <zephyr:snippet-bt-ll-sw-split>` snippet (see :ref:`app_build_snippets`).

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem (SLM)
----------------------

.. toggle::

   * The :file:`overlay-native_tls.conf` overlay file is no longer supported with the ``thingy91/nrf9160/ns`` board target due to flash memory constraints.
     If you need to use native TLS with Thingy:91, you must disable features from the :file:`prj.conf` and :file:`overlay-native_tls.conf` configuration files to free up flash memory.

Libraries
=========

This section describes the changes related to libraries.

AT command parser
-----------------

.. toggle::

   * The :ref:`at_cmd_parser_readme` library has been deprecated in favor of the :ref:`at_parser_readme` library and will be removed in a future version.

     You can follow this guide to migrate your application to use the :ref:`at_parser_readme` library.
     This will reduce the footprint of the application and will decrease memory requirements on the heap.

     To replace :ref:`at_cmd_parser_readme` with the :ref:`at_parser_readme`, complete the following steps:

     1. Replace the :kconfig:option:`CONFIG_AT_CMD_PARSER` Kconfig option with the :kconfig:option:`CONFIG_AT_PARSER` Kconfig option.

     #. Replace header files:

        * Remove:

          .. code-block:: C

           #include <modem/at_cmd_parser.h>
           #include <modem/at_params.h>

        * Add:

          .. code-block:: C

           #include <modem/at_parser.h>

     #. Replace AT parameter list:

        * Remove:

          .. code-block:: C

           struct at_param_list param_list;

        * Add:

          .. code-block:: C

           struct at_parser parser;

     #. Replace AT parameter list initialization:

        * Remove:

          .. code-block:: C

           /* `param_list` is a pointer to the AT parameter list.
            * `AT_PARAMS_COUNT` is the maximum number of parameters of the list.
            */
           at_params_list_init(&param_list, AT_PARAMS_COUNT);

           /* Other code. */

           /* `at_string` is the AT command string to be parsed.
            * `&remainder` is a pointer to the returned remainder after parsing.
            * `&param_list` is a pointer to the AT parameter list.
            */
           at_parser_params_from_str(at_string, &remainder, &param_list);

        * Add:

          .. code-block:: C

           /* `&at_parser` is a pointer to the AT parser.
            * `at_string` is the AT command string to be parsed.
            */
           at_parser_init(&at_parser, at_string);

          .. note::

             Remember to check the returned error codes from the :ref:`at_parser_readme` functions.
             For the sake of simplicity, they have been omitted in this migration guide.
             Refer to the :ref:`at_parser_readme` documentation for more information on the API and the returned error codes.

     #. Replace integer parameter retrieval:

        * Remove:

          .. code-block:: C

           int value;

           /* `&param_list` is a pointer to the AT parameter list.
            * `index` is the index of the parameter to retrieve.
            * `&value` is a pointer to the output integer variable.
            */
           at_params_int_get(&param_list, index, &value);

           uint16_t value;
           at_params_unsigned_short_get(&param_list, index, &value);

           /* Other variants: */
           at_params_short_get(&param_list, index, &value);
           at_params_unsigned_int_get(&param_list, index, &value);
           at_params_int64_get(&param_list, index, &value);

        * Add:

          .. code-block:: C

           int value;

           /* `&at_parser` is a pointer to the AT parser.
            * `index` is the index of the parameter to retrieve.
            * `&value` is a pointer to the output integer variable.
            *
            * Note: this function is type-generic on the type of the output integer variable.
            */
           err = at_parser_num_get(&at_parser, index, &value);

           uint16_t value;
           /* Note: this function is type-generic on the type of the output integer variable. */
           err = at_parser_num_get(&at_parser, index, &value);

     #. Replace string parameter retrieval:

        * Remove:

          .. code-block:: C

           /* `&param_list` is a pointer to the AT parameter list.
            * `index` is the index of the parameter to retrieve.
            * `value` is the output buffer where the string is copied into.
            * `&len` is a pointer to the length of the copied string.
            *
            * Note: the copied string is not null-terminated.
            */
           at_params_string_get(&param_list, index, value, &len);

           /* Null-terminate the string. */
           value[len] = '\0';

        * Add:

          .. code-block:: C

           /* `&at_parser` is a pointer to the AT parser.
            * `index` is the index of the parameter to retrieve.
            * `value` is the output buffer where the string is copied into.
            * `&len` is a pointer to the length of the copied string.
            *
            * Note: the copied string is null-terminated.
            */
           at_parser_string_get(&at_parser, index, value, &len);

     #. Replace parameter count retrieval:

        * Remove:

          .. code-block:: C

           /* `&param_list` is a pointer to the AT parameter list.
            * `count` is the returned parameter count.
            */
           uint32_t count = at_params_valid_count_get(&param_list);

        * Add:

          .. code-block:: C

           size_t count;

           /* `&at_parser` is a pointer to the AT parser.
            * `&count` is a pointer to the returned parameter count.
            */
           at_parser_cmd_count_get(&at_parser, &count);

     #. Replace command type retrieval:

        * Remove:

          .. code-block:: C

           /* `at_string` is the AT string that we want to retrieve the command type of.
            */
           enum at_cmd_type type = at_parser_at_cmd_type_get(at_string);

        * Add:

          .. code-block:: C

           enum at_parser_cmd_type type;

           /* `&at_parser` is a pointer to the AT parser.
            * `&type` pointer to the returned command type.
            */
           at_parser_cmd_type_get(&at_parser, &type);

LTE link control library
------------------------

.. toggle::

   * For applications using :ref:`lte_lc_readme`:

     * Replace the use of the :c:func:`lte_lc_factory_reset` function with the following:

      * If the :c:enumerator:`LTE_LC_FACTORY_RESET_ALL` value is used with the :c:func:`lte_lc_factory_reset` function:

         .. code-block:: C

            #include <nrf_modem_at.h>

            err = nrf_modem_at_printf("AT%%XFACTORYRESET=0");

      * If the :c:enumerator:`LTE_LC_FACTORY_RESET_USER` value is used with the :c:func:`lte_lc_factory_reset` function:

         .. code-block:: C

            #include <nrf_modem_at.h>

            err = nrf_modem_at_printf("AT%%XFACTORYRESET=1");

     * Replace the use of the :c:func:`lte_lc_reduced_mobility_get` function with the following:

      .. code-block:: C

         #include <nrf_modem_at.h>

         uint16_t mode;

         ret = nrf_modem_at_scanf("AT%REDMOB?", "%%REDMOB: %hu", &mode);
         if (ret != 1) {
            /* Handle failure. */
         } else {
            /* Handle success. */
         }

     * Replace the use of the :c:func:`lte_lc_reduced_mobility_set` function with the following:

      * If the :c:enumerator:`LTE_LC_REDUCED_MOBILITY_DEFAULT` value is used with the :c:func:`lte_lc_reduced_mobility_set` function:

         .. code-block:: C

            #include <nrf_modem_at.h>

            err = nrf_modem_at_printf("AT%%REDMOB=0");

      * If the :c:enumerator:`LTE_LC_REDUCED_MOBILITY_NORDIC` value is used with the :c:func:`lte_lc_reduced_mobility_set` function:

         .. code-block:: C

            #include <nrf_modem_at.h>

            err = nrf_modem_at_printf("AT%%REDMOB=1");

      * If the :c:enumerator:`LTE_LC_REDUCED_MOBILITY_DISABLED` value is used with the :c:func:`lte_lc_reduced_mobility_set` function:

         .. code-block:: C

            #include <nrf_modem_at.h>

            err = nrf_modem_at_printf("AT%%REDMOB=2");
