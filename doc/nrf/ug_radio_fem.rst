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

Work is underway to make the protocols shipped with |NCS| use FEM.
At the moment, :ref:`ug_thread` and :ref:`ug_zigbee` support the :ref:`nRF21540 DK <nrf21540dk_nrf52840>` and the nRF21540 EK for nRF52 Series devices, but there is no multiprotocol support or support for nRF5340 yet.

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
#. Enable support for MPSL implementation in |NCS| by setting the :option:`CONFIG_MPSL` Kconfig option to ``y``.

.. _ug_radio_fem_nrf21540_gpio:

Adding support for nRF21540 in GPIO mode
****************************************

The nRF21540 device is a range extender that can be used with nRF52 and nRF53 Series devices.
However, software support for nRF21540 is currently available for nRF52 Series devices only.
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
