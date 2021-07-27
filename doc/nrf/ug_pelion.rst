.. _ug_pelion:

Using Pelion with the |NCS|
###########################

.. contents::
   :local:
   :depth: 2

Pelion is an IoT cloud platform that offers services in the fields of connectivity, device management, and edge computing.
It allows you to remotely manage and update your IoT devices.
The |NCS| provides Pelion's ``mbed-cloud-client`` library (the Pelion Device Management library) as one of the external submodule repositories managed with :ref:`zephyr:west`.
This library is fetched into the :file:`ncs/modules/lib/pelion-dm` directory of the |NCS|.
Enabling the library allows you to connect your devices to Pelion Device Management and exchange data with it using either LTE or OpenThread.

To read more about Pelion, see the `Pelion website`_ and the `Pelion Device Management documentation`_.

An example of an application that is using the Pelion Device Management library in the |NCS| is the :ref:`pelion_client`.

Setting up dependencies
***********************

For the library to work with your device, complete the following configuration steps:

1. `Create Pelion developer account`_.
#. `Enable and configure Mbed TLS`_.
#. `Provision the device with credentials`_, required for the connection to Pelion Device Management Portal.
#. `Provision the device with update resources`_ for the firmware update procedure to work.
#. `Create a dedicated partition on flash`_ for the Pelion Device Management library
#. `Configure dynamic allocation memory`_ with sufficient space for heap.
#. `Enable and configure the network backend`_.

See the following sections for details.

Create Pelion developer account
===============================

To use the Pelion Device Management library, you need a developer account on `Pelion Device Management Portal`_.

Enable and configure Mbed TLS
=============================

The Pelion Device Management library depends on a properly configured Mbed TLS library.
To simplify the development within the |NCS|, you can use a predefined set of Mbed TLS configuration options.

To enable the set that is compatible with :ref:`nrfxlib:nrf_security`, use the :kconfig:`CONFIG_PELION_NRF_SECURITY` Kconfig option.

Make sure to properly configure the memory region used by Mbed TLS library for dynamic allocations.
Memory for this library is allocated from either the libc heap or a dedicated buffer, depending on the configuration.
For more information about using the dedicated buffer for Mbed TLS heap, see help for the :kconfig:`CONFIG_MBEDTLS_ENABLE_HEAP` and :kconfig:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig options.
If :kconfig:`CONFIG_MBEDTLS_ENABLE_HEAP` is disabled and Mbed TLS configuration files do not add any overrides, the libc heap is used.

Provision the device with credentials
=====================================

You must provision the device before it can connect to Pelion Device Management Portal.

When using Pelion, you can complete the provisioning process using either a production tool (Factory Configurator Utility for factory provisioning) or the developer flow process.
For more information about the provisioning process, see `Provisioning devices for Pelion Device Management`_ in the Pelion documentation.

Provision the device with update resources
==========================================

For details about provisioning the device with update resources, see the `Device Management Update`_ guide in the Pelion documentation.

.. note::
   Zephyr's port of Pelion might not support the most recent update client features.
   The Device Management Update link points to the documentation of the version compatible with the |NCS|.
   For details, refer to the `Pelion Device Management documentation`_ and `Pelion Device Management release notes`_.

Create a dedicated partition on flash
=====================================

The Pelion Device Management library requires a partition named ``pelion_storage`` to be defined on flash.
The partition is used to store the non-volatile data, such as credentials and identifiers.
Because this kind of data is sensitive, keep the partition on the SoC internal flash or make sure to secure the access to the memory.

Configure dynamic allocation memory
===================================

The Pelion Device Management library depends on the new C standard library (newlib).
In case of targets without the memory management unit (MMU), given that the application is not running in the Userspace, the portion of RAM that remains unallocated for any other purpose is used by the libc heap.
Make sure to leave enough space for the heap, so that the Pelion Device Management library works correctly.

For more information about the new C standard library, see Zephyr's :ref:`libc_api`.

Enable and configure the network backend
========================================

The Pelion Device Management library uses standard POSIX sockets for the network communication.
The library was tested with the following network backends:

* Cellular (LTE, NB-IoT)
* OpenThread

After initializing and setting up the Pelion object instance, it continuously retries to connect to Pelion's Device Management server.

Cellular backend
----------------

The cellular backend uses of the on-board modem that is part of the nRF9160 SiP.
For more information about working with the modem, see the :ref:`ug_nrf9160` documentation.

To simplify the connection setup, you can use the :ref:`lte_lc_readme`.

OpenThread backend
------------------

Thread is a low-power mesh networking technology.
It allows a device to access the Internet if one of the mesh elements (boarder router) share the Internet connection.

For more information, see :ref:`zephyr:thread_protocol_interface` in the Zephyr documentation and :ref:`ug_thread` in the |NCS| documentation.

Pelion configuration
********************

To enable the Pelion Device Management library in the |NCS|, use the :kconfig:`CONFIG_PELION_CLIENT` Kconfig option.

You can control the Pelion Device Management library features using Kconfig options that are defined within the Pelion Device Management library repository.
The following options are among the most important ones:

* :kconfig:`CONFIG_PELION_UPDATE` - This option enables the device firmware update (DFU) feature.
* :kconfig:`CONFIG_PELION_TRANSPORT_MODE_TCP`, :kconfig:`CONFIG_PELION_TRANSPORT_MODE_UDP`, :kconfig:`CONFIG_PELION_TRANSPORT_MODE_UDP_QUEUE` - These options select the transport protocol used by the library.

To see all options, check the Pelion Device Management library subtree in configuration system (menuconfig) or read the `Zephyr integration tutorial`_ in the Pelion documentation.
