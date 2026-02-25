.. _tfm_pm_to_dts_migration:

Migrating from pm_static.yml to DTS Overlays
#############################################

This guide shows how to translate a ``pm_static.yml`` file into an
equivalent DTS overlay, using a real-world Matter sample as an example.

Why Migrate?
************

The Partition Manager (``pm_static.yml``) is being phased out in favor of
standard Zephyr devicetree-based partition definitions. DTS overlays are:

- Portable across Zephyr ecosystem
- Validated by the DTS compiler at build time
- Easier to review in code review (no YAML anchors/aliases)
- Consistent with upstream Zephyr partition handling

Step-by-Step Translation
************************

Consider a ``pm_static_nrf54l15dk_nrf54l15_cpuapp.yml`` like this:

.. code-block:: yaml

   mcuboot:
     address: 0x0
     region: flash_primary
     size: 0xD000                    # 52 kB
   mcuboot_pad:
     address: 0xD000
     size: 0x800                     # 2 kB padding
   app:
     address: 0xD800
     size: 0x164800                  # ~1426 kB
   mcuboot_primary:
     span: [mcuboot_pad, app]
     address: 0xD000
     size: 0x165000                  # ~1428 kB
   factory_data:
     address: 0x172000
     size: 0x1000                    # 4 kB
   settings_storage:
     address: 0x173000
     size: 0xA000                    # 40 kB
   mcuboot_secondary:
     address: 0x0
     region: external_flash
     size: 0x165000

The equivalent DTS overlay:

.. code-block:: devicetree

   &cpuapp_rram {
       partitions {
           compatible = "fixed-partitions";
           #address-cells = <1>;
           #size-cells = <1>;

           /* mcuboot → boot_partition */
           boot_partition: partition@0 {
               label = "mcuboot";
               reg = <0x00000000 DT_SIZE_K(52)>;
           };

           /*
            * mcuboot_pad is NOT represented in DTS.
            * It's handled internally by MCUboot sysbuild.
            *
            * app → slot0_ns_partition
            * (In TF-M builds, slot0_partition is TF-M secure,
            *  slot0_ns_partition is the NS application)
            */
           slot0_ns_partition: partition@d000 {
               label = "image-0-nonsecure";
               reg = <0x0000D000 0x164800>;
           };

           /* factory_data → factory_partition */
           factory_partition: partition@172000 {
               label = "factory-data";
               reg = <0x00172000 DT_SIZE_K(4)>;
           };

           /* settings_storage → storage_partition */
           storage_partition: partition@173000 {
               label = "storage";
               reg = <0x00173000 DT_SIZE_K(40)>;
           };
       };
   };

   /* mcuboot_secondary on external flash */
   &mx25r64 {
       partitions {
           compatible = "fixed-partitions";
           #address-cells = <1>;
           #size-cells = <1>;

           slot1_partition: partition@0 {
               label = "image-1";
               reg = <0x00000000 0x165000>;
           };
       };
   };

Translation Rules
*****************

.. list-table::
   :header-rows: 1
   :widths: 30 30 40

   * - pm_static.yml concept
     - DTS equivalent
     - Notes
   * - ``mcuboot``
     - ``boot_partition``
     - Label: ``"mcuboot"``
   * - ``mcuboot_pad``
     - (none)
     - Handled internally by MCUboot sysbuild; not in DTS
   * - ``app`` / ``mcuboot_primary_app``
     - ``slot0_ns_partition``
     - Label: ``"image-0-nonsecure"``
   * - ``tfm``
     - ``slot0_partition``
     - Label: ``"image-0"``
   * - ``mcuboot_primary``
     - ``span:`` not in DTS
     - The "span" concept doesn't exist in DTS; MCUboot
       infers primary = slot0 + slot0_ns from the labels
   * - ``mcuboot_secondary``
     - ``slot1_partition``
     - Label: ``"image-1"``; can be on external flash
   * - ``settings_storage``
     - ``storage_partition``
     - Label: ``"storage"``
   * - ``factory_data``
     - ``factory_partition``
     - Label: ``"factory-data"``
   * - ``tfm_ps``
     - ``tfm_ps_partition``
     - Label: ``"tfm-ps"``
   * - ``tfm_its``
     - ``tfm_its_partition``
     - Label: ``"tfm-its"``
   * - ``tfm_otp_nv_counters``
     - ``tfm_otp_partition``
     - Label: ``"tfm-otp"``
   * - ``region: flash_primary``
     - ``&flash0`` or ``&cpuapp_rram``
     - ``&flash0`` for nRF91/nRF53; ``&cpuapp_rram`` for nRF54L
   * - ``region: external_flash``
     - ``&mx25r64`` (or similar)
     - Reference the actual external flash node
   * - ``size: 0x10000``
     - ``reg = <addr DT_SIZE_K(64)>``
     - Use ``DT_SIZE_K()`` for readability, or raw hex

Common Pitfalls
***************

1. **Forgetting alignment**: pm_static lets the Partition Manager handle
   alignment. In DTS, YOU must ensure addresses respect the TrustZone
   region size (4/16/32 kB depending on SoC).

2. **mcuboot_pad**: Don't add it to DTS. It's automatically handled.

3. **Span partitions**: ``mcuboot_primary`` and ``tfm_secure`` are "span"
   partitions in pm_static that group sub-partitions. DTS doesn't have
   this concept — MCUboot determines slot boundaries from labels.

4. **Hex vs DT_SIZE_K**: Both work in DTS. Use ``DT_SIZE_K()`` for round
   kB values, raw hex for odd sizes like ``0x164800``.

5. **Label names matter**: MCUboot and TF-M look for specific labels
   (``"mcuboot"``, ``"image-0"``, ``"image-0-nonsecure"``, ``"image-1"``,
   ``"tfm-ps"``, ``"tfm-its"``, ``"tfm-otp"``, ``"storage"``). Using
   wrong labels will cause build or runtime failures.
