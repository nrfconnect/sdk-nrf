.. _ug_nrf54h20_configuration:

Configuring the nRF54H20 DK
###########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 DK uses both devicetree and Kconfig for its hardware and software configuration.

You can find the basics of both devicetree and Kconfig in Zephyr in the :ref:`zephyr:application` section of the Zephyr documentation.
You can also read the :ref:`app_build_system` section describing the |NCS| additions to the Zephyr configuration system.

However, the multicore nature of the nRF54H20 DK required some changes to how devicetree files are organized.

Customizing the DTS configuration
*********************************

The configuration details for the nRF54H20 DK are located under the :file:`soc/common/nordic` directory or the specific board directory itself.

You can find documentation for the output files created in your application build directory in :ref:`zephyr:devicetree-in-out-files`.
You can use overlay files to customize this configuration.

To see and test how to use overlays for changing nodes, see the *Lesson 3* of the `nRF Connect SDK Fundamentals course`_ on the Nordic Developer Academy website.

Generated HEX files
*******************

When building an application for the nRF54H20 DK, you are building all domain images at once.
This process generates the following :file:`zephyr.hex` images:

* Application core application
* PPR core application
* Radio core firmware

Additionally, the build process generates the following user information configuration registers (UICR) contents (:file:`uicr.hex`) to set up access for domains:

* System Controller UICR
* Application UICR
* Radio UICR

.. note::
   ``west flash`` uses :file:`uicr_merged.hex` files that are pre-merged HEX files combining the relevant :file:`zephyr.hex` + :file:`uicr.hex` for a domain that has UICRs.
   Flashing both :file:`zephyr.hex` + :file:`uicr.hex` will result in the same configuration.

You must flash all the HEX files into the device.
For more information on building images for the nRF54H20 DK, see :ref:`ug_nrf54h20_gs`.
