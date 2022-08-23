.. _ug_matter_overview_architecture_integration:

Matter integration in the |NCS|
###############################

.. contents::
   :local:
   :depth: 2

Matter is included in the |NCS| as one of the submodule repositories managed with the :ref:`zephyr:west` tool, using a dedicated fork ``nrfconnect/sdk-connectedhomeip``.
That is, the code used for the |NCS| and Matter integration is stored in the Matter repository (nRF Connect platform) and is compiled when building one of the available :ref:`matter_samples`.
Both instances depend on each other, but their development is independent to ensure that they both support the latest stable version of one another.
The fork is maintained and verified as a part of the |NCS| release process.

Matter is located on the top application layer of the integration model, looking from the networking point of view.
The |NCS| and Zephyr provide the Bluetooth® LE and Thread stacks, which must be integrated with the Matter stack using a special intermediate layer.
The |NCS|'s Multiprotocol Service Layer (MPSL) driver allows running Bluetooth LE and Thread concurrently on the same radio chip.

.. figure:: images/matter_nrfconnect_overview_simplified_ncs.svg
   :alt: nRF Connect platform in Matter

   nRF Connect platform in Matter

For detailed description, see the :doc:`matter:nrfconnect_platform_overview` page in the Matter documentation.

Matter platform designs (System-on-Chip, multiprotocol)
*******************************************************

Matter in the |NCS| supports the *System-on-Chip, multiprotocol* platform designs, available with the related network stack on Nordic Semiconductor devices in the |NCS|.
For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

Matter over Thread
==================

In this design, the Matter stack, the OpenThread stack and the Bluetooth LE stack run on a single SoC.

This platform design is suitable for the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns

The design differences between the supported SoCs are the following:

* On the nRF5340 SoC, the network core runs both the Bluetooth LE Controller and the 802.15.4 IEEE Radio Driver.
* On the nRF52840 SoC, all components are located on the application core.

.. figure:: /images/thread_platform_design_multi.svg
   :alt: Multiprotocol Thread and Bluetooth LE architecture (nRF52)

   Multiprotocol Thread and Bluetooth LE architecture on nRF52 Series devices

.. figure:: /images/thread_platform_design_nRF53_multi.svg
   :alt: Multiprotocol Thread and Bluetooth LE architecture (nRF53)

   Multiprotocol Thread and Bluetooth LE architecture on nRF53 Series devices

Matter over Wi-Fi
=================

In this design, the Matter stack, the Wi-Fi stack and the Bluetooth LE stack run on a single SoC.

This platform design is suitable for the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf7002dk_nrf5340_cpuapp, nrf5340dk_nrf5340_cpuapp

.. note::
   The ``nrf5340dk_nrf5340_cpuapp`` requires the ``nrf7002_ek`` shield attached for this platform design.
