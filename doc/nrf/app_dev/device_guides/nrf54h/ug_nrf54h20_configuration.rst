.. _ug_nrf54h20_configuration:

Configuring the nRF54H20 SoC
############################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC uses both devicetree and Kconfig for its hardware and software configuration.

You can find the basics of both devicetree and Kconfig in Zephyr in the :ref:`zephyr:application` section of the Zephyr documentation.
You can also read the :ref:`app_build_system` section describing the |NCS| additions to the Zephyr configuration system.

However, the multicore nature of the nRF54H20 SoC required some changes to how devicetree files are organized.

Customizing the DTS configuration
*********************************

The configuration details for the nRF54H20 SoC are located under the :file:`soc/common/nordic` directory or the specific board directory itself.

You can find documentation for the output files created in your application build directory in :ref:`zephyr:devicetree-in-out-files`.

To customize this configuration, you can use overlay files.
For more information, see the following resources:

* The :ref:`configuration_and_build` page
* *Lesson 3* of the `nRF Connect SDK Fundamentals course`_ on the Nordic Developer Academy website

Generated HEX files
*******************

When building an application for the nRF54H20 SoC, you are building all domain images at once.
This process generates the following :file:`zephyr.hex` images:

* Application core application
* PPR core application
* Radio core firmware
* User Information Configuration Registers (UICR) contents

You can use the UICR to specify the device-wide configuration needed by the firmware.
See :ref:`ug_nrf54h20_ironside_se_uicr_image` for a description of the UICR generation process.

You must flash all the HEX files into the device.

For more information on building in the |NCS|, see the :ref:`configuration_and_build` page.
For an example of how to build and program images on the nRF54H20 DK, see :ref:`ug_nrf54h20_gs`.
