.. _ncs_release_notes_changelog:

Changelog for |NCS| v3.3.99
###########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the main branch of the |NCS|, as compared to the latest official release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

.. HOWTO

   When adding a new PR, decide whether it needs an entry in the changelog.
   If it does, update this page.
   Add the sections you need, as only a handful of sections are kept when the changelog is cleaned.
   The "Protocols" section serves as a highlight section for all protocol-related changes, including those made to samples, libraries, and other components that implement or support protocol functionality.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v3.3.0`_ for the list of issues valid for the latest release.

Changelog
*********

The following sections provide detailed lists of changes by component.

IDE, OS, and tool support
=========================

|no_changes_yet_note|

Board support
=============

* Updated the Thingy:91 and Thingy:91 X devicetree partitions to match the Partition Manager partitions.

Build and configuration system
==============================

|no_changes_yet_note|

Bootloaders and DFU
===================

|no_changes_yet_note|

* Added support for the new LCS API to control bootloader behavior.
  When this API is enabled, the bootloader can skip signature verification in early LCS states and abort the boot process in the decommissioned state.

Developing with nRF91 Series
============================

|no_changes_yet_note|

Developing with nRF70 Series
============================

|no_changes_yet_note|

Developing with nRF54L Series
=============================

|no_changes_yet_note|

Developing with nRF54H Series
=============================

* Updated:

   * MCUboot to support MRAM write protection for up to four images.

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

|no_changes_yet_note|

Developing with custom boards
=============================

|no_changes_yet_note|

Security
========

* Added:

  * Support for the X25519 key pair storage in the :ref:`Key Management Unit (KMU) <ug_kmu_guides_supported_key_types>`.

* Updated:

  * How the :kconfig:option:`CONFIG_NRF_SECURITY` and :kconfig:option:`CONFIG_PSA_CRYPTO` interact with each other.
    :kconfig:option:`CONFIG_NRF_SECURITY` is now promptless and auto-enabled indirectly by :kconfig:option:`CONFIG_PSA_CRYPTO`.
  * Approach to store keys in the KMU so that AEAD algorithms with non-default (shortened) tag lengths are supported.

* Removed:

  * The following Kconfig options:

    * ``CONFIG_NRF_SECURITY_ADVANCED``

Trusted Firmware-M (TF-M)
-------------------------

|no_changes_yet_note|

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Bluetooth® LE
-------------

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

|no_changes_yet_note|

Matter fork
+++++++++++

|no_changes_yet_note|

nRF IEEE 802.15.4 radio driver
------------------------------

|no_changes_yet_note|

Thread
------

|no_changes_yet_note|

Wi-Fi®
------

|no_changes_yet_note|

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

Connectivity bridge
-------------------

|no_changes_yet_note|

High-Performance Framework (HPF)
--------------------------------

* Added support for the nRF54LV10A SoC in HPF MSPI.

IPC radio firmware
------------------

|no_changes_yet_note|

Matter bridge
-------------

|no_changes_yet_note|

nRF5340 Audio
-------------

* Added the :kconfig:option:`CONFIG_BT_AUDIO_BROADCAST_BASE_PRINT` kconfig option to print the contents of the BASE when it is received.
  This option is intended for debugging purposes.
  It is only valid for the :ref:`broadcast sink application <nrf53_audio_broadcast_sink_app>`.
* :kconfig:option:`CONFIG_LTO` to enable link-time optimization for the application.
  It improves application performance and reduces code size.

* Updated the call to :c:func:`hci_vs_sdc_iso_read_tx_timestamp` so that, when sending ISO data, it is performed at regular intervals instead of every SDU interval.
  This change reduces the frequency of application-controller time synchronization, while significantly reducing processing overhead.

nRF Desktop
-----------

|no_changes_yet_note|

nRF Machine Learning (Edge Impulse)
-----------------------------------

|no_changes_yet_note|

Thingy:53: Matter weather station
---------------------------------

|no_changes_yet_note|

Installer (MCUboot Firmware Loader installer)
-----------------------------------------------

|no_changes_yet_note|

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added:

  * The :ref:`channel_sounding_ipt_initiator` and :ref:`channel_sounding_ipt_reflector` samples to demonstrate how to use the Bluetooth Channel Sounding (CS) Inline Phase Correction Term Transfer (IPT) feature.

Bluetooth Mesh samples
----------------------

* :ref:`bluetooth_mesh_light` sample:

  * The partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target is not backward compatible with the previous versions of the sample.
    This affects builds with support of point-to-point Device Firmware Update (DFU) over the Simple Management Protocol (SMP).

  * Updated the approach to build the sample with point-to-point Device Firmware Update (DFU) over the Simple Management Protocol (SMP).
    There are no :file:`overlay-dfu.conf` and :file:`sysbuild-dfu.conf` files anymore.
    DFU support is enabled as a suffixed configuration.

* :ref:`bluetooth_mesh_light_lc` sample:

  * The partition layout for the ``nrf5340dk/nrf5340/cpuapp/ns`` board target is not backward compatible with the previous versions of the sample.
    This affects builds with support for the EMDS.

* :ref:`ble_mesh_dfu_distributor` sample:

  * The partition layout for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets is not backward compatible with the previous versions of the sample.

* :ref:`ble_mesh_dfu_target` sample:

  * The partition layout for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54l15dk/nrf54l10/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets is not backward compatible with the previous versions of the sample.

Bluetooth Fast Pair samples
---------------------------

* Updated the DTS overlays of the Fast Pair samples to use the new ``zephyr,mapped-partition`` compatible property for each partition node.
  The change aligns the board target overlays with the layout produced by the Partition Manager-to-DTS helper script (:file:`scripts/pm_to_dts.py`).

* Removed the remaining ``nrf54h20dk/nrf54h20/cpuapp`` board target configurations from the following samples:

  * :ref:`fast_pair_locator_tag`
  * :ref:`fast_pair_input_device`

  Updated the sample documentation to remove all references to the ``nrf54h20dk/nrf54h20/cpuapp`` board target, including descriptions of nRF54H-specific solutions such as the legacy SUIT DFU integration.

  Support for this target was already dropped during the IronSide SE migration in the |NCS| v3.1.0 release.

Cellular samples
----------------

|no_changes_yet_note|

Cryptography samples
--------------------

* Added support for the ``nrf54ls05dk/nrf54ls05a/cpuapp`` board target to all samples, except :ref:`crypto_aes_kw`, :ref:`crypto_kmu_cracen_usage`, and :ref:`crypto_persistent_key`.

Debug samples
-------------

|no_changes_yet_note|

DFU samples
-----------

* Updated:

  * The ref:`mcuboot_minimal_configuration` has been moved to the :file:`samples/dfu` directory.

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

|ISE| samples
-------------

|no_changes_yet_note|

Keys samples
------------

|no_changes_yet_note|

Matter samples
--------------

* Removed support for the nRF7002 DK and nRF5340 DK used with the nRF7002 EK shield from all Matter samples, as this configuration was deprecated in |NCS| v3.2.0.
  The removal is mainly due to the very limited non-volatile memory available for application code.
  As an alternative, use the nRF54LM20A or nRF54LM20B SoC with the nRF7002-EB II shield.
  This combination provides significantly more non-volatile memory for Matter over Wi-Fi applications.

Networking samples
------------------

* :ref:`azure_iot_hub` sample:

  * Added support for the nRF54LM20 DK with the nRF7002-EB II shield.

NFC samples
-----------

|no_changes_yet_note|

nRF5340 samples
---------------

|no_changes_yet_note|

Peripheral samples
------------------

* :ref:`802154_phy_test` sample:

  * Removed unused functions from the :file:`rf_proc.c` file and the :file:`rf_proc.h` header file.

* :ref:`radio_test` sample:

  * Added:

    * ``start_tx_sweep_with_sleep`` shell command that allows for running a TX sweep with silent periods between each frequency.
    * ``start_tx_sweep_with_sleep_modulated`` shell command that allows for running a modulated TX sweep with silent periods between each frequency.
    * ``set_channel_sequence`` shell command that allows for setting a custom channel sequence for the ``start_tx_sweep_with_sleep`` and ``start_tx_sweep_with_sleep_modulated`` commands.
    * ``print_channel_sequence`` shell command that prints the currently configured channel sequence.
    * ``set_channel_sequence_hopping_mode`` shell command that allows for setting the hopping mode for the channel sequence.
    * Support for pin debugging of radio events on nRF54L Series devices.

  * Updated the ``start_duty_cycle_modulated_tx`` command to accept an optional argument specifying the number of packets to transmit.
    If no additional argument is provided, the command works as before.
  * Fixed an issue where the ``start_duty_cycle_modulated_tx`` command would not work correctly on nRF52 Series devices if the command was triggered more than once.

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

|no_changes_yet_note|

Thread samples
--------------

|no_changes_yet_note|

Wi-Fi samples
-------------

|no_changes_yet_note|

Other samples
-------------

|no_changes_yet_note|

Drivers
=======

This section provides detailed lists of changes by :ref:`driver <drivers>`.

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

* :ref:`liblwm2m_carrier_readme` library:

  * Added support for building without Partition Manager.
    This requires an ``lwm2m_carrier`` partition to be defined in devicetree.

Bluetooth libraries and services
--------------------------------

|no_changes_yet_note|

Common Application Framework
----------------------------

|no_changes_yet_note|

Debug libraries
---------------

|no_changes_yet_note|

DFU libraries
-------------

|no_changes_yet_note|

Gazell libraries
----------------

|no_changes_yet_note|

Security libraries
------------------

|no_changes_yet_note|

Modem libraries
---------------

* :ref:`nrf_modem_lib_readme` library:

  * Added support for building for the nRF91 board without Partition Manager.
    This requires ``cpuapp_cpucell_ipc_shm_ctrl``, ``cpuapp_cpucell_ipc_shm_heap``, and ``cpucell_cpuapp_ipc_shm_heap`` partitions to be defined in the devicetree.
  * Use a separate ``cpucell_cpuapp_ipc_shm_trace`` partition for modem tracing instead of splitting the ``cpucell_cpuapp_ipc_shm_heap`` partition.

* Removed the deprecated PDN library.
  Use the PDN management functionality in the :ref:`lte_lc_readme` library instead.

Multiprotocol Service Layer libraries
-------------------------------------

|no_changes_yet_note|

Libraries for networking
------------------------

* :ref:`lib_nrf_provisioning` library:

  * Removed dependency on the :ref:`lte_lc_readme` library.
    The :ref:`at_monitor_readme` library is now used for tracking network connectivity status.

* :ref:`lib_nrf_cloud` library:

  * Fixed:

    * An issue where PEM private keys with CRLF line endings could not be decoded correctly for JWT signing used in nRF Cloud CoAP authentication.
    * A DTLS socket option setup to inspect ``errno`` after ``setsockopt()`` failures instead of comparing the return value to ``EOPNOTSUPP`` or ``EINVAL``, which could mishandle unsupported options during DTLS configuration.
    * The DTLS handshake timeout configuration on native (non-modem) sockets by using ``TLS_DTLS_HANDSHAKE_TIMEOUT_MIN`` and ``TLS_DTLS_HANDSHAKE_TIMEOUT_MAX`` instead of the modem-only ``TLS_DTLS_HANDSHAKE_TIMEO`` option, avoiding handshake failures when connecting to nRF Cloud over Wi-Fi.
    * The internal CoAP client socket descriptor not being updated immediately after a new DTLS socket was connected, which could leave ``client->cc.fd`` stale during reconnect and interfere with CoAP polling, receive, and retransmit paths.

  * Removed the nRF Cloud Alerts library.
    As part of the migration to *nRF Cloud powered by Memfault*, the nRF Cloud Alerts feature is now redundant.
    `Memfault's Trace Events <Memfault: Error Tracking with Trace Events_>`_ feature replaces the Alerts feature, as it provides equivalent functionality for event reporting, and it also adds enhanced debugging capabilities that were not available with Alerts.

* :ref:`lib_nrf_cloud_pgps` library:

  * Updated the range for the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_NUM_PREDICTIONS` and :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_REPLACEMENT_THRESHOLD` Kconfig options to values supported by nRF Cloud.

  * Fixed:

    * An issue that caused predictions to be stored into incorrect index in flash, leading to an error when the affected predictions were used.
    * An issue where the library tried to use predictions that had not yet been flushed to flash, leading to a prediction validation error.
    * An issue where the library kept trying to use corrupted predictions.

Libraries for NFC
-----------------

|no_changes_yet_note|

nRF RPC libraries
-----------------

|no_changes_yet_note|

Other libraries
---------------

|no_changes_yet_note|

Shell libraries
---------------

|no_changes_yet_note|

sdk-nrfxlib
-----------

See the changelog for each library in the :doc:`nrfxlib documentation <nrfxlib:README>` for additional information.

Scripts
=======

This section provides detailed lists of changes by :ref:`script <scripts>`.

* :ref:`west_sbom` script:

* Added:

  * Licence handling of the LLVM toolchain libraries.
  * Support for hex only ``softdevice`` sysbuild domains.
  * Updated the SPDX License List database to version 3.28.0.

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

* Added a ``memfaultEn`` control key in the Device Shadow, enabling dynamic enable/disable of Memfault chunk uploads at runtime.
  This is done through Device Shadow updates.
  A new control variable is used.

CoreMark integration
--------------------

|no_changes_yet_note|

DULT integration
----------------

|no_changes_yet_note|

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``8d14eebfe0b7402ebdf77ce1b99ba1a3793670e9``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Fixed a possible infinite loop in the bootloader information sharing procedure when the Kconfig options :kconfig:option:`CONFIG_BOOT_SHARE_DATA_BOOTINFO` and :kconfig:option:`CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION` are enabled.
  Iterating boot-info security counters could spin forever if the :c:func:`boot_nv_security_counter_get` function failed for an image index (for example, backends that expose only one NV counter when :kconfig:option:`CONFIG_BOOT_IMAGE_NUMBER` is greater than one).

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``684c9e8f32e4373a21098559f748f06915f950c9``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 684c9e8f32 ^911b3da139

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^684c9e8f32

The current |NCS| main branch is based on revision ``684c9e8f32`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Additions specific to |NCS|
---------------------------

|no_changes_yet_note|

zcbor
=====

|no_changes_yet_note|

Documentation
=============

* Added:

  * Description of the MRAM auto-powerdown tradeoff in the :ref:`ug_nrf54h20_pm_optimization` documentation page.
  * MSD disable step in the :ref:`ug_nrf54h20_gs` guide.
  * MRAM11 Ironside SE update limitations in the :ref:`ug_nrf54h20_ironside_update` and :ref:`ug_nrf54h20_ironside_services` documentation pages.

Updated:

  * The :ref:`ug_nrf54h20_architecture_lifecycle` documentation page, expanding the description of the LCS states.
