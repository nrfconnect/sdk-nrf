.. _migration_3.0:

Migration guide for |NCS| v3.0.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.9.0 to |NCS| v3.0.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.0_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

.. _asset_tracker_v2:

Asset Tracker v2
----------------

.. toggle::

   * The Asset Tracker v2 application has been removed.
     For development of asset tracking applications, refer to the `Asset Tracker Template <Asset Tracker Template_>`_.

     The factory-programmed Asset Tracker v2 firmware is still available to program the nRF91 Series devices using the `Programmer app`_, the `Quick Start app`_, and the `Cellular Monitor app`_.

nRF5340 Audio applications
--------------------------

.. toggle::

   * The :ref:`nrf53_audio_app` :ref:`nrf53_audio_app_building_script` now requires the transport (``-t/--transport``) type to be included.
   * The :ref:`nrf53_audio_app` :ref:`nrf53_audio_app_building_standard` now requires an extra :ref:`CMake option to provide extra Kconfig fragments <cmake_options>` to select the device type.

Libraries
=========

This section describes the changes related to libraries.

Google Fast Pair
----------------

.. toggle::

   For applications and samples using the :ref:`bt_fast_pair_readme` library:

   * If you use sysbuild for generating a hex file with the Fast Pair provisioning data, you must align your application with the new approach for passing the provisioning parameters (the Model ID and the Anti-Spoofing Private Key).
     The ``FP_MODEL_ID`` and ``FP_ANTI_SPOOFING_KEY`` CMake variables are replaced by the corresponding ``SB_CONFIG_BT_FAST_PAIR_MODEL_ID`` and ``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY`` Kconfig options that are defined at the sysbuild level.
     The following additional build parameters for Fast Pair are no longer valid:

     ``-DFP_MODEL_ID=0xFFFFFF -DFP_ANTI_SPOOFING_KEY=AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=``

     You must replace them with the new sysbuild Kconfig options.
     You can provide them as additional build parameters to the build command as follows:

     .. tabs::

        .. tab:: Windows

           ``-DSB_CONFIG_BT_FAST_PAIR_MODEL_ID=0xFFFFFF -DSB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY='\"AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=\"'``

        .. tab:: Linux

           ``-DSB_CONFIG_BT_FAST_PAIR_MODEL_ID=0xFFFFFF -DSB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY=\"AbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbAbA=\"``

     You can replace this exemplary method with any other configuration method that is supported by sysbuild.

     .. note::
        To avoid build failures, you must surround the string value for the Anti-Spoofing Private Key parameter with the special character sequence instead of the typical ``"`` character.
        The surrounding characters depend on your operating system:

        .. tabs::

           .. tab:: Windows

              1. Replace the standard ``"`` character with the ``\"`` characters.
              #. Surround the modified string value with the ``'`` character.

           .. tab:: Linux

              Replace the standard ``"`` character with the ``\"`` characters.

        The special character sequence is only required when you pass the ``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY`` Kconfig option as an additional build parameter.

   * You must remove the ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option from the sysbuild configuration in your project.
     The ``SB_CONFIG_BT_FAST_PAIR`` option no longer exists in this |NCS| release.
     Additionally, if you rely on the ``SB_CONFIG_BT_FAST_PAIR`` Kconfig option to set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option in the main image configuration of your application, you must align your main image configuration and set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option explicitly.

   * If your Fast Pair application uses the Find My Device (FMD) extension, you must update your application code to correctly track the FMDN provisioning state.
     From this |NCS| release, you must not rely on the :c:member:`bt_fast_pair_fmdn_info_cb.provisioning_state_changed` callback to report the initial provisioning state right after the Fast Pair module is enabled with the :c:func:`bt_fast_pair_enable` function call.
     Instead, you must use the :c:func:`bt_fast_pair_fmdn_is_provisioned` function to initialize the FMDN provisioning state right after the :c:func:`bt_fast_pair_enable` function call.
     For more details, see the :ref:`ug_bt_fast_pair_gatt_service_fmdn_info_callbacks_provisioning_state` section in the Fast Pair integration guide.

nRF Cloud library
-----------------

.. toggle::

   For applications and samples using the :ref:`lib_nrf_cloud` library:

   * You must set the :kconfig:option:`CONFIG_NRF_CLOUD` Kconfig option to access the nRF Cloud libraries.
     This option is now disabled by default to prevent the unintended inclusion of nRF Cloud Kconfig variables in non-nRF Cloud projects, addressing a previous issue.

Location library
----------------

.. toggle::

   For applications and samples using the :ref:`lib_location` library:

   * Support for HERE location services and the :kconfig:option:`CONFIG_LOCATION_SERVICE_HERE` Kconfig option has been removed.
     To use external location services, enable the :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` option and implement the required APIs.

   * The ``service`` parameter in :c:struct:`location_cellular_config` and :c:struct:`location_wifi_config` has been removed.
     The library supports only one location service, so the ``service`` parameter is no longer needed.

.. _migration_3.0_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

.. _requirements_clt:

IDE, OS, and tool support
=========================

.. toggle::

   |nrf_CLT_deprecation_note|

   It is recommended to install the latest version of `nRF Util`_, as listed in the :ref:`installing_vsc` section of the installation page.

.. _gs_app_tcm:
.. _gs_assistant:
.. _auto_installation_tcm_setup:
.. _toolchain_update:

Installation of the SDK and toolchain
-------------------------------------

.. toggle::

   The Toolchain Manager app has been deprecated: starting from the |NCS| v3.0.0, it no longer provides the latest toolchain and |NCS| versions for installation.

   Use one of the two :ref:`installation methods <install_ncs>` to manage the toolchain and SDK versions, either the recommended |nRFVSC| extension or the command line with nRF Util.

Build system
============

.. toggle::

   * The default runner for the ``west flash`` command has been changed to use `nRF Util`_ instead of ``nrfjprog`` that is part of the archived `nRF Command Line Tools`_.
     This affects all :ref:`boards <app_boards>` that used ``nrfjprog`` as the west runner backend for programming the following SoCs and SiPs:

     * nRF91 Series (including nRF91x1)
     * nRF7002
     * nRF5340 (including nRF5340 Audio DK)
     * Nordic Thingy:53
     * nRF52 Series
     * nRF21540

     This change is made to ensure that the programming process is consistent across all boards and to provide a more robust programming experience.
     The ``west flash`` command now uses the ``nrfutil device`` subcommand by default to flash the application to the board.

     It is recommended to install nRF Util to avoid potential issues during programming.
     Complete the following steps:

     1. Follow the steps for `Installing nRF Util`_ in its official documentation.
     2. Install the `nrfutil device <Device command overview_>`_ using the following command:

        .. code-block::

           nrfutil install device

     If you prefer to continue using ``nrfjprog`` for programming devices, :ref:`specify the west runner <programming_selecting_runner>` with ``west flash``.

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem
----------------

.. toggle::

   The error event ``LWM2M_CARRIER_ERROR_RUN`` has been removed from the :ref:`SLM_AT_CARRIER`.

   * Errors that were previously notified to the application with the ``LWM2M_CARRIER_ERROR_RUN`` event type have instead been added to :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION`.

Libraries
=========

This section describes the changes related to libraries.

Download client
---------------

.. toggle::

   * The :ref:`lib_download_client` library has been deprecated in favor of the :ref:`lib_downloader` library and will be removed in a future |NCS| release.

     You can follow this guide to migrate your application to use the :ref:`lib_downloader` library.
     This will reduce the footprint of the application and will decrease memory requirements on the heap.

     To replace :ref:`lib_download_client` with the :ref:`lib_downloader`, complete the following steps.

     1. Kconfig options:

         * Replace:

            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_STACK_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_STACK_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_SHELL` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_SHELL` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_HTTP_TIMEO_MS` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_COAP_MAX_RETRANSMIT_REQUEST_COUNT` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_MAX_RETRANSMIT_REQUEST_COUNT` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_BLOCK_SIZE_512` Kconfig option.

         * Remove:

            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_BUF_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS` Kconfig option.
            * The :kconfig:option:`CONFIG_DOWNLOAD_CLIENT_CID` Kconfig option.

         * Add:

            * The :kconfig:option:`CONFIG_DOWNLOADER_TRANSPORT_COAP` Kconfig option to enable CoAP support.
            * The :kconfig:option:`CONFIG_NET_IPV4` Kconfig option to enable IPv4 support.
            * The :kconfig:option:`CONFIG_NET_IPV6` Kconfig option to enable IPv6 support.

     #. Replace header files:

        * Remove:

          .. code-block:: C

             #include <net/download_client.h>

        * Add:

          .. code-block:: C

             #include <net/downloader.h>

     #. Replace download client initialization:

        * Remove:

          .. code-block:: C

              static struct download_client dlc;
              static int callback(const struct download_client_evt *event);

              download_client_init(&dlc, callback)

        * Add:

         .. code-block:: C

            static struct downloader dl;
            static int callback(const struct downloader_evt *event);
            static char dl_buf[2048]; /* Use buffer size set by CONFIG_DOWNLOAD_CLIENT_BUF_SIZE previously */
            static struct downloader_cfg dl_cfg = {
               .callback = callback,
               .buf = dl_buf,
               .buf_size = sizeof(dl_buf),
            };

            downloader_init(&dl, &dl_cfg);

     #. Update download client callback:

        * Replace:

            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_FRAGMENT` event with :c:enumerator:`DOWNLOADER_EVT_FRAGMENT`.
            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_ERROR` event with :c:enumerator:`DOWNLOADER_EVT_ERROR`.
            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_DONE` event with :c:enumerator:`DOWNLOADER_EVT_DONE`.

        * Remove:

            * :c:enumerator:`DOWNLOAD_CLIENT_EVT_CLOSED` event.

        * Add:

            * :c:enumerator:`DOWNLOADER_EVT_STOPPED` event.
            * :c:enumerator:`DOWNLOADER_EVT_DEINITIALIZED` event.

     #. Server connect and disconnect:

        * The :c:func:`download_client_disconnect` function is not ported to the new downloader.
          The downloader is expected to connect when the download begins.
          If the ``keep_connection`` flag is set in the host configuration the connection persists after the download completes or is aborted by the :c:func:`downloader_cancel` function.
          In this case, the downloader is disconnected when it is deinitialized by the :c:func:`downloader_deinit` function.


     #. Replace file download:

        We show the changes for the :c:func:`download_client_start` function here, though the required work is
        similar to the :c:func:`download_client_get` function.

        * Remove:

        .. code-block:: C

           int err;
           const struct download_client_cfg dlc_config = {
              ...
           };

           err = download_client_set_host(&dlc, dl_host, &dlc_config);

           err = download_client_start(&dlc, dl_file, offset);

        * Add:

        .. code-block:: C

           /* Note: All configuration of the downloader is done through the config structs.
            * The downloader struct should not be modified by the application.
            */

           static struct downloader_host_cfg dl_host_cfg = {
                   ...
                   /* Note:
                    * .frag_size_override is replaced by .range_override.
                    * .set_tls_hostname is replaced by .set_native_tls.
                    * dlc.close_when_done is moved here and inverted(.keep_connection).
                    * Set .cid if CONFIG_DOWNLOAD_CLIENT_CID was enabled in the download client.
                    */
           };

           int err = downloader_get_with_host_and_file(&dl, &dl_host_cfg, dl_host, dl_file, offset);

        .. note::
           The new downloader has an API to download the file using the URI directly.

     #. [optional] Deinitialize the downloader after use:

        The new downloader can be deinitialized to free its resources.
        If another download is required later on, a new downloader instance needs to be initialized.

        * Add:

        .. code-block:: C

           err = downloader_deinit(&dl);

Modem SLM
---------

.. toggle::

   For applications and samples using the :ref:`lib_modem_slm` library:

    * Replace the ``CONFIG_MODEM_SLM_WAKEUP_PIN`` Kconfig option with :kconfig:option:`CONFIG_MODEM_SLM_POWER_PIN`.
    * Replace the ``CONFIG_MODEM_SLM_WAKEUP_TIME`` Kconfig option with :kconfig:option:`CONFIG_MODEM_SLM_POWER_PIN_TIME`.
    * Replace the :c:func:`modem_slm_wake_up` function with :c:func:`modem_slm_power_pin_toggle`.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.

Bluetooth Mesh
--------------

.. toggle::

   * Support for Tinycrypt-based security toolbox (:kconfig:option:`CONFIG_BT_MESH_USES_TINYCRYPT`) has started the deprecation procedure and is not recommended for future designs.
   * For platforms that do not support the TF-M: The default security toolbox is based on the Mbed TLS PSA API (:kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA`).
   * For platforms that support the TF-M: The default security toolbox is based on the TF-M PSA API (:kconfig:option:`CONFIG_BT_MESH_USES_TFM_PSA`).

The :ref:`ug_bt_mesh_configuring` page provides more information about the updating of the images based on different security toolboxes.
