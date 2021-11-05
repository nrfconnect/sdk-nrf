.. _ug_radio_fem:

Radio front-end module (FEM) support
####################################

.. contents::
   :local:
   :depth: 2

This guide describes how to add support for 2 different front-end module (FEM) implementations to your application in |NCS|.

|NCS| allows you to extend the radio range of your development kit with an implementation of the front-end modules.
Front-end modules are range extenders used for boosting the link robustness and link budget of wireless SoCs.

The FEM support is based on the :ref:`nrfxlib:mpsl_fem`, which is integrated into the nrfxlib's MPSL library.
This library provides nRF21540 GPIO and Simple GPIO implementations, for 3-pin and 2-pin PA/LNA interfaces, respectively.

The implementations described in this guide are the following:

* :ref:`ug_radio_fem_nrf21540_gpio` - For the nRF21540 GPIO implementation that uses nRF21540.
* :ref:`ug_radio_fem_skyworks` - For the Simple GPIO implementation that uses the SKY66112-11 device.

These methods are only available to protocol drivers that are using FEM features provided by the MPSL library in multiprotocol scenarios.
They are also valid for cases where an application uses just one protocol, but benefits from features provided by MPSL.
To avoid conflicts, check the protocol documentation to see if it uses FEM support provided by MPSL.

|NCS| provides a friendly wrapper that configures FEM based on devicetree (DTS) and Kconfig information.
To enable FEM support, you must enable FEM and MPSL, and add an ``nrf_radio_fem`` node in the devicetree file.
The node can also be provided by the devicetree file of the target devkit or by an overlay file.
See :ref:`zephyr:dt-guide` for more information about the DTS data structure, and :ref:`zephyr:dt_vs_kconfig` for information about differences between DTS and Kconfig.

.. _ug_radio_fem_requirements:

Enabling FEM and MPSL
*********************

Before you add the devicetree node in your application, complete the following steps:

1. Add support for the MPSL library in your application.
   The MPSL library provides API to configure FEM.
   See :ref:`nrfxlib:mpsl_lib` in the nrfxlib documentation for details.
#. Enable support for MPSL implementation in |NCS| by setting the :kconfig:`CONFIG_MPSL` Kconfig option to ``y``.

.. _ug_radio_fem_nrf21540_gpio:

Adding support for nRF21540 in GPIO mode
****************************************

The nRF21540 device is a range extender that can be used with nRF52 and nRF53 Series devices.
For more information about nRF21540, see the `nRF21540`_ documentation.

The nRF21540 GPIO mode implementation of FEM is compatible with this device and implements the 3-pin PA/LNA interface.

.. note::
  In the naming convention used in the API of the MPSL library, the functionalities designated as ``PA`` and ``LNA`` apply to the ``tx-en-gpios`` and ``rx-en-gpios`` pins listed below, respectively.

To use nRF21540 in GPIO mode, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
            nrf_radio_fem: name_of_fem_node {
               compatible  = "nordic,nrf21540-fem";
               tx-en-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
               rx-en-gpios = <&gpio0 14 GPIO_ACTIVE_HIGH>;
               pdn-gpios   = <&gpio0 15 GPIO_ACTIVE_HIGH>;
         };
      };

#. Optionally replace the node name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``tx-en-gpios`` - GPIO characteristic of the device that controls the ``TX_EN`` signal of nRF21540.
   * ``rx-en-gpios`` - GPIO characteristic of the device that controls the ``RX_EN`` signal of nRF21540.
   * ``pdn-gpios`` - GPIO characteristic of the device that controls the ``PDN`` signal of nRF21540.

   These properties correspond to ``TX_EN``, ``RX_EN``, and ``PDN`` pins of nRF21540 that are supported by software FEM.

   Type ``phandle-array`` is used here, which is common in Zephyr's devicetree to describe GPIO signals.
   The first element ``&gpio0`` refers to the GPIO port ("port 0" has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for nRF21540, but for a different FEM module you can use ``GPIO_ACTIVE_LOW``.

   The state of the remaining control pins should be set in other ways and according to `nRF21540 Product Specification`_.

#. On nRF53 devices, you must also apply the same devicetree node mentioned in step 1 to the network core.
   To do so, apply the overlay to the correct network core child image by creating an overlay file named :file:`child_image/*childImageName*.overlay` in your application directory, for example :file:`child_image/multiprotocol_rpmsg.overlay`.

   The ``*childImageName*`` string must be one of the following values:

   *  ``multiprotocol_rpmsg`` for multiprotocol applications having support for both 802.15.4 and Bluetooth.
   *  ``802154_rpmsg`` for applications having support for 802.15.4, but not for Bluetooth.
   *  ``hci_rpmsg`` for application having support for Bluetooth, but not for 802.15.4.

Optional properties
===================

The following properties are optional and can be added to the devicetree node if needed:

* Properties that control the timing of interface signals:

  * ``tx-en-settle-time-us`` - Minimal time interval between asserting the ``TX_EN`` signal and starting the radio transmission, in microseconds.
  * ``rx-en-settle-time-us`` - Minimal time interval between asserting the ``RX_EN`` signal and starting the radio transmission, in microseconds.

    .. important::
        Values for these two properties cannot be higher than the Radio Ramp-Up time defined by :c:macro:`TX_RAMP_UP_TIME` and :c:macro:`RX_RAMP_UP_TIME`.
        If the value is too high, the radio driver will not work properly and will not control FEM.
        Moreover, setting a value that is lower than the default value can cause disturbances in the radio transmission, because FEM may be triggered too late.

  * ``pdn-settle-time-us`` - Time interval before the PA or LNA activation reserved for the FEM ramp-up, in microseconds.
  * ``trx-hold-time-us`` - Time interval for which the FEM is kept powered up after the event that triggers the PDN deactivation, in microseconds.

  The default values of these properties are appropriate for default hardware and most use cases.
  You can override them if you need additional capacitors, for example when using custom hardware.
  In such cases, add the property name under the required properties in the devicetree node and set a new custom value.

  .. note::
    These values have some constraints.
    For details, see `nRF21540 Product Specification`_.

.. _ug_radio_fem_skyworks:

Adding support for SKY66112-11
******************************

SKY66112-11 is one of many FEM devices that support the 2-pin PA/LNA interface.
Currently, only the nRF52 Series devices support this interface.
nRF53 Series devices are not supported.

.. note::
  In the naming convention used in the API of the MPSL library, the functionalities designated as ``PA`` and ``LNA`` apply to the ``ctx-gpios`` and ``crx-gpios`` pins listed below, respectively.

To use the Simple GPIO implementation of FEM with SKY66112-11, complete the following steps:

1. Add the following node in the devicetree file:

   .. code-block::

      / {
         nrf_radio_fem: name_of_fem_node {
            compatible = "skyworks,sky66112-11", "generic-fem-two-ctrl-pins";
            ctx-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
            crx-gpios = <&gpio0 14 GPIO_ACTIVE_HIGH>;
         };
      };

#. Optionally replace the node name ``name_of_fem_node``.
#. Replace the pin numbers provided for each of the required properties:

   * ``ctx-gpios`` - GPIO characteristic of the device that controls the ``CTX`` signal of SKY66112-11.
   * ``crx-gpios`` - GPIO characteristic of the device that controls the ``CRX`` signal of SKY66112-11.

   These properties correspond to ``CTX`` and ``CRX`` pins of SKY66112-11 that are supported by software FEM.

   Type ``phandle-array`` is used here, which is common in Zephyr's devicetree to describe GPIO signals.
   The first element ``&gpio0`` refers to the GPIO port ("port 0" has been selected in the example shown).
   The second element is the pin number on that port.
   The last element must be ``GPIO_ACTIVE_HIGH`` for SKY66112-11, but for a different FEM module you can use ``GPIO_ACTIVE_LOW``.

   The state of the other control pins should be set according to the SKY66112-11 documentation.
   See the official `SKY66112-11 page`_ for more information.

Optional properties
===================

The following properties are optional and can be added to the devicetree node if needed:

* Properties that control the timing of interface signals:

  * ``ctx-settle-time-us`` - Minimal time interval between asserting the ``CTX`` signal and starting the radio transmission, in microseconds.
  * ``crx-settle-time-us`` - Minimal time interval between asserting the ``CRX`` signal and starting the radio transmission, in microseconds.

  The default values of these properties are appropriate for default hardware and most use cases.
  You can override them if you need additional capacitors, for example when using custom hardware.
  In such cases, add the property name under the required properties in the devicetree node and set a new custom value.

  .. note::
    These values have some constraints.
    For details, see the official documentation at the `SKY66112-11 page`_.

* Properties that inform protocol drivers about gains provided by SKY66112-11:

  * ``tx-gain-db`` - Transmission gain value in dB.
  * ``rx-gain-db`` - Reception gain value in dB.

  The default values are accurate for SKY66112-11 but can be overridden when using a similar device with a different gain.

.. _ug_radio_fem_incomplete_connections:

Use case of incomplete physical connections to the FEM module
*************************************************************

The method of configuring FEM using the devicetree file allows you to opt out of using some pins.
For example if power consumption is not critical, the nRF21540 module PDN pin can be connected to a fixed logic level.
Then there is no need to define a GPIO to control the PDN signal. The line ``pdn-gpios = < .. >;`` can then be removed from the devicetree file.

Generally, if pin ``X`` is not used, the ``X-gpios = < .. >;`` property can be removed.
This applies to all properties with a ``-gpios`` suffix, for both nRF21540 and SKY66112-11.

.. _ug_radio_fem_boards:

Supported boards
****************

Two nRF21540 boards are available, showcasing the possibilities of the nRF21540 FEM:

* :ref:`nRF21540 DK <nrf21540dk_nrf52840>`
* nRF21540 EK, described in sections below

The front-end module feature is supported on the nRF52 and nRF53 Series devices.

.. _ug_radio_fem_nrf21540_ek:

nRF21540 EK
===========

The nRF21540 EK (Evaluation Kit) is an RF front-end module (FEM) for Bluetooth Low Energy, Bluetooth mesh, 2.4 GHz proprietary, Thread, and Zigbee range extension.
When combined with an nRF52 or nRF53 Series SoC, the nRF21540 RF FEM’s +21 dBm TX output power and 13 dB RX gain ensure a superior link budget for up to 16x range extension.

Overview
--------

The nRF21540 complementary device has a 50 Ω SMA transceiver interface and 2x 50 Ω SMA antenna interfaces.
This enables connecting an SoC or a signal generator to the input.
It also enables connecting the outputs to measurement tools or to antennas directly.
The FEM can be configured through the pins available on the Arduino headers.

The nRF21540's gain control, antenna switching, and modes are controlled using GPIO or SPI, or a combination of both.
GPIO and SPI are accessible through the Arduino Uno Rev3 compatible headers.
The shield also features two additional SMA connectors hooked to the dual antenna ports from the RF FEM, to monitor the performance of the RF FEM using any equipment desired.
The FEM SMA input can be connected to the nRF52 or nRF53 Series SoC RF output with a coaxial RF cable with SMA\SWF connectors.

.. figure:: /images/nrf21540_ek.png
   :width: 350px
   :align: center
   :alt: nRF21540_EK

   nRF21540 EK shield

Pin assignment of the nRF21540 EK
---------------------------------

+-----------------------+----------+-----------------+
| Shield connector pin  | SIGNAL   | FEM function    |
+=======================+==========+=================+
| D2                    | GPIO     | Mode Select     |
+-----------------------+----------+-----------------+
| D3                    | GPIO     | RX Enable       |
+-----------------------+----------+-----------------+
| D4                    | GPIO     | Antenna Select  |
+-----------------------+----------+-----------------+
| D5                    | GPIO     | TX Enable       |
+-----------------------+----------+-----------------+
| D9                    | GPIO     | Power Down      |
+-----------------------+----------+-----------------+
| D10                   | SPI CS   | Chip Select     |
+-----------------------+----------+-----------------+
| D11                   | SPI MOSI | Serial Data In  |
+-----------------------+----------+-----------------+
| D12                   | SPI MISO | Serial Data Out |
+-----------------------+----------+-----------------+
| D13                   | SPI SCK  | Serial Clock    |
+-----------------------+----------+-----------------+

.. _ug_radio_fem_nrf21540_ek_programming:

Programming
-----------

Set ``-DSHIELD=nrf21540_ek`` when you invoke ``west build`` or ``cmake`` in your Zephyr application.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file:

.. code-block:: none

	set(SHIELD nrf21540_ek)

To build with SES, in the :guilabel:`Extended Settings` specify ``-DSHIELD=nrf21540_ek``.
See :ref:`cmake_options`.

When building for a board with an additional network core, for example nRF5340, add an additional ``-DSHIELD`` variable with the *childImageName_* parameter between ``-D`` and ``SHIELD`` to build for the network core as well.
For example:

.. parsed-literal::
   :class: highlight

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf21540_ek -Dmultiprotocol_rpmsg_SHIELD=nrf21540_ek

In this command, the *childImageName_* parameter has the ``multiprotocol_rpmsg_`` value and builds a multiprotocol application with support for 802.15.4 and Bluetooth.
The *childImageName_* parameter can take the following values:

*  ``multiprotocol_rpmsg_`` for multiprotocol applications with support for 802.15.4 and Bluetooth
*  ``802154_rpmsg_`` for applications with support for 802.15.4, but without support for Bluetooth
*  ``hci_rpmsg_`` for application with support for Bluetooth, but without support for 802.15.4

References
----------

* `nRF21540 DK product page`_
* `nRF21540 Product Specification`_
* `nRF21540`_
