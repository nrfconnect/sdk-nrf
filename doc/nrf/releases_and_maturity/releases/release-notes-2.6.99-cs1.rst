.. _ncs_release_notes_2699_cs1:

|NCS| v2.6.99-cs1 Release Notes
###############################

.. contents::
   :local:
   :depth: 3

|NCS| v2.6.99-cs1 is a **customer sampling** release, tailored exclusively for participants in the nRF54L15 customer sampling program.
Do not use this release of |NCS| if you are not participating in the program.

The release is not part of the regular release cycle and is specifically designed to support early adopters working with the nRF54L15 device.
It is intended to provide early access to the latest developments for nRF54L15, enabling participants to experiment with new features and provide feedback.

The scope of this release is intentionally narrow, concentrating solely on the nRF54L15 device.
You can use the latest stable release of |NCS| to work with other boards.

All functionality related to nRF54L15 is considered experimental.

Changelog
*********

The following sections provide detailed lists of changes by component.

Peripherals support
===================

* Added experimental support for the following peripherals on the nRF54L15 PDK:

  * GPIO
  * SPIM
  * UART
  * TWIM
  * SAADC
  * WDT
  * GRTC
  * QDEC
  * I2S
  * PWM

Working with nRF54L Series
==========================

* Added:

  * The :ref:`ug_nrf54l15_gs` page.
  * The :ref:`nrf54l_features` page.

Protocols
=========

This section provides detailed lists of changes by :ref:`protocol <protocols>`.
See `Samples`_ for lists of changes for the protocol-related samples.

Amazon Sidewalk
---------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` in the following configurations:

  * Bluetooth® LE only
  * DFU over Bluetooth LE

Enhanced ShockBurst (ESB)
-------------------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

Applications
============

This section provides detailed lists of changes by :ref:`application <applications>`.

nRF Desktop
-----------

* Added:

  * Support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0``.
    The PDK can act as a sample mouse or keyboard.
    It supports the Bluetooth LE HID data transport and uses SoftDevice Link Layer with Low Latency Packet Mode (LLPM) enabled.
    The PDK uses MCUboot bootloader built in the direct-xip mode (``MCUBOOT+XIP``) and supports firmware updates using the :ref:`nrf_desktop_dfu`.
  * The ``nrfdesktop-wheel-qdec`` DT alias support to :ref:`nrf_desktop_wheel`.
    You must use the alias to specify the QDEC instance used for scroll wheel, if your board supports multiple QDEC instances (for example ``nrf54l15pdk_nrf54l15_cpuapp``).
    You need not define the alias if your board supports only one QDEC instance, because in that case, the wheel module can rely on the ``qdec`` DT label provided by the board.

Samples
=======

This section provides detailed lists of changes by :ref:`sample <samples>`.

Bluetooth samples
-----------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following Bluetooth samples:

  * :ref:`peripheral_lbs` sample
  * :ref:`bluetooth_central_hids` sample
  * :ref:`peripheral_hids_mouse` sample
  * :ref:`peripheral_hids_keyboard` sample
  * :ref:`central_and_peripheral_hrs` sample
  * :ref:`direct_test_mode` sample
  * :ref:`central_uart` sample
  * :ref:`peripheral_uart` sample

Cryptography samples
--------------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` in all cryptography samples:

  * Samples :ref:`crypto_persistent_key` and :ref:`crypto_tls` support the nRF54L15 PDK with the ``nrf54l15pdk_nrf54l15_cpuapp`` build target.
  * All other crypto samples support the nRF54L15 PDK with both ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp_ns`` build targets.

ESB samples
-----------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following ESB samples:

  * :ref:`esb_prx` sample
  * :ref:`esb_ptx` sample

Matter samples
--------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following Matter samples:

  * :ref:`matter_template_sample` sample
  * :ref:`matter_light_bulb_sample` sample
  * :ref:`matter_light_switch_sample` sample
  * :ref:`matter_thermostat_sample` sample
  * :ref:`matter_window_covering_sample` sample

  DFU support for the nRF54L15 PDK is available only for the ``release`` build type.

NFC samples
-----------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following NFC samples:

  * :ref:`record_launch_app` sample
  * :ref:`record_text` sample
  * :ref:`nfc_shell` sample
  * :ref:`nrf-nfc-system-off-sample` sample
  * :ref:`nfc_tnep_tag` sample
  * :ref:`writable_ndef_msg` sample

Peripheral samples
------------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following peripheral samples:

  * :ref:`radio_test` sample
  * :ref:`802154_phy_test` sample

Thread samples
--------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` with build targets ``nrf54l15pdk_nrf54l15_cpuapp`` and ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` in the following Thread samples:

  * :ref:`ot_cli_sample` sample
  * :ref:`ot_coprocessor_sample` sample

Trusted Firmware-M (TF-M) samples
---------------------------------

* :ref:`tfm_hello_world` sample:

  * Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` board.

Other samples
-------------

* Added the :ref:`coremark_sample` sample that demonstrates how to measure a performance of the supported SoCs by running the Embedded Microprocessor Benchmark Consortium (EEMBC) CoreMark benchmark.
  Included support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>`.

Libraries
=========

This section provides detailed lists of changes by :ref:`library <libraries>`.


Security libraries
------------------

* :ref:`nrf_security` library:

  * Added support for PSA crypto on the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>`.
    Enabled the PSA crypto support with a CRACEN driver as the backend by setting the Kconfig option :kconfig:option:`CONFIG_NRF_SECURITY`.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``a4eda30f5b0cfd0cf15512be9dcd559239dbfc91``, with some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in the :file:`ncs/nrf/modules/mcuboot` folder.

The following list summarizes both the main changes inherited from upstream MCUboot and the main changes applied to the |NCS| specific additions:

* Added experimental support for MCUboot-based DFU on the nRF54L15 PDK.

Zephyr
======

.. NOTE TO MAINTAINERS: All the Zephyr commits in the below git commands must be handled specially after each upmerge and each nRF Connect SDK release.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``0051731a41fa2c9057f360dc9b819e47b2484be5``, with some |NCS| specific additions.

For the list of upstream Zephyr commits (not including cherry-picked commits) incorporated into nRF Connect SDK since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline 0051731a41 ^23cf38934c

For the list of |NCS| specific commits, including commits cherry-picked from upstream, run:

.. code-block:: none

   git log --oneline manifest-rev ^0051731a41

The current |NCS| main branch is based on revision ``0051731a41`` of Zephyr.

.. note::
   For possible breaking changes and changes between the latest Zephyr release and the current Zephyr version, refer to the :ref:`Zephyr release notes <zephyr_release_notes>`.

Zephyr samples
--------------

* Added support for the :ref:`nRF54L15 PDK <ug_nrf54l15_gs>` in the following Zephyr samples:

  * :zephyr:code-sample:`blinky`
  * :ref:`zephyr:hello_world`
  * :zephyr:code-sample:`settings`
  * :zephyr:code-sample:`bluetooth_observer`
  * :zephyr:code-sample:`nrf_system_off`
  * :zephyr:code-sample:`ble_peripheral_hr`
  * :zephyr:code-sample:`ble_central_hr`
  * :zephyr:code-sample:`smp-svr`

Current nRF54L15 PDK Limitations
================================

General
-------

* The v0.2.1 revision of the nRF54L15 PDK has Button 3 and Button 4 connected to GPIO port 2 that does not support interrupts.
  An example of software workaround for this issue is implemented in the :ref:`dk_buttons_and_leds_readme` library.
  The workaround is enabled by default with the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option, but it increases the overall power consumption of the system.

Amazon Sidewalk
---------------

* LoRa/FSK is not supported.
* After a successful image transfer, the DFU image swap for the Hello World application takes almost 80 seconds.
* After an auto-connect, the first sent message contains corrupted data.
  A workaround for this issue is to disable the auto-connect or set up the connection manually before sending the uplink message.

Matter
------

* The Device Firmware Update (DFU) is available only for the internal RAM memory space and for the ``release`` build configuration.
* External flash memory is not supported.
* A crash might occur when the Matter thread and Openthread thread are using PSA API concurrently.
  Such crash might be observed during the commissioning process to the next Matter fabric when the device is already connected to a Matter fabric.

Thread
------

* Link Metrics is not supported.
* Thread commissioning fails since EC J-PAKE is not supported.

Documentation
=============

* Added:

  * The :ref:`ug_nrf54l` section.
  * The :ref:`ug_coremark` page.

* Updated the table listing the :ref:`boards included in sdk-zephyr <app_boards_names_zephyr>` with the nRF54L15 PDK board.
