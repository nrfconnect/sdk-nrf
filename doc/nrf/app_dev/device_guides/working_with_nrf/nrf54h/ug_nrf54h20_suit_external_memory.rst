.. _ug_nrf54h20_suit_external_memory:

Firmware upgrade with external memory
#####################################

.. contents::
   :local:
   :depth: 2

The Secure Domain firmware is unable to access the external memory by itself.
However, the application domain can implement an IPC service, which allows the Secure Domain firmware to access the external memory through application domain serving as a proxy.
This guide explains how to prepare the application domain firmware and the SUIT envelope to perform SUIT firmware upgrade using external memory.

.. note::
   To use external memory with SUIT, you can either use the push model-based or the fetch model-based firmware upgrade.
   See :ref:`ug_nrf54h20_suit_push` for details about the push model and :ref:`ug_nrf54h20_suit_fetch` for details on how to migrate from the push to fetch model.

The following terms are used in this guide:

* External memory (extmem) service - IPC service allowing the Secure Domain to use the application domain as a proxy when accessing external memory.

* Companion image - Application domain firmware implementing the extmem service and external memory driver.
  The IPC service allows the Secure Domain to access the external memory.

Overview of external memory in SUIT firmware updates
****************************************************

The SUIT envelope must always be stored in the non-volatile memory in the MCU.
The SUIT manifests stored in the envelope contain instructions that the device must perform to fetch other required payloads.
To store payloads in the external memory, a Device Firmware Update (DFU) cache partition must be defined in the external memory's devicetree node.
The push model-based update and the fetch model-based update differ in the way the cache partition is filled with the images.

When the Secure Domain processes the ``suit-install`` sequence, issuing ``suit-directive-fetch`` on any non-integrated payload will instruct the Secure Domain firmware to search for a given URI in all cache partitions in the system.
However, when such a cache partition is located in the external memory, the Secure Domain is unable to access the data directly.
Before any ``suit-directive-fetch`` directive is issued that accesses a payload stored on the external memory, a companion image that implements an external memory device driver must be booted.

The companion image consists of two main parts:

* Device driver adequate for the external memory device

* IPC service exposed towards the Secure Domain

When the companion image is booted and a directive that accesses the data on the external memory is issued, such as the ``suit-directive-fetch`` or ``suit-directive-copy`` directives, the Secure Domain firmware uses the IPC service provided by the companion image to access the contents of the external memory.
Apart from booting the companion image, the update process does not differ from regular push model-based or fetch model-based updates.

Difference between push and fetch models
========================================

Push model
----------

In the push model, the cache partition contents are created on the building machine and pushed to the device without modifications.
The images are extracted to the cache partition files using the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE`  Kconfig option.

No additional sequences are required in the SUIT manifest.

For more details, see :ref:`ug_nrf54h20_suit_push`.

Fetch model
-----------

In the fetch model, the SUIT processor runs on the application core.

In the SUIT manifest, you can define a component that represents the cache partition in the external memory.
Within the ``suit-payload-fetch`` sequence, you can then store fetched payloads into a ``CACHE_POOL`` component.
The device then pulls the images from external sources and manages their storage in the cache partitions.

For more details, see :ref:`ug_nrf54h20_suit_fetch`.

Enabling external flash support in SUIT DFU
*******************************************

The :ref:`nrf54h_suit_sample` sample contains several example configurations that enable the external memory for SUIT DFU.

The configurations using the push model are the following:

* ``sample.suit.smp_transfer.cache_push.extflash``
* ``sample.suit.smp_transfer.cache_push.extflash.bt``

The configurations using the fetch model are following:

* ``sample.suit.smp_transfer.full_processing.extflash``
* ``sample.suit.smp_transfer.full_processing.extflash.bt``

You can find these configurations defined in the :file:`samples/suit/smp_transfer/sample.yaml` file.
This file specifies which options need to be enabled.

Alternatively, you can follow the following steps to manually enable external memory in SUIT DFU.

Common steps for both push and fetch models
===========================================

1. Turn on the external flash chip on the nRF54H20 DK using the `nRF Connect Board Configurator`_ app within `nRF Connect for Desktop`_ .

   .. note::
      This step is needed only on nRF54H20 DK. Skip this step if you are using different hardware.

#. Enable the ``SB_CONFIG_SUIT_BUILD_FLASH_COMPANION`` sysbuild Kconfig option, which enables the build of the reference companion image.
   See the :ref:`suit_flash_companion` user guide for instructions on how to configure the companion image using sysbuild.

#. Define a new DFU cache partition in the external memory in the DTS file:

   .. code-block:: devicetree

      &mx25uw63 {
         ...
         status = "okay";
         partitions {
            compatible = "fixed-partitions";
            #address-cells = <1>;
            #size-cells = <1>;

            dfu_cache_partition_1: partition@0 {
               reg = <0x0 DT_SIZE_K(1024)>;
            };
         };
      };

   Note that the name of the partition must follow the following format: ``dfu_cache_partition_<n>``.
   The number at the end determines the ``CACHE_POOL`` ID, which will be used later in the SUIT manifest.
   This number must be greater than 0 and less than the value of :kconfig:option:`CONFIG_SUIT_CACHE_MAX_CACHES`.
   The Secure Domain firmware supports up to eight DFU cache partitions.

#. Modify the application manifest file :file:`app_envelope.yaml.jinja2` by completing the following steps:

   a. Append the ``MEM`` type component that represents the companion image in the same SUIT manifest file:

      .. code-block:: yaml

         suit-components:
             ...
         - - MEM
           - {{ flash_companion['dt'].label2node['cpu'].unit_addr }}
           - {{ get_absolute_address(flash_companion['dt'].chosen_nodes['zephyr,code-partition']) }}
           - {{ flash_companion['dt'].chosen_nodes['zephyr,code-partition'].regs[0].size }}

      In this example, the component index is ``3``.
      In the following steps, the companion image component is selected with ``suit-directive-set-component-index: 3``.

   #. Append directives to the ``suit-shared-sequence`` sequence in the application manifest file (:file:`app_envelope.yaml.jinja2`) to set the expected digest of the companion application:

      .. code-block:: yaml

         suit-shared-sequence:
         - suit-directive-set-component-index: 3
         - suit-directive-override-parameters:
             suit-parameter-image-digest:
               suit-digest-algorithm-id: cose-alg-sha-256
               suit-digest-bytes:
                 file: {{ flash_companion['binary'] }}

   #. Modify the ``suit-install`` sequence in the application manifest file (:file:`app_envelope.yaml.jinja2`) to load into RAM and boot the companion image before accessing the candidate images stored in the external memory:

      .. code-block:: yaml

         suit-install:
         # Use CAND_IMG component to access flash companion binary, stored as one of the integrated payloads.
         - suit-directive-set-component-index: 1
         - suit-directive-override-parameters:
             suit-parameter-uri: "#{{ flash_companion['name'] }}"
             suit-parameter-image-digest:
               suit-digest-algorithm-id: cose-alg-sha-256
               suit-digest-bytes:
                 file: {{ flash_companion['binary'] }}
         - suit-directive-fetch:
           - suit-send-record-failure
         # Verify integrity of the companion binary in the envelope.
         - suit-condition-image-match:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure
         # Copy companion into local RAM.
         - suit-directive-set-component-index: 3
         - suit-directive-override-parameters:
             suit-parameter-source-component: 1
         - suit-directive-copy:
           - suit-send-record-failure
         # Verify integrity of the companion binary in the local RAM.
         - suit-condition-image-match:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure
         # Boot the companion application.
         - suit-directive-invoke:
           - suit-send-record-failure

      To further improve the update procedure robustness, you can copy all integrity checks from the ``suit-install`` to the ``suit-candidate-verification`` sequence.

   #. Append the flash companion binary as integrated payload inside SUIT envelope:

      .. code-block:: yaml

         SUIT_Envelope_Tagged:
            suit-authentication-wrapper: { ... }
            suit-manifest: { .. }

            suit-integrated-payloads:
               "#{{ flash_companion['name'] }}": {{ flash_companion['binary'] }}

      The flash companion binary must be available before the driver of the external memory is booted, so it must be transferred as part of the update candidate envelope and fit within the DFU partition inside the internal MRAM memory.

Steps specific for the push model
=================================

1. Enable the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_PUSH_TO_CACHE` option to allow the application core to modify cache partitions.

#.  Enable the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE` Kconfig option for every image that needs to be updated from external memory.

#. Modify the manifest files for all domains by completing the following steps:

   a. Ensure that the URI used by the ``suit-directive-fetch`` command to fetch a given image matches the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI` Kconfig option.

   #. Ensure that the envelope integrates the specified image within the envelope integrated payloads section.
      This is ensured by default if you use the provided SUIT envelope templates.

Steps specific for the fetch model
==================================

1. Enable the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL` Kconfig option to allow the application core to process SUIT manifests.

#. Enable the :kconfig:option:`CONFIG_SUIT_STREAM_SOURCE_FLASH` Kconfig option, which allows the SUIT processor on the application core to read and parse DFU cache partitions.

#. Modify the application manifest file :file:`app_envelope.yaml.jinja2` by completing the following steps:

   a. Modify the ``CACHE_POOL`` identifier in the SUIT manifest:

      .. code-block:: yaml

         suit-components:
             ...
         - - CACHE_POOL
           - 1

      The ``CACHE_POOL`` identifier must match the identifier of the cache partition defined in the DTS file.

   #. Add the ``suit-payload-fetch`` sequence:

      .. code-block:: yaml

         suit-payload-fetch:
         - suit-directive-set-component-index: 2
         - suit-directive-override-parameters:
             suit-parameter-uri: 'file://{{ app['binary'] }}'
         - suit-directive-fetch:
           - suit-send-record-failure

      This snippet assumes that ``CACHE_POOL`` is the third component on the manifest's components list (so its component index is ``2``)

#. If your application uses the radio core, complete the following steps to modify the radio core manifest file :file:`rad_envelope.yaml.jinja2`:

   a. Modify the ``CACHE_POOL`` identifier in the SUIT manifest:

      .. code-block:: yaml

         suit-components:
             ...
         - - CACHE_POOL
           - 1

      The ``CACHE_POOL`` identifier must match the identifier of the cache partition defined in the DTS file.

   #. Add the ``suit-payload-fetch`` sequence:

      .. code-block:: yaml

         suit-payload-fetch:
         - suit-directive-set-component-index: 2
         - suit-directive-override-parameters:
             suit-parameter-uri: 'file://{{ radio['binary'] }}'
         - suit-directive-fetch:
           - suit-send-record-failure

      This snippet assumes that ``CACHE_POOL`` is the third component on the manifest's components list (so its component index is ``2``)

#. Complete the following steps to modify the root manifest file :file:`root_with_binary_nordic_top_extflash.yaml.jinja2`:

   a. Add the ``suit-payload-fetch`` sequence:

      .. code-block:: yaml

         suit-payload-fetch:
         - suit-directive-set-component-index: 0
         - suit-directive-override-parameters:
             suit-parameter-uri: "#{{ application['name'] }}"
         - suit-directive-fetch:
           - suit-send-record-failure
         - suit-condition-dependency-integrity:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure
         - suit-directive-process-dependency:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure

      This snippet assumes that ``CAND_MFST`` is the first component on the manifest's components list (so its component index is ``0``).

   #. If your application uses the radio core, append the following to the ``suit-payload-fetch`` sequence:

      .. code-block:: yaml

         suit-payload-fetch:
         - suit-directive-set-component-index: 0
         - suit-directive-override-parameters:
             suit-parameter-uri: "#{{ radio['name'] }}"
         - suit-directive-fetch:
           - suit-send-record-failure
         - suit-condition-dependency-integrity:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure
         - suit-directive-process-dependency:
           - suit-send-record-success
           - suit-send-record-failure
           - suit-send-sysinfo-success
           - suit-send-sysinfo-failure

Testing the application with external flash support
===================================================

1. |open_terminal_window_with_environment|

#. Build and flash the application by completing the following commands:

   .. code-block:: console

      west build ./ -b nrf54h20dk/nrf54h20/cpuapp -T <configuration_name>
      west flash

   The build system will automatically use :ref:`configuration_system_overview_sysbuild` and generate a :file:`build/zephyr/dfu_suit.zip` archive, which contains the SUIT envelope and candidate images.

#. Build a new version of the application with the incremented ``CONFIG_N_BLINKS`` value.

#. Download the new :file:`dfu_suit.zip` archive to your mobile device.

#. Use the `nRF Connect Device Manager`_ mobile app to update your device with the new firmware by completing the following steps:

   a. Ensure that you can access the :file:`dfu_suit.zip` archive from your phone or tablet.

   #. In the mobile app, scan and select the device to update.

   #. Switch to the :guilabel:`Image` tab.

   #. Press the :guilabel:`SELECT FILE` button and select the :file:`dfu_suit.zip` archive.

   #. Press the :guilabel:`START` button.
      This initiates the DFU process of transferring the image to the device.

      The Device Manager mobile application will unpack the file and upload the SUIT envelope to the device.
      The firmware images will be uploaded separately by the mobile application to the device, if the device requests it.

   #. Wait for the DFU to finish and then verify that the application works properly.

Create custom companion images
******************************

Nordic Semiconductor provides a reference companion image in the :file:`samples/suit/flash_companion` directory, which can serve as a base for developing a customized companion image.

Limitations
***********

* The Secure Domain, System Controller and companion image update candidates must always be stored in the MRAM.
  Trying to store those candidates in external memory will result in a failure during the installation process.

* The companion image needs either a dedicated area in the executable region of the MRAM or it needs to fit within the local RAM that is assigned to the application domain.
  The default flash companion application uses local RAM, so there is no need to reserve additional area in the main application memory map.
