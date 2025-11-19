.. _ncs_release_notes_310_preview3:

Changelog for |NCS| v3.1.0-preview3
###################################

.. contents::
   :local:
   :depth: 2

This changelog reflects the most relevant changes from the latest official release.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.0.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

|no_changes_yet_note|

Board support
=============

* Added:

  * Bias-pull-up for Thingy:91 X nRF9151 UART RX pins.
  * Alternative partition tables for Thingy:91 X.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

|no_changes_yet_note|

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

* Increased the default value of the :kconfig:option:`CONFIG_MPSL_HFCLK_LATENCY` Kconfig option to support slower crystals.
  See the Kconfig description for a detailed description on how to select the correct value for a given application.
* Added the :ref:`ug_nrf54l_dfu_config` documentation page, describing how to configure Device Firmware Update (DFU) and secure boot settings using MCUboot and NSIB.

Developing with nRF54H Series
=============================

|no_changes_yet_note|

Developing with nRF53 Series
============================

|no_changes_yet_note|

Developing with nRF52 Series
============================

|no_changes_yet_note|

Developing with Thingy:91 X
===========================

|no_changes_yet_note|

Developing with Thingy:91
=========================

|no_changes_yet_note|

Developing with Thingy:53
=========================

|no_changes_yet_note|

Developing with PMICs
=====================

|no_changes_yet_note|

Developing with Front-End Modules
=================================

* Added:

  * Support for the nRF21540 Front-End Module in GPIO/SPI mode for nRF54L Series devices.

Developing with custom boards
=============================

|no_changes_yet_note|

Security
========

* Added the new section about :ref:`ug_crypto_index`.
  The new section includes pages about :ref:`ug_crypto_architecture` (new page) and :ref:`crypto_drivers` (moved from :ref:`nrf_security` library).

* Updated:

  * The :ref:`ug_tfm_logging` page with more details about how to configure logging on the same UART instance as the application for nRF5340 and nRF91 Series devices.
  * The :ref:`crypto_drivers` page with more details about the driver selection process.

Protocols
=========

|no_changes_yet_note|

Amazon Sidewalk
---------------

|no_changes_yet_note|

Bluetooth® LE
-------------

* Added the :kconfig:option:`CONFIG_BT_CTLR_CHANNEL_SOUNDING_TEST` Kconfig option.
  This option reduces the NVM usage of Channel Sounding when disabled by removing the ``LE CS Test`` and ``LE CS Test End`` HCI commands.

|no_changes_yet_note|

Bluetooth Mesh
--------------

|no_changes_yet_note|

DECT NR+
--------

|no_changes_yet_note|

Enhanced ShockBurst (ESB)
-------------------------

|no_changes_yet_note|

Gazell
------

|no_changes_yet_note|

Matter
------

* Added FastTrack Recertification and Portfolio Certification programs.

* Updated:

  * The ``west zap-generate`` command to remove previously generated ZAP files before generating new files.
    To skip removing the files, use the ``--keep-previous`` argument.
  * The :ref:`ug_matter_creating_custom_cluster` user guide by adding information about implementing custom commands.

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

* Updated:

  * The Kconfig option :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD` has been replaced by :kconfig:option:`CONFIG_NRF_802154_CCA_ED_THRESHOLD_DBM` to ensure consistent behavior on different SoC families and to reduce the likelihood of misconfiguration.

Thread
------

|no_changes_yet_note|


Wi-Fi®
------

|no_changes_yet_note|

Applications
============

|no_changes_yet_note|

Connectivity bridge
-------------------

* Fixed to resume Bluetooth connectable advertising after a disconnect.


IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

|no_changes_yet_note|

nRF5340 Audio
-------------

* Added:

  * Experimental support for Audio on the nRF5340 DK, with LED state indications and button controls.

* Updated:

  * The application to use the ``NFC.TAGHEADER0`` value from FICR as the broadcast ID instead of using a random ID.
  * The application to change from Newlib to Picolib to align with |NCS| and Zephyr.
  * The application to use the :ref:`net_buf_interface` API to pass audio data between threads.
    The :ref:`net_buf_interface` will also contain the metadata about the audio stream in the ``user_data`` section of the API.
    This change was done to transition to standard Zephyr APIs, as well as to have a structured way to pass N-channel audio between modules.
  * The optional buildprog tool to use `nRF Util`_ instead of nrfjprog that has been deprecated.
  * The documentation pages with information about the :ref:`SD card playback module <nrf53_audio_app_overview_architecture_sd_card_playback>` and :ref:`how to enable it <nrf53_audio_app_configuration_sd_card_playback>`.

* Removed:

  * The uart_terminal tool to use standardized tools.
    Similar functionality is provided through the `nRF Terminal <nRF Terminal documentation_>`_ in the |nRFVSC|.

nRF Desktop
-----------

* Added:

  * The :ref:`nrf_desktop_hid_eventq`.
    The utility can be used by an application module to temporarily queue HID events related to keypresses (button press or release) to handle them later.
  * The :ref:`nrf_desktop_hid_keymap`.
    The utility can be used by an application module to map an application-specific key ID to a HID report ID and HID usage ID pair according to statically defined user configuration.
    The :file:`hid_keymap.h` file was moved from the :file:`configuration/common` directory to the :file:`src/util` directory.
    The file is now the header of the :ref:`nrf_desktop_hid_keymap` and contains APIs exposed by the utility.

* Updated:

  * The application configurations for dongles on memory-limited SoCs (such as nRF52820) to reuse the system workqueue for GATT Discovery Manager (:kconfig:option:`CONFIG_BT_GATT_DM_WORKQ_SYS`).
    This helps to reduce RAM usage.
  * Link Time Optimization (:kconfig:option:`CONFIG_LTO`) to be enabled in MCUboot configurations of the nRF52840 DK (``mcuboot_smp``, ``mcuboot_qspi``).
    LTO no longer causes boot failures and it reduces the memory footprint.
  * The :ref:`nrf_desktop_hids` to use shared callbacks for multiple HID reports:

    * Use the :c:func:`bt_hids_inp_rep_send_userdata` function to send HID input reports while in report mode.
    * Use an extended callback with the notification event to handle subscriptions for HID input reports in report mode (:c:struct:`bt_hids_inp_rep`).
    * Use generic callbacks to handle HID feature and output reports.

    This approach simplifies the process of adding support for new HID reports.
  * The :ref:`nrf_desktop_hid_state` to:

    * Use the :ref:`nrf_desktop_hid_eventq` to temporarily queue HID events related to keypresses before a connection to the HID host is established.
    * Use the :ref:`nrf_desktop_hid_keymap` to map an application-specific key ID from :c:struct:`button_event` to a HID report ID and HID usage ID pair.

    The features were implemented directly in the HID state module before.
    This change simplifies the HID state module implementation and allows code reuse.
  * The HID input and output report maps (``input_reports`` and ``output_reports`` arrays defined in the :file:`configuration/common/hid_report_desc.h` file) to contain only IDs of enabled HID reports.
  * The default value of the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT` Kconfig option to ``64``.
    This ensures that more complex configurations fit in the limit.
  * The :ref:`nrf_desktop_hid_reportq` to accept HID report IDs that do not belong to HID input reports supported by the application (are not part of the ``input_reports`` array defined in :file:`configuration/common/hid_report_desc.h` file).
    Before the change, providing an unsupported HID report ID caused an assertion failure.
    Function signatures of the :c:func:`hid_reportq_subscribe` and :c:func:`hid_reportq_unsubscribe` functions were slightly changed (both functions return an error in case the provided HID report ID is unsupported).

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Serial LTE modem
----------------

* Added:

  * The ``AT#XAPOLL`` command to asynchronously poll sockets for data.
  * The send flags for ``#XSEND``, ``#XSENDTO``, ``#XTCPSEND`` and ``#XUDPSEND`` commands.
  * The send flag value ``512`` for waiting for acknowledgment of the sent data.

* Updated:

  * The ``AT#XPPP`` command to support the CID parameter to specify the PDN connection used for PPP.
  * The ``#XPPP`` notification to include the CID of the PDN connection used for PPP.
  * The initialization of the application to ignore a failure in nRF Cloud module initialization.
    This occurs sometimes especially during development.
  * The initialization of the application to send "INIT ERROR" over to UART and show clear error log to indicate that the application is not operational in case of failing initialization.
  * The PPP downlink data to trigger the indicate pin when SLM is in idle.
  * The ``AT#XTCPCLI`` and the ``AT#XUDPCLI`` commands to support CID of the PDN connection.

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Amazon Sidewalk samples
-----------------------

|no_changes_yet_note|

Bluetooth samples
-----------------

* Added experimental ``llvm`` toolchain support for the nRF54L Series board targets to the following samples:

  * :ref:`peripheral_lbs`
  * :ref:`central_uart`
  * :ref:`power_profiling`

* :ref:`bluetooth_isochronous_time_synchronization` sample:

  * Fixed an issue where the sample would assert with the :kconfig:option:`CONFIG_ASSERT` Kconfig option enabled.
    This was due to calling the :c:func:`bt_iso_chan_send` function from a timer ISR handler and sending SDUs to the controller with invalid timestamps.

* :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples:

  * Added a workaround to an issue with unexpected disconnections that resulted from improper handling of the Bluetooth Link Layer procedures by the connected Bluetooth Central device.
    This resolves the :ref:`known issue <known_issues>` NCSDK-33632.

|no_changes_yet_note|

Bluetooth Mesh samples
----------------------

|no_changes_yet_note|

Bluetooth Fast Pair samples
---------------------------

* :ref:`fast_pair_locator_tag` sample:

  * Added possibility to build and run the sample without the motion detector support (with the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR` Kconfig option disabled).
  * Updated the :ref:`fast_pair_locator_tag_testing_fw_update_notifications` section to improve the test procedure.
    The application provides now an additional log message to indicate that the firmware version is being read.

Cellular samples
----------------

* Added support for the Thingy:91 X to the following samples:

  * :ref:`nrf_cloud_rest_device_message`
  * :ref:`nrf_cloud_rest_cell_location`
  * :ref:`nrf_cloud_rest_fota`

* Deprecated the LTE Sensor Gateway sample.
  It is no longer maintained.

* :ref:`modem_shell_application` sample:

  * Added ``ATE0`` and ``ATE1`` in AT command mode to handle echo off/on.

* nRF Cloud multi-service sample:

  * Added support for native simulator platform and updated the documentation accordingly.

* :ref:`nrf_provisioning_sample` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.

* :ref:`nrf_cloud_rest_device_message` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
  * Removed Provisioning service and JITP.

* :ref:`nrf_cloud_rest_cell_location` sample:

  * Removed JITP.

* :ref:`nrf_cloud_rest_fota` sample:

  * Updated the sample to use Zephyr's :ref:`zephyr:conn_mgr_docs` feature.
  * Fixed SMP FOTA for the nRF9160 DK.
  * Removed JITP.

Cryptography samples
--------------------

|no_changes_yet_note|

Debug samples
-------------

|no_changes_yet_note|

DECT NR+ samples
----------------

|no_changes_yet_note|

Edge Impulse samples
--------------------

|no_changes_yet_note|

Enhanced ShockBurst samples
---------------------------

|no_changes_yet_note|

Gazell samples
--------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Changed Bluetooth Low Energy variant of the Soft Device Controller (SDC) to use the Peripheral-only role in all Matter samples.

Networking samples
------------------

* :ref:`download_sample` sample:

  * Added the :ref:`CONFIG_SAMPLE_PROVISION_CERT <CONFIG_SAMPLE_PROVISION_CERT>` Kconfig option to provision the root CA certificate to the modem.
    The certificate is provisioned only if the :ref:`CONFIG_SAMPLE_SECURE_SOCKET <CONFIG_SAMPLE_SECURE_SOCKET>` Kconfig option is set to ``y``.
  * Fixed an issue where the network interface was not re-initialized after a fault.

NFC samples
-----------

* Added experimental ``llvm`` toolchain support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target to the following samples:

  * :ref:`writable_ndef_msg`
  * :ref:`nfc_shell`

* :ref:`record_text` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`radio_test` sample:

  * Added experimental ``llvm`` toolchain support for the ``nrf54l15dk/nrf54l15/cpuapp`` board target.

PMIC samples
------------

|no_changes_yet_note|

Protocol serialization samples
------------------------------

|no_changes_yet_note|

SDFW samples
------------

|no_changes_yet_note|

Sensor samples
--------------

|no_changes_yet_note|

SUIT samples
------------

|no_changes_yet_note|

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_secure_peripheral_partition` sample:

  * Added support for the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target.

Thread samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

* :ref:`wifi_radiotest_samples`:

  * Updated :ref:`wifi_radio_test` and :ref:`wifi_radio_test_sd` samples to clarify platform support for single-domain and multi-domain radio tests.

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

* Added the :ref:`mspi_sqspi` that allows for communication with devices that use MSPI bus-based Zephyr drivers.

Wi-Fi drivers
-------------

|no_changes_yet_note|

Flash drivers
-------------

|no_changes_yet_note|

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.

Binary libraries
----------------

|no_changes_yet_note|

Bluetooth libraries and services
--------------------------------

* :ref:`bt_fast_pair_readme` library:

  * Updated the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_MOTION_DETECTOR` Kconfig option dependency.
    The dependency has been updated from the :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT` Kconfig option to :kconfig:option:`CONFIG_BT_FAST_PAIR_FMDN_DULT_MOTION_DETECTOR`.

Common Application Framework
----------------------------

* :ref:`caf_ble_state`:

  * Removed the tracking of the active Bluetooth connections.
    CAF no longer assumes that the Bluetooth Peripheral device (:kconfig:option:`CONFIG_BT_PERIPHERAL`) supports only one simultaneous connection (:kconfig:option:`CONFIG_BT_MAX_CONN`).

Debug libraries
---------------

* Added an experimental :ref:`Zephyr Core Dump <zephyr:coredump>` backend that writes a core dump to an internal flash or RRAM partition.
  To enable this backend, set the :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_OTHER` and :kconfig:option:`CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION` Kconfig options.

* :ref:`cpu_load` library:

  * Added prefix ``NRF_`` to all Kconfig options (for example, :kconfig:option:`CONFIG_NRF_CPU_LOAD`) to avoid conflicts with Zephyr Kconfig options with the same names.

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

* :ref:`nrf_security` library:

  * Updated:

    * The name of the Kconfig option ``CONFIG_PSA_USE_CRACEN_ASYMMETRIC_DRIVER`` to :kconfig:option:`CONFIG_PSA_USE_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER`, which is more descriptive and more consistent with the options of the other drivers.
    * The placement of the page about nRF Security drivers.
      The page was moved to :ref:`ug_crypto_index` and renamed to :ref:`crypto_drivers`.


Modem libraries
---------------

* :ref:`nrf_modem_lib_readme`:

  * Fixed an issue with modem fault handling in the :ref:`nrf_modem_lib_lte_net_if`, where the event must be deferred from interrupt context before it can be forwarded to the Zephyr's :ref:`net_mgmt_interface` module.

* :ref:`at_parser_readme` library:

  * Added support for parsing DECT NR+ modem firmware names.

  * Updated the following macros and functions to return ``-ENODATA`` when the target subparameter to parse is empty:

    * :c:macro:`at_parser_num_get` macro
    * Functions:

      * :c:func:`at_parser_int16_get`
      * :c:func:`at_parser_uint16_get`
      * :c:func:`at_parser_int32_get`
      * :c:func:`at_parser_uint32_get`
      * :c:func:`at_parser_int64_get`
      * :c:func:`at_parser_uint64_get`
      * :c:func:`at_parser_string_get`

* :ref:`lte_lc_readme` library:

  * Added the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` and :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig options to enable setting a fallback DNS address.
    The :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_MODULE` Kconfig option is enabled by default.
    If the application has configured a DNS server address in Zephyr's native networking stack, using the :kconfig:option:`CONFIG_DNS_SERVER1` Kconfig option, the same server is set as the fallback address for DNS queries offloaded to the nRF91 Series modem.
    Otherwise, the :kconfig:option:`CONFIG_LTE_LC_DNS_FALLBACK_ADDRESS` Kconfig option controls the fallback DNS server address that is set to Cloudflare's DNS server: 1.1.1.1 by default.
    The device might or might not receive a DNS address by the network during PDN connection.
    Even within the same network, the PDN connection establishment method (PCO vs ePCO) might change when the device operates in NB-IoT or LTE Cat-M1, resulting in missing DNS addresses when one method is used, but not the other.
    Having a fallback DNS address ensures that the device always has a DNS to fallback to.

* Modem SLM library:

  * Added:

    * The ``CONFIG_MODEM_SLM_UART_RX_BUF_COUNT`` Kconfig option for configuring RX buffer count.
    * The ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` Kconfig option for configuring RX buffer size.
    * The ``CONFIG_MODEM_SLM_UART_TX_BUF_SIZE`` Kconfig option for configuring TX buffer size.
    * The ``CONFIG_MODEM_SLM_AT_CMD_RESP_MAX_SIZE`` Kconfig option for buffering AT command responses.

  * Updated:

      * The software maturity of the library to supported instead of experimental.
      * The UART implementation between the host device, using the Modem SLM library, and the device running the Serial LTE modem application.

  * Removed:

    * The ``CONFIG_MODEM_SLM_DMA_MAXLEN`` Kconfig option.
      Use ``CONFIG_MODEM_SLM_UART_RX_BUF_SIZE`` instead.
    * The ``modem_slm_reset_uart()`` function, as there is no longer a need to reset the UART.

* :ref:`modem_info_readme` library:

  * Added:

    * The :c:func:`modem_info_get_rsrq` function for requesting the RSRQ.
    * The :c:macro:`SNR_IDX_TO_DB` macro for converting the SNR index to dB.

Multiprotocol Service Layer libraries
-------------------------------------

* Added an implementation of the API required by the MPSL (defined by :file:`mpsl_hwres.h`) for the nRF53 and nRF54L Series devices.

* Updated the implementation of the following interrupt service routine wrappers:

  * :c:func:`mpsl_timer0_isr_wrapper`
  * :c:func:`mpsl_rtc0_isr_wrapper`
  * :c:func:`mpsl_radio_isr_wrapper`

  Now, they do not trigger the kernel scheduler or use any kernel APIs.

  .. note::

     Invoking kernel APIs or triggering the kernel scheduler from Zero Latency Interrupts is considered undefined behavior.
     Users of MPSL timeslots should not assume that thread rescheduling will occur automatically at the end of a timeslot.

Libraries for networking
------------------------

* :ref:`lib_nrf_cloud` library:

  * Updated:

    * To return negative :file:`errno.h` errors instead of positive ZCBOR errors.
    * The CoAP download authentication to no longer depend on the :ref:`CoAP Client library <zephyr:coap_client_interface>`.

* :ref:`lib_nrf_provisioning` library:

  * Added

    * The :kconfig:option:`CONFIG_NRF_PROVISIONING_INITIAL_BACKOFF` Kconfig option to configure the initial backoff time for provisioning retries.
    * The :kconfig:option:`CONFIG_NRF_PROVISIONING_STACK_SIZE` Kconfig option to configure the stack size of the provisioning thread.
    * A new query parameter to limit the number of provisioning commands included in a single provisioning request.
      This limit can be configured using the :kconfig:option:`CONFIG_NRF_PROVISIONING_CBOR_RECORDS` Kconfig option.

  * Updated:

    * Limited key-value pairs in a single provisioning command to ``10``.
      This is done to reduce the RAM usage of the library.

  * Fixed an issue where the results from the :c:func:`zsock_getaddrinfo` function were not freed when the CoAP protocol was used for connection establishment.

* :ref:`lib_downloader` library:

  * Fixed:

    * A bug in the shell implementation causing endless download retries on errors.
    * A bug in the shell to allow multiple downloads.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

* :ref:`dult_readme` library:

  * Updated the write handler of the accessory non-owner service (ANOS) GATT characteristic to no longer assert on write operations if the DULT was not enabled at least once.

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

* Added the :file:`ncs_ironside_se_update.py` script in the :file:`scripts/west_commands` folder.
  The script adds the west command ``west ncs-ironside-se-update`` for installing an IronSide SE update.

* :ref:`nrf_desktop_config_channel_script` Python script:

  * Updated:

    * The udev rules for Debian, Ubuntu, and Linux Mint HID host computers (replaced the :file:`99-hid.rules` file with :file:`60-hid.rules`).
      This is done to ensure that the rules are properly applied for an nRF Desktop device connected directly over Bluetooth LE.
      The new udev rules are applied to any HID device that uses the Nordic Semiconductor Vendor ID (regardless of Product ID).
    * The HID device discovery to ensure that a discovery failure of a HID device would not affect other HID devices.
      Without this change, problems with discovery of a HID device could lead to skipping discovery and listing of other HID devices (even if the devices work properly).

Integrations
============

This section provides detailed lists of changes by :ref:`integration <integrations>`.

Google Fast Pair integration
----------------------------

|no_changes_yet_note|

Edge Impulse integration
------------------------

|no_changes_yet_note|

Memfault integration
--------------------

|no_changes_yet_note|

AVSystem integration
--------------------

|no_changes_yet_note|

nRF Cloud integration
---------------------

|no_changes_yet_note|

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``81315483fcbdf1f1524c2b34a1fd4de6c77cd0f4``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:


* Fixed an issue related to referencing the ARM Vector table of the application, which causes jumping to wrong address instead of the application reset vector for some builds when Zephyr LTO (Link Time Optimization) was enabled.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``9a6f116a6aa9b70b517a420247cd8d33bbbbaaa3``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 9a6f116a6a ^fdeb735017

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^9a6f116a6a

The current |NCS| main branch is based on revision ``9a6f116a6a`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Trusted Firmware-M
==================

|no_changes_yet_note|

cJSON
=====

|no_changes_yet_note|

Documentation
=============

* Added the :ref:`log_rpc` library documentation page.
