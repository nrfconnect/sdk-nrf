.. _ug_radio_fem_skyworks:

Enabling support for front-end modules using Simple GPIO interface
##################################################################

.. contents::
   :local:
   :depth: 2

You can use the Skyworks range extenders with nRF52, nRF53 and nRF54L Series devices.
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
            compatible = "skyworks,sky66112-11", "radio-fem-two-ctrl-pins";
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

#. On nRF53 Series devices, apply the same devicetree node mentioned in **Step 1** to the network core using sysbuild.
   To apply the overlay to the correct network core image, create an overlay file named :file:`sysbuild/*image_name*/boards/nrf5340dk_nrf5340_cpunet.overlay` in your application directory, for example :file:`sysbuild/ipc_radio/boards/nrf5340dk_nrf5340_cpunet.overlay`.
   For more information, see the :ref:`Migrating to sysbuild <child_parent_to_sysbuild_migration>` page.

   The *image_name* value is ``ipc_radio``:

   ``ipc_radio`` represents all applications with support for the combination of both 802.15.4 and Bluetooth.
   You can configure your application using the following sysbuild Kconfig options:

   * :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO` for applications having support for 802.15.4, but not for Bluetooth.
   * :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC` for application having support for Bluetooth, but not for 802.15.4.
   * :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO` and :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC` for multiprotocol applications having support for both 802.15.4 and Bluetooth.

   .. note::
       This step is not needed when testing with the :ref:`direct_test_mode` or :ref:`radio_test` samples on nRF53 Series devices.

#. On nRF54L Series devices, make sure the GPIO pins of the SoC selected to control ``ctx-gpios`` and ``crx-gpios`` support GPIOTE.
   For example, on the nRF54L15 device, use pins belonging to GPIO P1 or GPIO P0 only.
   You cannot use the GPIO P2 pins, because there is no related GPIOTE peripheral.
   It is recommended to use the GPIO pins that belong to the PERI Power Domain of the nRF54L device.
   For example, on the nRF54L15, these are pins belonging to GPIO P1.
   Using pins belonging to Low Power Domain (GPIO P0 on nRF54L15) is supported but requires more DPPI and PPIB channels of the SoC.
   Ensure that the following devicetree instances are enabled (have ``status = "okay"``):

   * ``dppic10``
   * ``dppic20``
   * ``dppic30``
   * ``ppib11``
   * ``ppib21``
   * ``ppib22``
   * ``ppib30``

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
