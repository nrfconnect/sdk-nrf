.. _ug_radio_fem_nrf21540_gpio:

Enabling GPIO mode support for nRF21540
#######################################

The `nRF21540`_ device is a range extender that you can use with nRF52 and nRF53 Series devices.
The nRF21540 GPIO mode implementation of FEM is compatible with the nRF21540 device and implements the 3-pin PA/LNA interface.

.. ncs_implementation_desc_start

The |NCS| provides code that configures FEM based on devicetree (DTS) and Kconfig information using the :ref:`nrfxlib:mpsl` (MPSL) library.
The FEM hardware description in the application's devicetree file is an essential part of the configuration.
To enable FEM support, an ``nrf_radio_fem`` node must be present in the application's devicetree file.
The node can be provided by the devicetree file of the target board, by an overlay file, or through the :makevar:`SHIELD` CMake variable (see :ref:`cmake_options`).
See :ref:`zephyr:dt-guide` for more information about the DTS data structure, and :ref:`zephyr:dt_vs_kconfig` for information about differences between DTS and Kconfig.

.. ncs_implementation_desc_end

.. note::
    - See also :ref:`ug_radio_fem_optional_properties` when enabling support for nRF21540.
    - In the naming convention used in the API of the MPSL library, the functionalities designated as ``PA`` and ``LNA`` apply to the ``tx-en-gpios`` and ``rx-en-gpios`` pins listed below, respectively.

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
   *  ``hci_ipc`` for application having support for Bluetooth, but not for 802.15.4.

   .. note::
       This step is not needed when testing with :ref:`direct_test_mode` and :ref:`radio_test` on the nRF53 Series devices.
