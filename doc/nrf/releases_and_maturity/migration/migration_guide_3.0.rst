:orphan:

.. _migration_3.0:

Migration guide for |NCS| v3.0.0
################################

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

   Starting from the |NCS| v3.0.0, the Toolchain Manager app no longer provides the latest toolchain and |NCS| versions for installation.

   Use one of the two :ref:`installation methods <install_ncs>` to manage the toolchain and SDK versions, either the recommended |nRFVSC| or the command line with nRF Util.

.. _required_build_system_mig_300:

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

     .. note::

        For |NCS| 3.0.0, use the nrfutil-device v2.8.8.

     It is recommended to install nRF Util to avoid potential issues during programming.
     Complete the following steps:

     1. Follow the steps for `Installing nRF Util`_ in its official documentation.
     2. Install the `nrfutil device <Device command overview_>`_ using the following command:

        .. code-block::

           nrfutil install device=2.8.8 --force

     If you prefer to continue using ``nrfjprog`` for programming devices, :ref:`specify the west runner <programming_selecting_runner>` with ``west flash``.

   * Erasing the external memory when programming a new firmware image with the ``west flash`` series now always correctly honors the ``--erase`` flag (and its absence) both when using the ``nrfjprog`` and ``nrfutil`` backends.
     Before this release, the ``nrjfprog`` backend would always erase only the sectors of the external flash used by the new firmware, and the ``nrfutil`` backend would always erase the whole external flash.

Application development
=======================

The following are the changes required to migrate your applications to the |NCS| 3.0.0.

ZMS settings backend
--------------------

.. toggle::

   The new settings backend for ZMS is not compatible with the old version.

   To keep using the legacy backend, enable the :kconfig:option:`CONFIG_SETTINGS_ZMS_LEGACY` Kconfig option.

   To migrate from the legacy backend to the new backend remove the Kconfig options :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE`
   and :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE` from your conf files.

Bluetooth® LE legacy pairing
----------------------------

.. toggle::

   Support for Bluetooth LE legacy pairing is no longer enabled by default because it is not secure.
   The :kconfig:option:`CONFIG_BT_SMP_SC_PAIR_ONLY` Kconfig option is enabled by default in Zephyr.
   If you still need to support the Bluetooth LE legacy pairing and you accept the security risks, disable the option in the configuration.

   .. caution::
      Using Bluetooth LE legacy pairing introduces, among others, a risk of passive eavesdropping.
      Supporting Bluetooth LE legacy pairing makes devices vulnerable to downgrade attacks.

CRACEN initialization
---------------------

.. toggle::

   In the |NCS| versions 2.8.0 and 2.9.0, you must explicitly configure the CRACEN initialization.
   It is done by adding the :kconfig:option:`CONFIG_CRACEN_LOAD_MICROCODE` Kconfig option to the image configuration.
   This option allows to select the given image to initialize CRACEN.

   However, from |NCS| 3.0.0, CRACEN is automatically initialized.
   The new build configuration option (:kconfig:option:`SB_CONFIG_CRACEN_MICROCODE_LOAD_ONCE`) now controls this process at the sysbuild level.
   When enabled, the build system automatically determines which image must handle the initialization of CRACEN.

   Unlike in the |NCS| versions 2.8.0 and 2.9.0, where CRACEN initialization is disabled by default in the MCUboot configuration, CRACEN is initialized by the earliest bootloader by default in the |NCS| 3.0.0.
   This change can lead to scenarios where CRACEN might be initialized twice or not initialized at all.
   When migrating from the |NCS| v2.9.0 to v3.0.0, you must analyze which image is responsible for initializing CRACEN before and after the firmware update to ensure correct operation.
   Make sure to adjust your bootloader or application upgrade path accordingly to avoid any issues related to CRACEN initialization.

nRF54H20
========

This section describes the changes specific to the nRF54H20 SoC and DK support in the |NCS|.

Dependencies
------------

The following required dependencies for the nRF54H20 SoC and DK have been updated.

nRF Util
++++++++

.. toggle::

   * ``nrfutil`` has been updated to v7.13.0.

     Install nRF Util v7.13.0 as follows:

      1. Download the nRF Util executable file from the `nRF Util development tool`_ product page.
      #. Add nRF Util to the system path on Linux and macOS, or environment variables on Windows, to run it from anywhere on the system.
         On Linux and macOS, use one of the following options:

         * Add nRF Util's directory to the system path.
         * Move the file to a directory in the system path.

      #. On macOS and Linux, give ``nrfutil`` execute permissions by typing ``chmod +x nrfutil`` in a terminal or using a file browser.
         This is typically a checkbox found under file properties.
      #. On macOS, to run the nRF Util executable, you need to allow it in the system settings.
      #. Verify the version of the nRF Util installation on your machine by running the following command:

         .. code-block::

            nrfutil --version

      #. If your version is lower than 7.13.0, run the following command to update nRF Util:

         .. code-block::

            nrfutil self-upgrade

     For more information, see the `nRF Util`_ documentation.

nRF Util device
+++++++++++++++

.. toggle::

  * nRF Util ``device`` command has been updated to v2.8.8.

    Install the nRF Util ``device`` command v2.8.8 as follows:

    .. code-block::

       nrfutil install device=2.8.8 --force

    For more information, consult the `nRF Util`_ documentation.

nRF Util trace
++++++++++++++

.. toggle::

  * nRF Util ``trace`` command has been updated to v3.3.0.
    Install the nRF Util ``trace`` command v3.3.0 as follows:

    .. code-block::

       nrfutil install trace=3.3.0 --force

    For more information, consult the `nRF Util`_ documentation.

nRF Util suit
+++++++++++++

.. toggle::

   * nRF Util ``suit`` command has been updated to v0.9.0.
     Install the nRF Util ``suit`` command v0.9.0 as follows:

     .. code-block::

        nrfutil install suit=0.9.0 --force

     For more information, consult the `nRF Util`_ documentation.

nRF54H20 BICR
+++++++++++++

.. toggle::

   * The nRF54H20 BICR has been updated (from the one supporting |NCS| v2.9.0 as well as |NCS| v2.9.0-nRF54H20-1).
     To update the BICR of your development kit while in Root of Trust, do the following:

     1. Build your application using |NCS| v3.0.0.
     #. Connect the nRF54H20 DK to your computer using the **DEBUGGER** port on the DK.

        .. note::

           On MacOS, connecting the DK might repeatedly trigger a popup displaying the message ``Disk Not Ejected Properly``.
           To disable this, run ``JLinkExe``, then run ``MSDDisable`` in the J-Link Commander interface.

     #. List all the connected development kits to see their serial number (matching the one on the DK's sticker):

        .. code-block::

           nrfutil device list

     #. Program the BICR by running nRF Util from your application folder using the following command:

        .. code-block::

           nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware ./build/<your_application_name>/zephyr/bicr.hex --core Application --serial-number <serial_number>

nRF54H20 SoC binaries
+++++++++++++++++++++

.. toggle::

   * The *nRF54H20 SoC binaries* bundle has been updated to version 0.9.6.

     .. caution::
        If migrating from |NCS| v2.9.0 or lower, you must follow steps from :ref:`migration_2.9.0-nRF54H20-1` to update the *nRF54H20 SoC binaries* bundle to version 0.9.2.

     .. note::
        The nRF54H20 SoC binaries only support specific versions of the |NCS| and do not support rollbacks to a previous version.
        Upgrading the nRF54H20 SoC binaries on your development kit might break the DK's compatibility with applications developed for previous versions of the |NCS|.
        For more information, see :ref:`abi_compatibility`.

     To update the SoC binaries bundle of your development kit while in Root of Trust, do the following:

     1. Download the `nRF54H20 SoC binaries v0.9.6`_.

        .. note::
           On macOS, ensure that the ZIP file is not unpacked automatically upon download.

     #. Purge the device as follows:

        .. code-block::

           nrfutil device recover --core Application --serial-number <serial_number>
           nrfutil device recover --core Network --serial-number <serial_number>
           nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

     #. Run ``west update``.
     #. Move the correct :file:`.zip` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command:

        .. code-block::

           nrfutil device x-suit-dfu --firmware nrf54h20_soc_binaries_v0.9.6.zip --serial-number <serial_number>

     #. Purge the device again as follows:

        .. code-block::

           nrfutil device recover --core Application --serial-number <serial_number>
           nrfutil device recover --core Network --serial-number <serial_number>
           nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

SDK and toolchain
+++++++++++++++++

.. toggle::

   * To update the SDK and the toolchain, do the following:

     1. Open Toolchain Manager in nRF Connect for Desktop.
     #. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
     #. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version |release|.

Application development
-----------------------

The following are the changes required to migrate your applications to the |NCS| 3.0.0.

Entropy source for radio applications
+++++++++++++++++++++++++++++++++++++

.. toggle::

   * The default entropy source was changed to use the SSF service.
     As a result, the communication channel as well as RAM regions, dedicated to communicate with the SDFW are now enabled by default.
     This can result in incompatible UICRs if your application relies on the defaults.
     If UICRs are incompatible, the application cannot be upgraded using DFU, but must be programmed using the **DEBUGGER** port.
     If you want to update your application using DFU, add the following overlay to your radio application if you want to maintain UICR compatibility:

     .. code-block:: dts

        /* Switch back to the pseudo-random entropy source. */
        / {
           chosen {
             zephyr,entropy = &prng;
           };
           /delete-node/ psa-rng;
           prng: prng {
              compatible = "nordic,entropy-prng";
              status = "okay";
           };
        };
        /* Disable IPC between cpusec <-> cpurad. */
        &cpusec_cpurad_ipc {
           status = "disabled";
        };
        &cpurad_ram0x_region {
           status = "disabled";
        };
        &cpusec_bellboard {
           status = "disabled";
        };

SUIT MPI configuration
++++++++++++++++++++++

.. toggle::

   The SUIT MPI configuration has been moved from local Kconfig options to sysbuild.
   To migrate your application, move all ``CONFIG_MPI_*`` options from the application configuration into the :file:`sysbuild.conf` file.
   For example, to migrate the root manifest vendor ID, remove the following line from the :file:`prj.conf` file:

   .. code-block:: kconfig

      CONFIG_SUIT_MPI_ROOT_VENDOR_NAME="acme.corp"

   And add the following line inside the :file:`sysbuild.conf` file:

   .. code-block:: kconfig

      SB_CONFIG_SUIT_MPI_ROOT_VENDOR_NAME="acme.corp"

   If your project does not use the :file:`sysbuild.conf` file, you must create one.

Samples and applications
========================

This section describes the changes related to samples and applications.

.. _asset_tracker_v2:

Asset Tracker v2
----------------

.. toggle::

   * The Asset Tracker v2 application has been removed.
     For development of asset tracking applications, refer to the `Asset Tracker Template <Asset Tracker Template_>`_.

     The factory-programmed Asset Tracker v2 firmware is still available to program the nRF91xx DKs using the `Programmer app`_, the `Quick Start app`_, and the `Cellular Monitor app`_.

nRF Desktop
-----------

.. toggle::

   * The default device names (the :ref:`CONFIG_DESKTOP_DEVICE_PRODUCT <config_desktop_app_options>` Kconfig option) have been updated to remove the "52" infix, because the nRF Desktop application supports other SoC Series also.
     As a result of this change, peripherals using firmware from |NCS| 3.0.0 (or newer) will not pair with dongles using firmware from an older |NCS| release, and the other way around.
     Also aligned the :file:`99-hid.rules` file inside the HID Configurator script.
     The HID Configurator rule will not work with old device names.

     To keep backwards compatibility, revert locally, the changes introduced by commit hash ``5b80e46478462907a3cc4fd1686e241591775ffe``:

     * The :ref:`CONFIG_DESKTOP_DEVICE_PRODUCT <config_desktop_app_options>` Kconfig option defines the device name used by HID peripheral.
     * The ``peer_name`` array inside the :file:`ble_scan_def.h` file determines device name filters used by HID dongle while scanning for unpaired HID peripherals.
     * The :file:`99-hid.rules` file allows HID configurator Python script to configure nRF Desktop devices without root access.

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
     The ``FP_MODEL_ID`` and ``FP_ANTI_SPOOFING_KEY`` CMake variables are replaced by the corresponding :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_MODEL_ID` and :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY` Kconfig options that are defined at the sysbuild level.
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

        The special character sequence is only required when you pass the :kconfig:option:`SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY` Kconfig option as an additional build parameter.

   * You must remove the :kconfig:option:`SB_CONFIG_BT_FAST_PAIR` Kconfig option from the sysbuild configuration in your project.
     The :kconfig:option:`SB_CONFIG_BT_FAST_PAIR` option no longer exists in this |NCS| release.
     Additionally, if you rely on the :kconfig:option:`SB_CONFIG_BT_FAST_PAIR` Kconfig option to set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option in the main image configuration of your application, you must align your main image configuration and set the :kconfig:option:`CONFIG_BT_FAST_PAIR` Kconfig option explicitly.

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

Application development
=======================

The following are the changes recommended to migrate your applications to the |NCS| 3.0.0.

Performance optimization for ZMS settings backend
-------------------------------------------------

.. toggle::

   For the new backend you can now enable some performance optimizations using the following Kconfig options:

   * :kconfig:option:`CONFIG_SETTINGS_ZMS_LL_CACHE`: Used for caching the linked list nodes related to Settings Key/Value entries.
   * :kconfig:option:`CONFIG_SETTINGS_ZMS_LL_CACHE_SIZE`: The size of the linked list cache (each entry occupies 8B of RAM).
   * :kconfig:option:`CONFIG_SETTINGS_ZMS_NO_LL_DELETE`: Disables deleting the linked list nodes when deleting a Settings Key.
     Use this option only when the application is always using the same Settings Keys.
     When the application uses random Keys, enabling this option could lead to incrementing the linked list nodes without corresponding Keys and cause excessive delays to loading of the Keys.
     Use this option only to accelerate the delete operation for a fixed set of Settings elements.
   * :kconfig:option:`CONFIG_SETTINGS_ZMS_LOAD_SUBTREE_PATH`: Loads first the subtree path passed in the argument, then continue to load all the Keys in the same subtree if the handler returns a zero value.

Samples and applications
========================

This section describes the changes related to samples and applications.

Serial LTE Modem
----------------

.. toggle::

   The error event ``LWM2M_CARRIER_ERROR_RUN`` has been removed from the :ref:`SLM_AT_CARRIER`.

   * Errors that were previously notified to the application with the ``LWM2M_CARRIER_ERROR_RUN`` event type have instead been added to :c:macro:`LWM2M_CARRIER_ERROR_CONFIGURATION`.

Bluetooth Fast Pair Locator tag
-------------------------------

.. toggle::

   * If you want to align your application project with the newest version of the :ref:`fast_pair_locator_tag` sample and still maintain the DFU backwards compatibility for your already deployed products that are based on the ``nrf52840dk/nrf52840``  and the ``nrf54l15dk/nrf54l15/cpuapp`` board targets, use the RSA signature algorithm (the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA` Kconfig option) that is supported as part of the previous |NCS| releases.
     In the current |NCS| release, the MCUboot DFU signature type has been changed:

     * To the Elliptic curve digital signatures with curve P-256 (ECDSA P256 - the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256` Kconfig option) in case of the ``nrf52840dk/nrf52840`` board target.
     * To the Edwards-curve digital signature with curve Curve25519 (ED25519 - the :kconfig:option:`SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519` Kconfig option) in case of the ``nrf54l15dk/nrf54l15/cpuapp`` board target.

     As a result, you will not be able to perform DFU from an old version to a new one.

Wi-Fi®
------

.. toggle::

   * For samples using Wi-Fi features:

     * The nRF70 driver heap was part of the system shared heap :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`.
       Now dedicated heaps have been defined for Wi-Fi driver control plane and data plane operations.
       Default value of heap for control plane operations is 20000 bytes and for data plane operations, it is 130000.
       It is recommended to disable :kconfig:option:`CONFIG_HEAP_MEM_POOL_IGNORE_MIN`, and let system calculate the ``K_HEAP`` size.
       Any subsequent RAM overflow issues need to be solved by fine-tuning :kconfig:option:`CONFIG_NRF_WIFI_CTRL_HEAP_SIZE` and :kconfig:option:`CONFIG_NRF_WIFI_DATA_HEAP_SIZE`.


Libraries
=========

This section describes the changes related to libraries.

Download client
---------------

.. toggle::

   * The Download client library has been deprecated in favor of the :ref:`lib_downloader` library and will be removed in a future |NCS| release.

     You can follow this guide to migrate your application to use the :ref:`lib_downloader` library.
     This will reduce the footprint of the application and will decrease memory requirements on the heap.

     To replace Download client with the :ref:`lib_downloader`, complete the following steps.

     1. Kconfig options:

         * Replace:

            * The ``CONFIG_DOWNLOAD_CLIENT`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_MAX_FILENAME_SIZE` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_STACK_SIZE`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_STACK_SIZE` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_SHELL`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_SHELL` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_HTTP_TIMEO_MS` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_COAP_MAX_RETRANSMIT_REQUEST_COUNT`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_MAX_RETRANSMIT_REQUEST_COUNT` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_COAP_BLOCK_SIZE`` Kconfig option with the :kconfig:option:`CONFIG_DOWNLOADER_COAP_BLOCK_SIZE_512` Kconfig option.

         * Remove:

            * The ``CONFIG_DOWNLOAD_CLIENT_BUF_SIZE`` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE`` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_RANGE_REQUESTS`` Kconfig option.
            * The ``CONFIG_DOWNLOAD_CLIENT_CID`` Kconfig option.

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

This section describes the changes related to protocols.

Bluetooth Mesh
--------------

.. toggle::

   * Support for Tinycrypt-based security toolbox (:kconfig:option:`CONFIG_BT_MESH_USES_TINYCRYPT`) has started the deprecation procedure and is not recommended for future designs.
   * For platforms that do not support the TF-M: The default security toolbox is based on the Mbed TLS PSA API (:kconfig:option:`CONFIG_BT_MESH_USES_MBEDTLS_PSA`).
   * For platforms that support the TF-M: The default security toolbox is based on the TF-M PSA API (:kconfig:option:`CONFIG_BT_MESH_USES_TFM_PSA`).

     The :ref:`ug_bt_mesh_configuring` page provides more information about the updating of the images based on different security toolboxes.
   * Due to an incompatibility between the old and new ZMS backend for Settings, the mesh device will not be able to load its settings and provisioning data.
     This affects nRF54L Series devices.

     Make sure to unprovision mesh device before flashing the new firmware.

     Alternatively, enable the :kconfig:option:`CONFIG_SETTINGS_ZMS_LEGACY` Kconfig option to use the old backend and recover the device settings and provisioning data.
     Enable :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE` and adjust :kconfig:option:`CONFIG_SETTINGS_ZMS_NAME_CACHE_SIZE` according to the device needs.
