:orphan:

.. _migration_2.8:

Migration guide for |NCS| v2.8.0 (Working draft)
################################################

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

SUIT DFU for nRF54H20
---------------------

.. toggle::

  * The manifest sequence number is no longer configured through a :ref:`sysbuild <configuring_sysbuild>` Kconfig option.
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

    For the list of all variables, set through the :file:`VERSION`, refer to the :ref:`ug_nrf54h20_suit_customize_dfu`.

Nordic Secure Immutable Bootloader (NSIB, B0, or B0n)
-----------------------------------------------------

.. toggle::

   Custom printing has been dropped in favor of using the logging subsystem, with output printed out to the default logging device.
   The ``CONFIG_SECURE_BOOT_DEBUG`` Kconfig option has been removed.
   To disable logging in B0 or B0n, set the :kconfig:option:`CONFIG_LOG` option to ``n``.
   To send logs over RTT instead of UART, apply the following settings:

       * Enable the :kconfig:option:`CONFIG_USE_SEGGER_RTT` and :kconfig:option:`CONFIG_RTT_CONSOLE` Kconfig options.
       * Disable the :kconfig:option:`CONFIG_UART_CONSOLE` and :kconfig:option:`CONFIG_SERIAL` Kconfig options.

nRF70 Series
------------

.. toggle::

   * The nRF70 Series support is now part of Zephyr upstream and it requires the following changes:

    * The nRF70 Series driver namespace has been renamed from ``NRF700X`` to ``NRF70``.
      For example, ``CONFIG_NRF700X_RAW_DATA_RX`` to ``CONIFG_NRF70_RAW_DATA_RX``.
      Update your application configurations to use the new namespace.
    * The nRF70 Series driver now uses per-module kernel heap with a higher default.
      If a sample or an application uses the kernel heap but uses less than the default size, a build warning is displayed.
      Use the :kconfig:option:`CONFIG_HEAP_MEM_POOL_IGNORE_MIN` Kconfig option and enable it to suppress the warning.

   * The WPA supplicant is also now part of Zephy upstream and it requires the following changes:

    * The WPA supplicant namespace has been renamed from ``WPA_SUPP`` to ``WIFI_NM_WPA_SUPPLICANT``.
      For example, ``CONFIG_WPA_SUPP=y`` to ``CONFIG_WIFI_NM_WPA_SUPPLICANT=y``.
      Update your application configurations to use the new namespace.

   * The SR co-existence feature should now be explicitly enabled using the :kconfig:option:`CONFIG_NRF70_SR_COEX` Kconfig option.
     The RF switch feature should be enabled using the :kconfig:option:`CONFIG_NRF70_SR_COEX_RF_SWITCH` Kconfig option.

Libraries
=========

This section describes the changes related to libraries.

Wi-Fi®
------

.. toggle::

   * For :ref:`lib_wifi_credentials` library:

     * Syntax for ``add`` command has been modified to support ``getopt`` model.
         For example, the following command with old syntax:
         ``wifi_cred add SSID WPA2-PSK password`` should be replaced with the following command with new syntax:
         ``wifi_cred add -s SSID -k 1 -p password``.
         ``wifi_cred add --help`` command will provide more information on the new syntax.

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

.. _migration_2.8_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem (SLM)
----------------------

.. toggle::

   The :file:`overlay-native_tls.conf` overlay file is no longer supported with the ``thingy91/nrf9160/ns`` board target due to flash memory constraints.
   If you need to use native TLS with Thingy:91, you must disable features from the :file:`prj.conf` and :file:`overlay-native_tls.conf` configuration files to free up flash memory.

Devicetree
----------

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
