.. _ug_matter_gs_kconfig:
.. _ug_matter_configuring_protocol:

Enabling Matter in Kconfig
##########################

.. contents::
   :local:
   :depth: 2

To use the Matter protocol, set the :kconfig:option:`CONFIG_CHIP` Kconfig option.
This option enables the Matter protocol stack and other associated features and components, such as C++ support or Zephyr networking layer.

For instructions about how to set Kconfig options, see :ref:`configure_application`.
For a list of advanced Matter Kconfig options, see :ref:`ug_matter_device_advanced_kconfigs`.

Kconfig option structure
************************

The Kconfig options for Matter applications in the |NCS| are stored in the following files:

* :file:`prj.conf` files, which are specific to the application.
* :file:`Kconfig.defaults` file, which is available in the :file:`module/lib/matter/config/nrfconnect/chip-module` directory and is used to populate :file:`prj.conf` with Kconfig option settings common to all samples.

For an example configuration, see the :ref:`Matter Template sample's <matter_template_sample>` :file:`prj.conf` files in the sample root directory.

Configuration options for other modules
***************************************

Because Matter is an application layer protocol on top of the other IPv6-based transport protocols (see :ref:`ug_matter_architecture`), it uses multiple software modules with their own configuration options to provide the communication between the devices and the necessary functionalities.
It uses modules such as Bluetooth® LE, the IPv6 stack (for example :ref:`Thread <ug_thread_configuring>`), :ref:`nRF Security <nrf_security_config>`, or :ref:`MCUboot <mcuboot:mcuboot_ncs>`.
Make sure to review the configuration options of these modules when configuring Matter.
