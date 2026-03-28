.. _mcuboot_config:

MCUboot Devicetree configuration
################################

.. contents::
   :local:
   :depth: 2

In some scenarios, you must synchronize the MCUboot configuration with other images to ensure consistent system behavior.

You can provide configuration parameters to the MCUboot image in two ways:

* Use Kconfig options to configure software components.
* Use Devicetree to describe the hardware configuration of the system.

For memory layout parameters, prefer Devicetree.
This approach keeps all memory related configuration in a single location and improves consistency.

Examples of parameters that require synchronization include the following:

* Partitioning scheme.
  Define the primary and secondary slots in both the MCUboot image and the application image so that update binaries are written to the correct memory areas.
* UUID values.
  Ensure that the value passed to the signing script matches the UUID expected by the bootloader.

After you define MCUboot images in the MCUboot configuration node, you can extend these entries with additional parameters, such as UUID values.

MCUboot is not the only bootloader solution provided in the NCS.
To support additional use cases, define the MCUboot configuration within a bootchain configuration node.
This structure enables multi-stage boot scenarios.

Bootchain configuration node
****************************

The bootchain configuration node is optional.
Define this node in the root scope of the Devicetree.

For example, the following node describes a bootchain with a single stage using MCUboot:

.. code-block:: devicetree

   / {
       bootchain {
           b0_mcuboot {
               compatible = "nordic,mcuboot";

               ...
           };
       };
   };

MCUboot configuration node
**************************

You must define the MCUboot configuration node as a child of the bootchain configuration node.
Set the ``compatible`` property to ``nordic,mcuboot``.

To configure the node to reference the MCUboot code partition, use the ``partitions`` property.
Add a list of images and, if needed, enable additional features.

Define the list of images as child nodes of the MCUboot configuration node.
For each image node, set the ``compatible`` property to ``nordic,mcuboot-image`` and include the following properties:

* ``image-index`` - Specify the index of the image in the MCUboot image list.
  The image with index ``0`` is booted directly by MCUboot.
* ``partitions`` - Specify the list of partitions assigned to the image.
  The first partition is the primary slot.
  The second partition is the secondary slot.

The following example shows an MCUboot configuration node with a single image and the code partition assigned to ``boot_partition``:

.. code-block:: devicetree

   / {
     bootchain {
       b0_mcuboot {
         compatible = "nordic,mcuboot";
         uuid-vid-required;
         uuid-cid-required;
         partitions = <&boot_partition>;

         images {
           application {
             compatible = "nordic,mcuboot-image";
             image-index = <0>;
             partitions = <&slot0_partition &slot1_partition>;
           };
         };
       };
     };
   };

Enabling UUID checks and defining UUID values
=============================================

To enable UUID checks, define the following optional properties in the MCUboot configuration node:

* ``uuid-vid-required`` - Enable vendor UUID validation.
  When enabled, each image must include a vendor UUID TLV that identifies the image vendor.

* ``uuid-cid-required`` - Enable class UUID validation.
  When enabled, each image must include a class UUID TLV that identifies the image class.

When you enable UUID checks, extend each image entry with additional properties:

* ``uuid-vid`` - Provide a list of vendor UUID values or vendor domain names used to derive the vendor UUID assigned to the image.
  The list must contain one value for each image slot, typically two.

* ``uuid-cid`` - Provide a list of class UUID values or class strings used to derive the class UUID assigned to the image.
  The list must contain one value for each image slot, typically two.

  Use different class UUID values to distinguish slots when using direct XIP modes.
  When using swap modes, use the same value for both slots so that the image is accepted as both an update candidate (slot 1) and a bootable image (slot 0).

The following example shows an MCUboot configuration node with vendor and class UUID checks enabled:

.. code-block:: devicetree

   / {
     bootchain {
       b0_mcuboot {
         compatible = "nordic,mcuboot";
         uuid-vid-required;
         uuid-cid-required;
         partitions = <&boot_partition>;

         images {
           application {
             compatible = "nordic,mcuboot-image";
             image-index = <0>;
             uuid-vid = "nordicsemi.com", "nordicsemi.com";
             uuid-cid = "nRF54L15_sample_app", "nRF54L15_sample_app";
             partitions = <&slot0_partition &slot1_partition>;
           };
         };
       };
     };
   };

Enabling bootloader configuration in the application
****************************************************

To use the Devicetree configuration, include the bootchain configuration node in all images.
Define this node in the :file:`dts` directory of the application and include it in overlay files dedicated to each image.

For example, you can define the following node in the :file:`dts/bootchain.dtsi` file:

.. code-block:: devicetree

   / {
     bootchain {
       b0_mcuboot {
         compatible = "nordic,mcuboot";
         uuid-vid-required;
         uuid-cid-required;
         partitions = <&boot_partition>;

         images {
           application {
             compatible = "nordic,mcuboot-image";
             image-index = <0>;
             uuid-vid = "nordicsemi.com", "nordicsemi.com";
             uuid-cid = "nRF54H20_sample_app", "nRF54H20_sample_app";
             partitions = <&slot0_partition &slot1_partition>;
           };

           radio {
             compatible = "nordic,mcuboot-image";
             image-index = <1>;
             uuid-vid = "nordicsemi.com", "nordicsemi.com";
             uuid-cid = "nRF54H20_sample_rad", "nRF54H20_sample_rad";
             partitions = <&slot2_partition &slot3_partition>;
           };
         };
       };
     };
   };

Then, include it in the :file:`boards/<board_name>/overlay` file for the main image:

.. code-block:: devicetree

   #include "../dts/bootchain.dtsi"

Also include it in the radio image overlay file, for example, in the :file:`sysbuild/ipc_radio.overlay` file for the ``ipc_radio`` image:

.. code-block:: devicetree

   #include "../dts/bootchain.dtsi"
