:orphan:

.. _ug_nrf54h20_suit_external_memory:

Firmware upgrade with external memory
#####################################

.. contents::
   :local:
   :depth: 2

Storing firmware upgrade data in external memory poses a unique challenge on nRF54H20 DK, because the secure domain is unable to access the external memory by itself.
The application domain is used as a proxy for the secure domain to access the external memory.

.. note::
   The prerequisite to this guide is the :ref:`ug_nrf54h20_suit_fetch` user guide, as this guide assumes that the application uses the fetch model to obtain the candidate images.
   See the :ref:`ug_nrf54h20_suit_fetch` for more details on how to migrate from push to fetch model.

The following terms are used in this guide:

* External memory (extmem) service - IPC service allowing the secure domain to use the application domain as a proxy when accessing external memory.

* Companion image - Application domain firmware implementing the extmem service and external memory driver.
  The IPC service allows the secure domain to access the external memory.

Overview of external memory in SUIT firmware updates
****************************************************

To use external memory with SUIT, the fetch model-based firmware upgrade is required.
The SUIT envelope must always be stored in the non-volatile memory in the MCU.
The SUIT manifests stored in the envelope contain instructions that the device must perform to fetch other required payloads.
To store payloads in the external memory, a DFU cache partition must be defined in the external memory's devicetree node.
In the SUIT manifest, you can define a component representing the cache partition in the external memory.
Within the ``suit-payload-fetch`` sequence, you can then store any fetched payload into any ``CACHE_POOL`` component.

When the secure domain processes the ``suit-install`` sequence, issuing ``suit-directive-fetch`` on any non-integrated payload will instruct the secure domain firmware to search for a given URI in all cache partitions in the system.
However, when such a cache partition is located in the external memory, the secure domain is unable to access the data.
Before any ``suit-directive-fetch`` directive is issued that accesses a payload stored on the external memory, a companion image that implements an external memory device driver must be booted.

The companion image consists of two main parts:

* Device driver adequate for the external memory device

* IPC service exposed towards the secure domain

When the companion image is booted and a directive that accesses the data on the external memory is issued, such as the ``suit-directive-fetch`` and ``suit-directive-copy`` directives, the secure domain firmware uses the IPC service provided by the companion image to access the contents of the external memory.
Beyond the booting of the companion image, the update process does not differ from regular fetch model-based update.

Enabling external flash support in SUIT DFU
*******************************************

The :file:`nrf/samples/suit/smp_transfer` sample contains a premade configuration enabling the external memory in SUIT DFU.
To enable the external memory, you must apply the :file:`nrf/samples/suit/smp_transfer/overlay-extmem.conf` and :file:`nrf/samples/suit/smp_transfer/ext_flash_nrf54h20dk.overlay` overlays to the sample build or complete the following steps:

1. Create a partition named ``companion_partition`` in the executable memory region that is accessible to the application domain by editing the devicetree (DTS) file:

   .. code-block:: devicetree

      &mram0 {
         partitions {
            companion_partition: partition@116000 {
               label = "companion-image";
               reg = <0x116000 DT_SIZE_K(64)>;
            };
         };
      };

   The ``companion_partition`` is the region where the companion image will be executed.
   Adjust the addresses accordingly to your application.

#. Define a new DFU cache partition in the external memory in the DTS file:

   .. code-block:: devicetree

      mx66um1g: mx66um1g45g@0 {
         ...
         partitions {
            dfu_cache_partition_1: partition@0 {
               reg = <0x0 DT_SIZE_K(512)>;
            };
         };
      };

   Note the name of the partition.
   The number at the end determines the ``CACHE_POOL`` ID, which will be used later in the SUIT manifest.

#. Modify the application manifest file :file:`app_envelope.yaml.jinja2` by completing the following:

   a. Modify the ``CACHE_POOL`` identifier in the SUIT manifest:

      .. code-block:: console

         suit-components:
             ...
         - - CACHE_POOL
           - 1

      The ``CACHE_POOL`` identifier must match the identifier of the cache partition defined in the DTS file.

   #. Append the ``MEM`` type component that represents the companion image in the same SUIT manifest file:

      .. code-block:: console

         suit-components:
             ...
         - - MEM
           - {{ flash_companion_subimage['dt'].label2node['cpu'].unit_addr }}
           - {{ get_absolute_address(flash_companion_subimage['dt'].chosen_nodes['zephyr,code-partition']) }}
           - {{ flash_companion_subimage['dt'].chosen_nodes['zephyr,code-partition'].regs[0].size }}

      In this example, the component index is ``3``.
      In the following steps, the companion image component is selected with ``suit-directive-set-component-index: 3``.

   #. Modify the ``suit-install`` sequence to boot the companion image before accessing the candidate images, which are stored in the external memory:

      .. code-block:: console

         suit-install:
         - suit-directive-set-component-index: 3
         - suit-directive-invoke:
            - suit-send-record-failure

      The companion image can be optionally upgraded and have its integrity checked.

#. Enable the :kconfig:option:`CONFIG_SUIT_EXTERNAL_MEMORY_SUPPORT` Kconfig option, which enables the build of the reference companion image to be used as a child image of the application firmware.
   It also enables other additional options that are required for the external memory DFU to work.

Create custom companion images
******************************

Nordic Semiconductor provides a reference companion image in the :file:`samples/suit/flash_companion` file, which can serve as a base for developing a customized companion image.

Limitations
***********

* The secure domain and companion image candidates must always be stored in MRAM.
  Trying to store those candidates in external memory will result in failure during the installation process.

* The companion image needs a dedicated area in the executable region of the MRAM that is assigned to the application domain.
