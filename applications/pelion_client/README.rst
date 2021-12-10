.. _pelion_client:

nRF Pelion Client
#################

.. contents::
   :local:
   :depth: 2

The nRF Pelion Client application demonstrates the integration of the Pelion Device Management library within the |NCS|.
The application constructs Open Mobile Alliance (OMA) objects, connects to the Pelion Device Management platform, and sends data to the web server over either LTE or OpenThread.

Pelion is an IoT cloud platform that offers services in the fields of connectivity, device management, and edge computing.
It allows you to remotely manage and update your IoT devices.
To read more, see the `Pelion website`_ and the `Pelion Device Management documentation`_.

.. note::
   |pelion_lib_explanation|
   Later on this page, this library is referred to as *Pelion Device Management library*.

Overview
********

The application integrates `Pelion Device Management`_ features and is based on the `Pelion Device Management Client library reference example`_.
It establishes a secure connection with the Pelion Device Management, starts the Pelion Device Management library, and initializes the Pelion Device Management Client.
The Device Management Client creates standard OMA objects that you can interact with from `Pelion Device Management Portal`_.
It then communicates with the network using sockets.

.. _pelion_client_device_provisioning:

Device provisioning
===================

You must provision the device before it can connect with Pelion Device Management Portal.

When using Pelion, you can complete the provisioning process using either a production tool (Factory Configurator Utility for factory provisioning) or the developer flow process.
For more information about the provisioning process, see `Provisioning devices for Pelion Device Management`_ in the Pelion documentation.

The application follows the developer flow provisioning process.
This process involves downloading a source file that contains a valid certificate.
The file is available from `Pelion Device Management Portal`_ and is used at build time to embed the required information to the application binary.

Upon first boot, or whenever the Pelion storage partition is erased (:c:func:`fcc_storage_delete`), the storage is populated with the credentials passed at build time.
The application does this by calling :c:func:`fcc_developer_flow` right after the Factory Configurator Client (FCC) is initialized (:c:func:`fcc_init`).
For more information about these functions, see the `Pelion's factory_configurator_client.h`_ file reference page.

Using a valid certificate allows the device to successfully register (connect) to Pelion Device Management Portal.
At this stage, the device is assigned a new device identity.

.. note::
   |factory_reset_note|

Run time flow overview
======================

The following sections describe the application operational flow at run time, after provisioning:

* `Factory Configuration Client initialization`_
* `Network back end initialization`_
* `Pelion Device Management library initialization`_
* `OMA object creation`_
* `Pelion setup finalization`_
* `Network communication`_

Factory Configuration Client initialization
-------------------------------------------

After the board boot sequence, the application's ``pelion_fcc`` module initializes Pelion's FCC.
At this stage, the developer flow is enabled and the certificate is loaded to the Pelion storage partition (if needed).

Network back end initialization
-------------------------------

In parallel to the FCC initialization, the application initializes the network back end.
The appropriate net module connects the required callbacks and starts either OpenThread (``net_ot``, using Zephyr's :ref:`zephyr:thread_protocol_interface`) or the link control (``net_lte``, using the :ref:`nrf_modem_lib_readme`).

The network module tracks the connection state and informs the rest of the application if the connection is established or the network is disconnected.

Pelion Device Management library initialization
-----------------------------------------------

The Device Management Client object is initialized from the Pelion Device Management library when the FCC is initialized.
The library attaches several client callbacks that are required for informing the application about the connection status with Pelion Device Management Portal.

OMA object creation
-------------------

After the Device Management Client is initialized by the application, it sends ``create_object_event``, which starts the creation of OMA objects, object instances, and object resources for each instance.
Each module used by the application completes this operation using the Pelion Device Management library API.
The nRF Pelion Client application creates objects of two types: OMA Digital Input and OMA Stopwatch.
You can find the code for handling these in the :file:`oma_digital_input.cpp` and :file:`oma_stopwatch.cpp` files, respectively, located in the :file:`src/modules` directory.
When all resources are created, they are added to the Device Management Client.

Pelion setup finalization
-------------------------

The Device Management Client uses standard network sockets for communication.
To avoid unnecessary pinging to servers on the network, the client setup is performed only after the network connection is established.
When information about the network back end connection is broadcast by the network module, the Pelion module continues with setting up the Device Management Client.

Network communication
---------------------

The Device Management Client starts communicating with the servers, either over LTE or OpenThread.
The device registers with Pelion Device Management Portal, if a valid certificate was provided at build time.
Information about all resources added to the Device Management Client is passed to Pelion Device Management Portal.

This completes the run time operational flow of the device.
You can now locate the device in the device directory of the Device Management and start interacting with it.
See `User interface`_ and `Testing`_ sections for more information about interaction options.

.. _pelion_client_internal_modules:

Firmware architecture
=====================

To allow greater flexibility for future modifications, the application has a modular structure, where each module has a defined scope of responsibility.
The application uses the :ref:`event_manager` to distribute events between modules in the system.

The following table lists modules that are part of the application.

+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| Module                        | Related application events                                                                                    | Comment                                                                                                                                |
+===============================+===============================================================================================================+========================================================================================================================================+
| ``pelion``                    | Sends ``pelion_state_event`` and ``pelion_create_objects_event``.                                             | Brings up the Device Management Client object and interacts with it. Monitors the connection state to the Device Management Portal.    |
|                               | Reacts to ``net_state_event``.                                                                                |                                                                                                                                        |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``pelion_fcc``                | Reacts to ``factory_reset_event``.                                                                            | Initializes Pelion's Factory Configurator Client (FCC) and initiates the developer flow mode.                                          |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``pelion_trace``              | None.                                                                                                         | If enabled, it initializes debug traces in the Pelion Device Management library.                                                       |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``net_ot``                    | Send ``net_state_event``.                                                                                     | Initialize the network back end and track the network connection state.                                                                |
| ``net_lte``                   | React to ``factory_reset_event``.                                                                             |                                                                                                                                        |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``oma_digital_input``         | Reacts to ``pelion_create_objects_event``, ``button_event``, ``net_state_event``, and ``pelion_state_event``. | Registers the respective Open Mobile Alliance object resources, which work as an example of the device communication with the cloud.   |
|                               |                                                                                                               | This resource enables buttons.                                                                                                         |
|                               |                                                                                                               | When starting the board, the firmware registers four buttons in Pelion Device Management Portal.                                       |
|                               |                                                                                                               | It then registers the status of the button (pushed or unpushed) and the number of pushes.                                              |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``oma_stopwatch``             | Reacts to ``pelion_create_objects_event``, ``net_state_event``, and  ``pelion_state_event``.                  | Registers the respective Open Mobile Alliance object resources, which work as an example of the device communication with the cloud.   |
|                               |                                                                                                               | This resource counts time, increasing it periodically.                                                                                 |
|                               |                                                                                                               | You can configure the period and reset the time count.                                                                                 |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``led_state``                 | Sends ``led_event``. Reacts to ``net_state_event`` and ``pelion_state_event``.                                | Reflects the connection state onto LEDs.                                                                                               |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``power_manager``             | Sends ``power_down_event``.                                                                                   | Switches the device into the low power mode when no interaction is detected for a given period of time.                                |
|                               | Reacts to ``keep_alive_event`` and ``wake_up_event``.                                                         |                                                                                                                                        |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``dfu``                       | Reacts to ``pelion_state_event``.                                                                             | Confirms that the DFU image is working fine after the update process.                                                                  |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+
| ``factory_reset_request``     | Sends ``factory_reset_event``.                                                                                | Checks if the factory reset is requested by the user.                                                                                  |
|                               | Reacts to ``button_event``.                                                                                   |                                                                                                                                        |
+-------------------------------+---------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------+

All modules react to ``module_state_event``.

On top of these modules, the application uses the following modules of the :ref:`lib_caf` (CAF) libraries, a set of generic modules based on the Event Manager and available to all applications in the |NCS|:

* :ref:`caf_buttons`
* :ref:`caf_leds`

The following figure shows the application architecture.
The figure visualizes relations between the Event Manager, modules, drivers, and libraries.

.. figure:: /images/pelion_client_architecture.svg
    :alt: Module hierarchy

    Relationship between modules and the Event Manager

.. _pelion_client_memory_allocation:

Memory allocation
=================

The application allocates memory dynamically for the following features:

* Event Manager events - Memory for these events is allocated from Zephyr's :ref:`system heap <zephyr:heap_v2>`.
* Pelion Device Management library - Memory for this library is allocated from the libc heap.
* Mbed TLS - Memory for this library is allocated from either the libc heap or a dedicated buffer.

Zephyr's system heap
--------------------

Zephyr contains its own memory allocation algorithm that implements heap.
To configure the heap memory size, use the :kconfig:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option.

For more information about Zephyr's system heap, see the :ref:`zephyr:heap_v2` page in Zephyr's documentation.

Libc heap
---------

The Pelion Device Management library depends on the new libc.
In case of targets without the memory management unit (MMU), given that the application is not running in the Userspace, the portion of RAM that remains unallocated for any other purpose is used by the libc heap.

Mbed TLS allocation buffer
--------------------------

The Mbed TLS library can use the libc heap for allocation.

Alternatively, it can use the dedicated allocation area (see `mbedtls_memory_buffer_alloc_init`_ in the official Mbed TLS documentation).
The :ref:`nrf_security` variant of the Mbed TLS library allows to set such dedicated allocation buffer.
See :kconfig:`CONFIG_MBEDTLS_ENABLE_HEAP` and :kconfig:`CONFIG_MBEDTLS_HEAP_SIZE` for more details.

Flash memory layout
===================

The partition layout on the flash memory is set by :ref:`partition_manager`.

The application uses static configurations of partitions to ensure that the partition layout does not change between builds.
The :file:`pm_static_${build_type}.yml` file located in each board directory in the :file:`configuration` directory defines the static Partition Manager configuration for the given board and build type.
See `Configuration files`_ for more information.

The following partitions are used by the project:

* MCUboot - where the bootloader is located.
* SPM - where Secure Partition Manager (SPM) is optionally located (on secure builds).
* MCUboot primary slot - where the active application image is located.
* Pelion storage - where the Pelion Device Management library stores its data.
* Settings storage - where Zephyr's settings subsystem stores data to be available between boots.
* MCUboot secondary slot - where the application's update image is stored.

.. figure:: /images/pelion_client_memory.svg
   :alt: Flash memory structure

   Flash memory structure

External flash configuration
----------------------------

The Partition Manager supports partitions in external flash.

Enabling external flash is essential due to overall size of the application.
The secondary application slot can reside within the external flash.

It is not recommended to use the external flash for holding settings or Pelion storage as these areas hold important or secret data.

Network back ends
=================

The application supports the following network back ends:

* LTE using the :ref:`nrf_modem_lib_readme` for the nRF9160 DK
* OpenThread using Zephyr's :ref:`zephyr:thread_protocol_interface` for the nRF52840 DK and nRF5340 DK

Transport structure
===================

The Pelion Device Management library is solely responsible for communication with the Pelion Device Management.
The registered resource data is transmitted using the LwM2M protocol based on CoAP.
CoAP uses standard Zephyr's UDP sockets.

The Device Management Client tries to connect to the server periodically when the server cannot be reached.
To mitigate this when there is no network connectivity, the application pauses the Device Management Client when the network module informs about the network back end disconnection.
The Device Management Client operation resumes when the network connection is restored.

Bootloader
==========

The application uses MCUboot for booting and to perform its image update operation.

The MCUboot is capable of swapping the application images located on the secondary and primary slot.
Because of this, the image is not booted directly from the secondary image slot.
The MCUboot supports an external flash and the secondary image slot can be placed there if the application image is too large for the primary slot.

For more information about the MCUboot, see the :ref:`MCUboot <mcuboot:mcuboot_wrapper>` documentation.

.. _pelion_client_dfu:

Device firmware update
======================

The application is updatable.
The new image is transmitted to the device using the Pelion infrastructure.

You can configure the update by starting an update campaign on `Pelion Device Management Portal`_.
All devices for the given campaign receive the update image.
If the update manifest attached to the image is valid, the new image is stored by the Pelion Device Management library into the MCUboot secondary application slot.
When the entire image is stored, the device reboots and MCUboot completes the device update by swapping the application images.
For details, see the `Device Management Update`_ guide in the Pelion documentation.

.. note::
   Zephyr's port of Pelion might not support the most recent update client features.
   The Device Management Update link points to the documentation of the version compatible with the |NCS|.
   For details, refer to the `Pelion Device Management documentation`_ and `Pelion Device Management release notes`_.

Application states
==================

The following diagram shows the application states for the connection to the network and Pelion Device Management Portal.

.. figure:: /images/pelion_client_states.svg
   :alt: Application states

   Application states

The internal procedures refer to situations where both connection types interact with each other:

* The setup procedure takes place when the network connection is established, but the application has not yet connected to Pelion Device Management Portal.
* The pause procedure takes place when the network connection is failing for a known reason.
* The resume procedure takes place when the network connection is restored.

The state transitions are related to the LED behavior, as described in `User interface`_.

Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns, nrf5340dk_nrf5340_cpuapp, nrf5340dk_nrf5340_cpuapp_ns, nrf52840dk_nrf52840

The nRF Pelion Client application is configured to compile and run as a non-secure application on nRF91's and nRF5340's Cortex-M33.
Therefore, it automatically includes the :ref:`secure_partition_manager` that prepares the required peripherals to be available for the application.

Pelion Device Management requirements
=====================================

You need a developer account on `Pelion Device Management Portal`_.

.. _pelion_client_reqs_build_types:

nRF Pelion Client build types
=============================

The nRF Pelion Client application does not use a single :file:`prj.conf` file.
Configuration files are provided for different build types for each supported board.

Each board has its own :file:`prj.conf` file, which represents a ``debug`` build type.
Other build types are covered by dedicated files with the build type added as a suffix to the ``prj`` part, as per the following list.
For example, the ``release`` build type file name is :file:`prj_release.conf`.
If a board has other configuration files, for example associated with partition layout or child image configuration, these follow the same pattern.

.. include:: /gs_modifying.rst
   :start-after: build_types_overview_start
   :end-before: build_types_overview_end

Before you start testing the application, you can select one of the build types supported by nRF Pelion Client application, depending on your development kit and the building method.
The application supports the following build types:

* ``debug`` -- Debug version of the application - can be used to verify if the application works correctly.
* ``release`` -- Release version of the application - can be used to achieve better performance and reduce memory consumption.

Not every board supports both mentioned build types.
The given board can also support some additional configurations of the nRF Pelion Client application.

.. note::
    `Selecting a build type`_ is optional.
    The ``debug`` build type is used by default if no build type is explicitly selected.

User interface
**************

The application uses buttons for updating the OMA object resource counters in Pelion Device Management Portal.
It also uses LEDs to reflect the application tasks.

Buttons
=======

The application uses the following buttons on the supported development kits:

* **Button 1** -- Connected with the OMA Digital Input object instance 0.
* **Button 2** -- Connected with the OMA Digital Input object instance 1.
* **Button 3** -- Connected with the OMA Digital Input object instance 2.
* **Button 4** -- Connected with the OMA Digital Input object instance 3.

Pressing each of these buttons updates the respective object's state resource.
Each button press is counted and the count value is reflected onto the respective object's counter resource.

You can reset Digital Input objects' counters by putting zero into their respective values in Pelion Device Management Portal.

.. note::
   |factory_reset_note|

LEDs
====

The application displays LED behavior that corresponds to the task performed by the application.
The following table shows the LED behavior demonstrated by the application:

+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+
| Status                        | LED behavior                       | Related application state                                                      |
+===============================+====================================+================================================================================+
| Network back end disconnected | **LED1** breathing (500-ms period) | NET_STATE_DISABLED or NET_STATE_DISCONNECTED                                   |
+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+
| Network back end connected    | **LED1** solid on                  | NET_STATE_CONNECTED                                                            |
+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+
| Pelion connection search      | **LED2** breathing (500-ms period) | PELION_STATE_DISABLED or PELION_STATE_INITIALIZED or PELION_STATE_UNREGISTERED |
+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+
| Pelion device suspended       | **LED2** breathing (200-ms period) | PELION_STATE_SUSPENDED                                                         |
+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+
| Pelion connection established | **LED2** solid on                  | PELION_STATE_REGISTERED                                                        |
+-------------------------------+------------------------------------+--------------------------------------------------------------------------------+

See `Application states`_ for an overview of the application state transitions.

Configuration
*************

|pelion_lib_explanation|
The Pelion Device Management library is enabled with :kconfig:`CONFIG_PELION_CLIENT` configuration option.

For the library to work, you must enable and properly configure the Mbed TLS library.
To connect to Pelion Device Management Portal, the device must be provisioned with valid credentials.
For the firmware update procedure to work, you must provision the device with valid update resources.

For more information about Pelion configuration options, read the `Zephyr integration tutorial`_ in the Pelion documentation.

Mbed TLS
========

The Pelion Device Management library is depending on a properly configured Mbed TLS library.
To simplify the development within the |NCS|, a predefined set of Mbed TLS configuration options was prepared.
Set compatible with :ref:`nrfxlib:nrf_security` can be enabled using :kconfig:`CONFIG_PELION_NRF_SECURITY` configuration option.

Pelion credentials
==================

To be able to connect to Pelion Device Management Portal, you need to complete the following configuration:

1. Generate and download a developer certificate file from `Pelion Device Management Portal`_.
   This file contains keys used for securing connection with the Pelion network.
#. Copy the downloaded file to :file:`applications/pelion_client/configuration/common/`, replacing the default :file:`mbed_cloud_dev_credentials.c` file.
#. The file is used according to the developer flow provisioning (see :ref:`pelion_client_device_provisioning`).
   If the certificate is not present, :c:func:`fcc_developer_flow()` copies it from the developer certificate file (which is now part of the application binary).

For details steps, see the `Provisioning development devices`_ guide in the Pelion documentation.

Firmware update
===============

You can provide the devices with the new version of the firmware image over-the-air from Pelion Device Management Portal (see :ref:`pelion_client_dfu`).

Before the update, you must enable a compatible bootloader.
The Pelion Device Management library is compatible with MCUboot.
Enable it with the :kconfig:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option.
Along with the bootloader, you must also enable an image manager with the :kconfig:`CONFIG_IMG_MANAGER` and :kconfig:`CONFIG_MCUBOOT_IMG_MANAGER` Kconfig options.

When MCUboot is enabled the |NCS| build system generates the update image that can be uploaded to the secondary (update) slot.
The resulting signed file named :file:`app_update.bin` can be found in the build directory.
For more information refer to :ref:`mcuboot_ncs`.
This image is signed with the MCUboot private key.
By default, if a private key is present at :file:`applications/pelion_client/configuration/${BOARD_NAME}/mcuboot_private.pem` it will be used for the signing process.
If a key is not present and configuration does not point to any specific key path, the default sample MCUBoot key will be used.

Additionally, if the application image is large, you may need to store the update image on an external flash device.
In such case, enable the external flash support and correctly configure the partition layout.

The Pelion Device Management library's update manager is enabled when you select the :kconfig:`CONFIG_PELION_UPDATE` Kconfig option.
This option enables components required for image transport and storage.

For the update campaign to be recognized and the image to be accepted, you need to provision the device with the valid update resources (device unique identifiers and certificate used for update process validation).
These resources are normally stored on device at production time.
To simplify the development process, you can have the update resources created by the update manifest generation tool (see `Pelion Manifest Tool for version 4.7`_ in the Pelion documentation).
The generated C file must be stored to the :file:`update_default_resources.c` file, located in the :file:`applications/pelion_client/configuration/common` directory.
The update resources embedded into the application image are used to provision the device.

The manifest tool can further help during development as it gives a possibility to trigger an update campaign using one shell command.

Configuration files
===================

The nRF Pelion Client application uses the following files as configuration sources:

* Devicetree Specification (DTS) files - These reflect the hardware configuration.
  See :ref:`zephyr:dt-guide` for more information about the DTS data structure.
* Kconfig files - These reflect the software configuration.
  See :ref:`kconfig_tips_and_tricks` for information about how to configure them.
* :file:`_def` files - These contain configuration arrays for the application modules.
  The :file:`_def` files are used by the nRF Pelion Client application modules and :ref:`lib_caf` modules.

The application configuration files for a given board must be defined in a board-specific directory in the :file:`applications/pelion_client/configuration/` directory.
For example, the configuration files for the nRF9160 DK are defined in the :file:`applications/pelion_client/configuration/nrf9160dk_nrf9160_ns` directory.

The following configuration files can be defined for any supported board:

* :file:`prj_build_type.conf` - Kconfig configuration file for a build type.
  To support a given build type for the selected board, you must define the configuration file with a proper name.
  For example, the :file:`prj_release.conf` defines configuration for ``release`` build type.
  The :file:`prj.conf` without any suffix defines the ``debug`` build type.
* :file:`app.overlay` - DTS overlay file specific for the board.
  Defining the DTS overlay file for a given board is optional.
* :file:`_def` files - These files are defined separately for modules used by the application.
  You must define a :file:`_def` file for every module that requires it and enable it in the configuration for the given board.

Building and running
********************

.. |sample path| replace:: :file:`applications/pelion_client`

Before building and running the firmware, ensure that the Pelion Device Management is set up.
Also, the device must be provisioned and configured with the credentials for the connection attempt to succeed.
See `Configuration`_ for details.

The application is built the same way to any other |NCS| application or sample.

.. include:: /includes/build_and_run.txt

.. note::
    For information about the known issues, see |NCS|'s :ref:`release_notes` and on the :ref:`known_issues` page.

Selecting a build type
======================

Before you start testing the application, you can select one of the :ref:`pelion_client_reqs_build_types`, depending on your development kit and building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type in SES
-----------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_ses_start
   :end-before: build_types_selection_ses_end

Selecting a build type from command line
----------------------------------------

.. include:: /gs_modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: build_types_selection_cmd_end

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the development kit with a terminal emulator to observe log messages (for debug build types).
#. Reset the development kit.
#. Turn on the development kit.
   **LED1** and **LED2** start slowly breathing, which indicates the network is connecting to the Pelion Device Management.
   After several seconds, both LEDs stop blinking and remain turned on.
   This indicates that the device has established connection to the network back end and the Pelion server.
#. Log in to `Pelion Device Management Portal`_.
#. In the left pane, select :guilabel:`Device directory`.
   The list of all available devices is displayed.

   .. figure:: /images/pelion_device_directory.png
      :scale: 25 %
      :alt: Pelion's device directory

      Pelion's device directory (click to enlarge)

   A device that is actively communicating with the server (or was recently communicating with it) is listed with the ``Registered`` state and a green icon.
   The name of the device shows up on the list (and in the application log) only when it successfully connects to the server.
#. Select your device from the list of devices.
   The :guilabel:`Device details` pane appears.
#. In the :guilabel:`Device details` pane, select the :guilabel:`Resources` tab.
#. Scroll down to the value for the ``Digital Input`` resource.

   .. figure:: /images/pelion_device_resources.png
      :scale: 25 %
      :alt: Pelion's device directory with the resources tab selected

      Pelion's device directory with the resources tab selected (click to enlarge)

#. Press **Button 1**.
   The Digital Input instance 0 value in Pelion Device Management Portal increases.
#. Scroll down to the value for the ``Stopwatch`` resource.
#. Wait for a couple of seconds to see the value increase.

Dependencies
************

This application uses the following Zephyr library:

* :ref:`zephyr:thread_protocol_interface`

This application uses the following |NCS| libraries and drivers:

* :ref:`lib_caf`
* :ref:`event_manager`
* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`
* :ref:`partition_manager`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`
* :ref:`nrfxlib:nrf_security`

It uses the following `sdk-mcuboot`_ library:

* :ref:`MCUboot <mcuboot:mcuboot_wrapper>`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`

.. |pelion_lib_explanation| replace:: The application uses Pelion's ``mbed-cloud-client`` library, which is fetched as a module into the :file:`ncs/modules/lib/pelion-dm` directory of the |NCS|.
.. |factory_reset_note| replace::  Pressing and holding **Button 1** during the board boot up starts the factory reset process.
   Triggering the factory reset starts the provisioning process again.
