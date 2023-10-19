.. _lib_bt_ll_acs_nrf53_readme:

LE Audio controller for nRF5340
###############################

.. contents::
   :local:
   :depth: 2

The LE Audio controller for nRF5340 is a link layer controller for the nRF5340 network core and is meant to be used with LE Audio applications.

This controller is compatible with the HCI RPMsg driver provided by the |NCS| Bluetooth® :ref:`bt_hci_drivers` core.
When the communication between the application and the network core on a device is handled by the HCI RPMsg driver, this link layer controller can be used to handle time critical low level communication and the radio.

.. note::
   The LE Audio controller for nRF5340 is under development and is provided in the :ref:`experimental state <software_maturity>`.
   Make sure to always use the latest version of the controller.

.. le_audio_controller_qdid_start

LE Audio controller for nRF5340 QDID
   The LE Audio controller for nRF5340 associated with this version of the |NCS| comes with the QDID #181316.

.. le_audio_controller_qdid_end

Configuration
*************

The controller can be enabled with the :Kconfig:option:`CONFIG_BT_LL_ACS_NRF53` set to ``y``.

The number of Connected Isochronous Streams (CIS) you can use in a Connected Isochronous Group (CIG) and the number of Broadcast Isochronous Streams (BIS) you can use in a Broadcast Isochronous Group (BIG) are dependent on which configurations you use.
The most relevant configurations are bitrate and number of retransmissions.
See the `Capabilities for the LE Audio Controller Subsystem for nRF5340`_ document for matrix tables listing the supported CIS in a CIG and the supported BIS in a BIG.
These matrix tables are based on the Basic Audio Profile (BAP) tables.

The controller has the following methods of configuration:

* Standard Host-Controller Interface (HCI) commands
* Vendor-Specific commands (VSC)

These are listed in :file:`ble_hci_vsc.h`.
See the :ref:`lib_bt_ll_acs_nrf53_api` for more information.

Usage
*****

This controller is provided as a HEX file and includes the LE Audio Controller Subsystem for nRF53.
Check the license file in the :file:`/bin` directory for usage conditions.

The HEX file is to be used with the |NCS| version it is delivered with.
Other combinations are not tested and their behavior is undefined.

When using the file, the controller is programmed to the network core of nRF5340.
Configurations and calls are handled through standard HCI and VSC.

Samples using the library
*************************

The :ref:`nrf53_audio_app` application uses the controller.

Files
*****

The network core for both gateway and headsets of the :ref:`nrf53_audio_app` is programmed with the precompiled Bluetooth Low Energy Controller binary file :file:`ble5-ctr-rpmsg_<XYZ>.hex`, where *<XYZ>* corresponds to the controller version, for example :file:`ble5-ctr-rpmsg_3216.hex`.
This file includes the LE Audio Controller Subsystem for nRF53.
If :ref:`DFU is enabled <nrf53_audio_app_configuration_configure_fota>`, the subsystem's binary file will be generated in the :file:`build/zephyr/` directory and will be called :file:`net_core_app_signed.hex`.

There are two other files in the :file:`/bin` directory:

* :file:`ble5-ctr-rpmsg_shifted_<XYZ>.hex` which has been shifted to make space for MCUboot.
* :file:`ble5-ctr-rpmsg_shifted_min_<XYZ>.hex` which is the same as above, but made for minimum MCUboot.

Also for these files, *<XYZ>* corresponds to the controller version.

Limitations
***********

The controller is marked as :ref:`experimental <software_maturity>`.

This controller and the LE Audio Controller Subsystem for nRF5340 it includes has been tested and works in configurations used by the :ref:`nrf53_audio_app` application (for example, 2 concurrent CIS, or BIS).
No other configurations than the ones used in the referenced application have been tested or documented for this library.

When you :ref:`build the nRF5340 Audio application with the nRF21540 FEM support <nrf53_audio_app_adding_FEM_support>`, the LE Audio controller for nRF5340 does not support the +20 dBm setting.
This is because of a power class restriction in the controller's QDID.

Dependencies
************

As the controller is provided precompiled in a binary format, there are no software dependencies.
In terms of hardware, this controller is only compatible with the nRF5340.

.. toctree::
    :maxdepth: 1
    :caption: Subpages:

    API_documentation
    CHANGELOG
