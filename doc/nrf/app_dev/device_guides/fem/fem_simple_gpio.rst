.. _ug_radio_fem_skyworks:

Enabling support for front-end modules using Simple GPIO interface
##################################################################

.. contents::
   :local:
   :depth: 2

You can use the Skyworks range extenders with nRF52 and nRF53 Series devices.
SKY66112-11 is one of many FEM devices that support the 2-pin PA/LNA interface.
You can use SKY66112-11 as an example on how to create bindings for different devices that support the 2-pin PA/LNA interface.

.. include:: fem_nrf21540_gpio.rst
    :start-after: ncs_implementation_desc_start
    :end-before: ncs_implementation_desc_end

The |NCS| provides also devicetree bindings for the SKY66114-11 and SKY66403-11.
For more details about devicetree binding, see: :ref:`Zephyr documentation <zephyr:dt-bindings>`.

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
#. On nRF53 devices, you must also apply the same devicetree node to the network core.

   Create a devicetree overlay file with the same information as you used in Steps 1 to 3.
   To apply the overlay to the correct network core child image, create the file in the :file:`child image` directory of your application directory, and name it :file:`*childImageName*.overlay`, for example :file:`child_image/multiprotocol_rpmsg.overlay`.
   The ``*childImageName*`` string must be one of the following values:

   *  ``multiprotocol_rpmsg`` for multiprotocol applications with support for 802.15.4 and Bluetooth.
   *  ``802154_rpmsg`` for applications with support for 802.15.4, but without support for Bluetooth.
   *  ``hci_ipc`` for applications with support for Bluetooth, but without support for 802.15.4.

   .. note::
       This step is not needed when testing with the :ref:`direct_test_mode` or :ref:`radio_test` samples on nRF53 Series devices.

Optional FEM properties for simple GPIO
***************************************

The following properties are optional for use with the simple GPIO implementation:

* Properties that control the other pins:

   * csd-gpios - GPIO characteristic of the device that controls the CSD signal of SKY66112-11.
   * cps-gpios - GPIO characteristic of the device that controls the CPS signal of SKY66112-11.
   * chl-gpios - GPIO characteristic of the device that controls the CHL signal of SKY66112-11.
   * ant-sel-gpios - GPIO characteristic of the device that controls the ANT_SEL signal of devices that support antenna diversity, for example SKY66403-11.

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

.. _ug_radio_fem_sw_support_mpsl_fem_output_simple_gpio:

Setting the FEM output power for simple GPIO
============================================

The ``tx_gain_db`` property in devicetree provides the FEM gain value to use with the simple GPIO FEM implementation.
The property must represent the real gain of the FEM.
This implementation does not support controlling the gain value during runtime.
