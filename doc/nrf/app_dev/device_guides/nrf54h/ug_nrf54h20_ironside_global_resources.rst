.. _ug_nrf54h20_ironside_global_resources:
.. _ug_nrf54h20_ironside_se_uicr:

Configuring global resources using UICR
#######################################

.. contents::
   :local:
   :depth: 2

|ISE| uses the User Information Configuration Registers (UICR) to securely configure persistent global resource settings.
The UICR is a dedicated non-volatile memory region (and its associated registers) located in the first NVR page (NVR0).
It holds device-specific configuration data, such as application-owned memory boundaries and selected peripheral parameters, which are used to configure the system.

|ISE| applies the UICR configuration as part of the system boot procedure, before starting the application core.
Updates to the UICR configuration require a system reset to take effect.
If |ISE| detects an invalid UICR configuration, it reports a :ref:`boot error <ug_nrf54h20_ironside_se_bootstatus_register_format>` and starts the application CPU in halted mode.
Additional details about the UICR error are written to the :ref:`boot report <ug_nrf54h20_ironside_se_boot_report>`.

.. note::
   When you configure a custom devicetree for your board, the build system automatically generates UICR content based on the devicetree configuration.

The following UICR fields are supported:

+----------------------+---------------------------------------------------------------------+
| UICR Field           | Description                                                         |
+======================+=====================================================================+
| UICR.VERSION         | A 32-bit value that encodes the UICR format version (16-bit major   |
|                      | and 16-bit minor).                                                  |
+----------------------+---------------------------------------------------------------------+
| UICR.LOCK            | Locks all contents of NVR0, preventing any further writes without   |
|                      | performing an ERASEALL operation.                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.APPROTECT       | Configures debugger and access-port permissions for each AP via the |
|                      | TAMPC peripheral.                                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.ERASEPROTECT    | Blocks ERASEALL commands to NVR0.                                   |
+----------------------+---------------------------------------------------------------------+
| UICR.PROTECTEDMEM    | Defines the size (in 4 KiB blocks) of an integrity-checked memory   |
|                      | region at the start of the application-owned part of MRAM.          |
+----------------------+---------------------------------------------------------------------+
| UICR.PERIPHCONF      | Points to an array of key-value entries used to initialize approved |
|                      | global peripherals.                                                 |
+----------------------+---------------------------------------------------------------------+
| UICR.MPCCONF         | Not supported yet.                                                  |
+----------------------+---------------------------------------------------------------------+
| UICR.WDTSTART        | Configures automatic start of a local watchdog timer before the     |
|                      | application core is booted, providing early system protection.      |
+----------------------+---------------------------------------------------------------------+
| UICR.SECURESTORAGE   | Defines secure storage configuration including address, and         |
|                      | partition sizes for cryptographic and ITS services.                 |
+----------------------+---------------------------------------------------------------------+
| UICR.SECONDARY       | Configures secondary firmware boot settings including processor     |
|                      | selection, triggers, memory protection, and peripheral access.      |
+----------------------+---------------------------------------------------------------------+

.. note::
   If no UICR values are programmed, |ISE| applies a set of :ref:`default configurations <ug_nrf54h20_ironside_se_protecting>`.
   Applications that do not require custom settings can rely on these defaults without modifying the UICR.

Performing an :ref:`ERASEALL <ug_nrf54h20_ironside_se_eraseall_command>` operation will erase all UICR contents and remove all protection mechanisms enforced through UICR.
See :ref:`ug_nrf54h20_ironside_se_protecting` for more information on protecting UICR contents in the field.

.. _ug_nrf54h20_ironside_se_uicr_image:

Generating the UICR image
*************************

When applications need application-specific UICR contents they may use the UICR image.
The UICR image is an automatically created build artifact that generates the UICR content and peripheral configuration (PERIPHCONF) entries.

UICR image
==========

The UICR image will generate HEX files at :file:`build/uicr/zephyr/` inside the build directory.

.. list-table::
   :header-rows: 1
   :widths: auto

   * - Component
     - File
     - Description
   * - Merged UICR image
     - :file:`zephyr.hex`
     - The merged hex file contains all other hex files. This file is automatically programmed when using ``west flash``.
   * - UICR-only image
     - :file:`uicr.hex`
     - Contains only the UICR configuration structure.
   * - PERIPHCONF-only image
     - :file:`periphconf.hex`
     - Contains only the PERIPHCONF entries. Generated when :kconfig:option:`CONFIG_GEN_UICR_GENERATE_PERIPHCONF` is enabled.
   * - Secondary PERIPHCONF image
     - :file:`secondary_periphconf.hex`
     - Contains only the secondary firmware's PERIPHCONF entries. Generated when :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF` is enabled.

UICR image generation process
=============================

The UICR image generation process:

1. Devicetree analysis - The build system analyzes all devicetrees in the sysbuild build to identify peripheral usage and pin assignments across all images.
#. PERIPHCONF generation - Python scripts generate C source code containing UICR configuration macros based on the devicetree.
#. Compilation - The generated C code is compiled into ELF binaries containing PERIPHCONF sections.
#. ELF extraction - The UICR generator extracts PERIPHCONF data from ELF files and merges them together.
#. Kconfig extraction - The UICR generator uses an application-specific UICR Kconfig to populate the UICR entries.
#. HEX file creation - Multiple HEX files are generated for different purposes.

Configuring the UICR image
**************************

Applications that use UICR features must place a :file:`sysbuild/uicr.conf` Kconfig fragment file in their project directory to configure the Kconfig of the UICR image.

The following code snippet shows an example configuration for secondary firmware support in :file:`sysbuild/uicr.conf`:

.. code-block:: kconfig

   # Configuration overlay for uicr image
   # Enable UICR.SECONDARY.ENABLE
   CONFIG_GEN_UICR_SECONDARY=y

For more examples of UICR configuration, see :ref:`ironside_se`:

* :ref:`secondary_boot_sample` - Basic secondary firmware configuration
* :ref:`secondary_boot_trigger_lockup_sample` - Secondary firmware with automatic triggers

UICR Kconfig configuration options
==================================

The UICR Kconfig options are listed below, and are defined with more detailed Kconfig help texts in :file:`zephyr/soc/nordic/common/uicr/gen_uicr/Kconfig`.

.. list-table:: UICR Kconfig Options
   :widths: 50 50
   :header-rows: 1

   * - Kconfig option
     - Description
   * - :kconfig:option:`CONFIG_GEN_UICR_GENERATE_PERIPHCONF`
     - Generate a PERIPHCONF hex in addition to the UICR hex
   * - :kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE`
     - Enable UICR.SECURESTORAGE
   * - :kconfig:option:`CONFIG_GEN_UICR_LOCK`
     - Enable UICR.LOCK
   * - :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT`
     - Enable UICR.ERASEPROTECT
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`
     - Protect application domain access port
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED`
     - Protect radio core access port
   * - :kconfig:option:`CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED`
     - Disable CoreSight subsystem
   * - :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM`
     - Enable UICR.PROTECTEDMEM
   * - :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM_SIZE_BYTES`
     - Protected memory size in bytes
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART`
     - Enable UICR.WDTSTART
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT0`
     - Use watchdog timer instance 0
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT1`
     - Use watchdog timer instance 1
   * - :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_CRV`
     - Initial Counter Reload Value (CRV) for the watchdog timer
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY`
     - Enable UICR.SECONDARY.ENABLE
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_GENERATE_PERIPHCONF`
     - Generate SECONDARY.PERIPHCONF hex alongside UICR
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROCESSOR_APPLICATION`
     - Boot secondary firmware on the APPLICATION processor
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROCESSOR_RADIOCORE`
     - Boot secondary firmware on the RADIOCORE processor
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER`
     - Enable UICR.SECONDARY.TRIGGER
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT0`
     - Trigger on Application domain watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONWDT1`
     - Trigger on Application domain watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP`
     - Trigger on Application domain CPU lockup reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT0`
     - Trigger on Radio core watchdog 0 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCOREWDT1`
     - Trigger on Radio core watchdog 1 reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_TRIGGER_RADIOCORELOCKUP`
     - Trigger on Radio core CPU lockup reset
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART`
     - Enable UICR.SECONDARY.WDTSTART
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_INSTANCE_WDT0`
     - Use watchdog timer instance 0 for secondary firmware
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_INSTANCE_WDT1`
     - Use watchdog timer instance 1 for secondary firmware
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_WDTSTART_CRV`
     - Secondary initial Counter Reload Value (CRV) for the watchdog timer
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM`
     - Enable UICR.SECONDARY.PROTECTEDMEM
   * - :kconfig:option:`CONFIG_GEN_UICR_SECONDARY_PROTECTEDMEM_SIZE_BYTES`
     - Secondary protected memory size in bytes

.. _ug_nrf54h20_ironside_se_uicr_version:

UICR.VERSION
------------

UICR.VERSION specifies the version of the UICR format in use.
It is divided into a 16-bit major version and a 16-bit minor version.

|ISE| requires that the VERSION field contains a supported version number.
An invalid or unsupported version number results in an error.
The versioning scheme allows |ISE| to support multiple UICR formats, enabling updates to the format without breaking compatibility with existing configurations.

The UICR erase value is interpreted as the highest version supported by the installed |ISE|.

.. note::
   Deployed devices must have the VERSION field configured, to ensure forward compatibility with newer |ISE| releases.

In the UICR generated by the build system, the VERSION field is set automatically based on the enabled features.

.. _ug_nrf54h20_ironside_se_uicr_lock:

UICR.LOCK
---------

Enabling :kconfig:option:`CONFIG_GEN_UICR_LOCK` locks the entire contents of the NVR0 page located in MRAM10.
This includes all values in both the UICR and the BICR (the Board Information Configuration Registers).
When :kconfig:option:`CONFIG_GEN_UICR_LOCK` is enabled, you can modify the UICR only by performing an ERASEALL operation.

.. note::
   While BICR is not erased during an ERASEALL operation, executing ERASEALL lifts the :kconfig:option:`CONFIG_GEN_UICR_LOCK` restriction, allowing write access to BICR.

Locking is enforced through an HMAC integrity check and by configuring the NVR page as read-only in the MRAMC.
The MAC is computed and stored on the first boot after the LOCK field has been enabled in the UICR.

If the integrity check fails, the application CPU subsystem will still be given clock and power, but the CPU will be stalled and prevented from executing instructions.
It is not possible to boot the secondary firmware if the integrity check fails.

.. _ug_nrf54h20_ironside_se_uicr_approtect:

UICR.APPROTECT
--------------

The UICR.APPROTECT configuration consists of the following sub-registers:

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_APPLICATION_PROTECTED`
  Controls access port protection for the application domain processor.
  This setting determines whether the debugger can access the application domain memory, registers, and debug features.

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_RADIOCORE_PROTECTED`
  Controls access port protection for the radio core processor.
  This setting determines whether the debugger can access the radio core memory, registers, and debug features.

:kconfig:option:`CONFIG_GEN_UICR_APPROTECT_CORESIGHT_PROTECTED`
  Controls access port protection for the CoreSight debug infrastructure.
  This setting determines whether system-level trace features are accessible.

.. note::
   To fully protect a production device, enable all three Kconfig options.

.. _ug_nrf54h20_ironside_se_uicr_eraseprotect:

UICR.ERASEPROTECT
-----------------

Enabling :kconfig:option:`CONFIG_GEN_UICR_ERASEPROTECT` blocks the ERASEALL operation.
However, it does not prevent erase operations initiated through other means, such as writing erase values via a debugger.

.. note::
   If this configuration is enabled and :kconfig:option:`CONFIG_GEN_UICR_LOCK` is also set, it is no longer possible to modify the UICR in any way.
   Therefore, this configuration should only be enabled during the final stages of production.

.. _ug_nrf54h20_ironside_se_uicr_protectedmem:

UICR.PROTECTEDMEM
-----------------

Enabling :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM` enables UICR.PROTECTEDMEM.
UICR.PROTECTEDMEM can be used to specify a memory region that will have its integrity ensured by |ISE|.
This memory can contain immutable bootloaders, UICR.PERIPHCONF entries, or any other data that should be immutable.
By ensuring the integrity of this memory region, |ISE| extends the Root of Trust to any immutable bootloader located in this region.

The memory region always starts at the lowest MRAM address of the application-owned memory and has an application-specific size specified in :kconfig:option:`CONFIG_GEN_UICR_PROTECTEDMEM_SIZE_BYTES`.

.. _ug_nrf54h20_ironside_se_uicr_periphconf:

UICR.PERIPHCONF
---------------

UICR.PERIPHCONF points to an array of key-value pairs used to initialize specific global peripherals before the application starts.
This mechanism allows for the one-time configuration of peripherals managed by |ISE| and is not designed for general system initialization.

Each entry in the array consists of two 32-bit values.
The fields in each value are described in the following tables.

Value 0:

+-------------------+----------------------------------+-------------------------+
| Bit number(s)     | 31-2                             | 1-0                     |
+-------------------+----------------------------------+-------------------------+
| Field             | REGPTR                           | UNUSED                  |
+-------------------+----------------------------------+-------------------------+
| Description       | Bits [31:2] of a pointer to a    | Unused.                 |
|                   | peripheral register.             |                         |
+-------------------+----------------------------------+-------------------------+

Value 1:

+-------------------+----------------------------------+
| Bit number(s)     | 31-0                             |
+-------------------+----------------------------------+
| Field             | VALUE                            |
+-------------------+----------------------------------+
| Description       | Register value.                  |
+-------------------+----------------------------------+

|ISE| processes the PERIPHCONF array sequentially, starting from the address specified by UICR.PERIPHCONF.ADDRESS.
Processing continues until either the number of entries defined by UICR.PERIPHCONF.MAXCOUNT has been processed, or an entry is encountered with the REGPTR field set to 0x3FFF_FFFF (all ones), which indicates the end of the array.

|ISE| uses an allow list to determine which register addresses the REGPTR field is permitted to reference.
Each register address in the allow list has an associated bit mask that specifies which bits from the VALUE field are applied to the target register.

Given an entry in the PERIPHCONF array and a bit mask M, |ISE| performs the following write operation::

   *(REGPTR << 2) = (VALUE & M) | (*(REGPTR << 2) & ~M)

The register allow list and corresponding bit masks are documented with each |ISE| release.

Each entry in the PERIPHCONF array is validated during processing.
To pass validation, (REGPTR << 2) must point to a register address included in the allow list.

After applying the entry, |ISE| performs a read-back check: it reads back the register value, applies the bit mask, and compares the result against the masked VALUE field.

The configuration procedure is aborted if an entry fails either the validation or the read-back check.
If a failure occurs, BOOTSTATUS.BOOTERROR is set to indicate the error condition, and a description of the failed entry is written to the boot report.

.. _ug_nrf54h20_ironside_se_periphconf_devicetree:

Generating PERIPHCONF from devicetree
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When :kconfig:option:`CONFIG_NRF_PERIPHCONF_GENERATE_ENTRIES` is enabled, the build system automatically generates entries for the PERIPHCONF partition based on the devicetree.
To determine which peripherals are in use, the build system analyzes the devicetree as follows:

#. Enumerate all peripheral nodes and include only those with a ``status`` property set to ``okay``.
#. Parse peripheral-specific attributes (for example, the ``owned-channels`` property in DPPIC nodes).
#. Collect GPIO pin assignments from all pin references (for example, ``pinctrl`` entries).

It then generates the appropriate configuration values by reusing existing properties.
The build system outputs the generated entries as a C file named :file:`periphconf_entries_generated.c` in the build directory, which is added as a source file to the build.
The generated C code uses the macros defined in :file:`zephyr/modules/hal_nordic/ironside/se/include/ironside_zephyr/se/uicr_periphconf.h` to insert entries into a special PERIPHCONF section in the ELF file which can be read by the UICR image.

See the following table for a mapping between the devicetree input used by the PERIPHCONF entry generator script and the resulting output in the :file:`periphconf_entries_generated.c` file:

.. list-table:: Mapping between devicetree and PERIPHCONF entry output (UICR Configuration)
   :header-rows: 1
   :widths: 25 15 35 25

   * - Devicetree node type
     - Properties
     - PERIPHCONF entry output
     - Example generated output
   * - Peripheral Access Control
     -
     -
     -
   * - Nordic global domain peripheral with status ``= {"okay", "reserved"}``
     - ``reg``

       ``interrupt-parent``
     - SPU Peripheral Permissions:
       PERIPHCONF_SPU_PERIPH_PERM(...) sets ownership and secure attribute based on bit 28 of bus parent or peripheral address.

       IRQ Routing:
       PERIPHCONF_IRQMAP_IRQ_SINK(...) maps interrupt to processor owning the interrupt controller or devicetree processor.
     -
       .. code-block:: c

          /* SPU137 configuration for uart136 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_PERIPH_PERM(0x5f9d0000UL, 5, true, true, NRF_OWNER_APPLICATION)
          );
          /* uart136 IRQ => APPLICATION */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_IRQMAP_IRQ_SINK(469, NRF_PROCESSOR_APPLICATION);
         );
   * - Channel-Based Features
     -
     -
     -
   * - Nordic global domain GPIOTE peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - GPIOTE Channel Control:
       PERIPHCONF_SPU_FEATURE_GPIOTE_CH(...) sets channel ownership to devicetree processor. Secure attribute from explicit specification or address logic.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpiote130 ch. 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GPIOTE_CH(0x5f920000UL, 0, 0, true, NRF_OWNER_APPLICATION);
          );
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - DPPIC Channel Control:
       PERIPHCONF_SPU_FEATURE_DPPIC_CH_SET(...) configures channel ownership and security.
     -
       .. code-block:: c

          /* SPU131 feature configuration for DPPIC130 ch. 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_DPPIC_CH(0x5f920000UL, 0, false, NRF_OWNER_RADIOCORE)
          );
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channel-groups``

       ``nonsecure-channel-groups``
     - DPPIC Channel Group Control:
       PERIPHCONF_SPU_FEATURE_DPPIC_CHG(...) configures channel group ownership and security.
     -
       .. code-block:: c

          /* SPU131 feature configuration for DPPIC130 ch. group 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_DPPIC_CHG(0x5f920000UL, 0, true, NRF_OWNER_APPLICATION)
          );
   * - Nordic global domain DPPIC peripheral with status ``= {"okay", "reserved"}``
     - ``sink-channels``

       ``source-channels``
     - PPIB Cross-Domain Connection:
       PERIPHCONF_PPIB_SUBSCRIBE_SEND(...) and PERIPHCONF_PPIB_PUBLISH_RECEIVE(...) connect PPI domains. Property name determines connection direction. (Ignored for DPPIC130)
     -
       .. code-block:: c

          /* PPIB133 ch. 0 => PPIB130 ch. 8 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_PPIB_SUBSCRIBE_SEND(0x5f99d000UL, 0)
          );
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_PPIB_PUBLISH_RECEIVE(0x5f925000UL, 8)
          );
   * - Nordic global domain IPCT peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - IPCT Channel Control:
       PERIPHCONF_SPU_FEATURE_IPCT_CH_SET(...) sets channel ownership and security attributes.
     -
       .. code-block:: c

          /* SPU131 feature configuration for ipct130 ch. 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_IPCT_CH(0x5f920000UL, 0, true, NRF_OWNER_RADIOCORE)
          );
   * - Nordic IPCT peripheral with status ``= {"okay", "reserved"}``
     - ``source-channel-links``

       ``sink-channel-links``
     - IPC Domain Mapping:
       PERIPHCONF_IPCMAP_CHANNEL_CFG(...) connects channels between domains.
     -
       .. code-block:: c

          /* RADIOCORE IPCT ch. 2 => GLOBALSLOW IPCT ch. 2 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_IPCMAP_CHANNEL(0, NRF_DOMAIN_RADIOCORE, 2, NRF_DOMAIN_GLOBALSLOW, 2)
          );
   * - Nordic GRTC peripheral with status ``= {"okay", "reserved"}``
     - ``owned-channels``

       ``child-owned-channels``

       ``nonsecure-channels``
     - GRTC Compare Channel Control:
       PERIPHCONF_SPU_FEATURE_GRTC_CC_SET(...) configures compare channel ownership and security.
     -
       .. code-block:: c

          /* SPU133 feature configuration for GRTC CC4 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GRTC_CC(0x5f990000UL, 4, true, NRF_OWNER_APPLICATION)
          );
   * - GPIO Pin Control
     -
     -
     -
   * - Nodes with GPIO pin properties
     - Any property with type ``phandle-array`` named *gpios* or ending with *-gpios*
     - GPIO Pin Ownership + Multiplexing:
       PERIPHCONF_SPU_FEATURE_GPIO_PIN(...) sets pin ownership. PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(...) configures pin multiplexer using internal lookup table.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio9, P9.0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000UL, 9, 0, true, NRF_OWNER_APPLICATION)
          );
          /* gpio9 - P9.0 CTRLSEL = 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(0x5f939200UL, 0, 0)
          );
   * - Nodes with pinctrl configuration properties
     - Pinctrl configuration properties ("pinctrl-0", "pinctrl-1", etc.)
     - Pin Function Control:
       PERIPHCONF_SPU_FEATURE_GPIO_PIN(...) for ownership. PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(...) for function-specific multiplexing.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio6, P6.0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000UL, 6, 0, true, NRF_OWNER_APPLICATION)
          );
          /* gpio6 - P6.0 CTRLSEL = 4 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(0x5f938c00UL, 0, 4)
          );
   * - Nordic SAADC peripheral
     - ``zephyr,input-positive``

       ``zephyr,input-negative``
     - Analog Pin Control:
       PERIPHCONF_SPU_FEATURE_GPIO_PIN(...) for pin ownership. PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(...) for GPIO function.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio0, P0.4 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000UL, 0, 4, true, NRF_OWNER_APPLICATION)
          );
          /* gpio0 - P0.4 CTRLSEL = 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(0x5f938000UL, 4, 0)
          );
   * - Nordic COMP/LPCOMP peripherals
     - ``psel``

       ``extrefsel``
     - Comparator Pin Control:
       PERIPHCONF_SPU_FEATURE_GPIO_PIN(...) for pin ownership. PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(...) for GPIO function.
     -
       .. code-block:: c

          /* SPU131 feature configuration for gpio1, P1.2 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_SPU_FEATURE_GPIO_PIN(0x5f920000UL, 1, 2, true, NRF_OWNER_APPLICATION)
          );
          /* gpio1 - P1.2 CTRLSEL = 0 */
          UICR_PERIPHCONF_ENTRY(
             PERIPHCONF_GPIO_PIN_CNF_CTRLSEL(0x5f938400UL, 2, 0)
          );

.. _ug_nrf54h20_ironside_se_uicr_wdtstart:

UICR.WDTSTART
-------------

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART` enables the automatic start of a local watchdog timer before the application core is booted.
This provides early system protection ensuring that the system can recover from early boot failures.

The UICR.WDTSTART configuration consists of three sub-registers:

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART`
  Controls whether the watchdog timer automatic start feature is enabled.

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT0` / :kconfig:option:`CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT1`
  Specifies which watchdog timer instance to configure and start.
  The following are valid values:

  * ``WDT0`` - Use watchdog timer instance 0
  * ``WDT1`` - Use watchdog timer instance 1

:kconfig:option:`CONFIG_GEN_UICR_WDTSTART_CRV`
  Sets the initial Counter Reload Value (CRV) for the watchdog timer.
  This value determines the watchdog timeout period.
  The CRV must be at least 15 (0xF) to ensure proper watchdog operation.

.. _ug_nrf54h20_ironside_uicr_securestorage:

UICR.SECURESTORAGE
------------------

:kconfig:option:`CONFIG_GEN_UICR_SECURESTORAGE` enables the secure storage system used by |ISE| for persistent storage of cryptographic keys and trusted data.
The secure storage is divided into separate partitions for different services and processor domains.
The total size of all configurations specified in UICR.SECURESTORAGE.\* must be aligned to a 4 KB boundary.
For more information, see :ref:`ug_nrf54h20_ironside_se_secure_storage`.

The UICR.SECURESTORAGE configuration consists of the following sub-registers:

UICR.SECURESTORAGE.ENABLE
  Controls whether the secure storage feature is enabled.

UICR.SECURESTORAGE.ADDRESS
  Specifies the start address of the secure storage region in memory.
  This address must be aligned to a 4 KB boundary and must point to a valid memory region that can be used for secure storage.

UICR.SECURESTORAGE.CRYPTO
  Configures partition sizes for the cryptographic service within the secure storage.

  UICR.SECURESTORAGE.CRYPTO.APPLICATIONSIZE1KB
    Sets the size of the ``APPLICATION`` domain partition for cryptographic storage, specified in 1 kiB blocks.

  UICR.SECURESTORAGE.CRYPTO.RADIOCORESIZE1KB
    Sets the size of the ``RADIOCORE`` domain partition for cryptographic storage, specified in 1 kiB blocks.

UICR.SECURESTORAGE.ITS
  Configures partition sizes for the Internal Trusted Storage (ITS) service within the secure storage.

  UICR.SECURESTORAGE.ITS.APPLICATIONSIZE1KB
    Sets the size of the ``APPLICATION`` domain partition for ITS, specified in 1 kiB blocks.

  UICR.SECURESTORAGE.ITS.RADIOCORESIZE1KB
    Sets the size of the ``RADIOCORE`` domain partition for ITS, specified in 1 kiB blocks.
