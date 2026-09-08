.. _building_nrf71:

Building and programming with nRF71 Series
##########################################

.. contents::
   :local:
   :depth: 2

This guide provides instructions on how to build and program the nRF71 Series development kits.
Whether you are working with single or multi-image builds, the following sections will guide you through the necessary steps.

Depending on the sample, you must program only the application core or both the Fast Lightweight Peripheral Processor (FLPR) and the application core.
Additionally, the process will differ based on whether you are working with a single-image or multi-image build.

Building for the application core only
**************************************

Building for the application core only follows the default building process for the |NCS|.
For instructions, see the :ref:`building` page.

Enabling the System OFF service
*******************************

When :ref:`building with Trusted Firmware-M <ug_tfm_building>`, you can build the nRF71 Series devices with the System OFF service enabled.

The System OFF service is one of the :ref:`TF-M platform services <ug_tfm_services_platform>` specific to the |NCS|.
It allows the non-secure application to request the system to enter the System OFF mode using a secure service call.

The System OFF mode is part of the power and clock management system and is available on selected Nordic Semiconductor devices, including the nRF71 Series devices that support TF-M.

To enable the System OFF service in the |NCS|, enable the :kconfig:option:`CONFIG_TFM_NRF_SYSTEM_OFF_SERVICE` Kconfig option.

Zephyr's :zephyr:code-sample:`nrf_system_off` sample demonstrates how to use the System OFF service.
