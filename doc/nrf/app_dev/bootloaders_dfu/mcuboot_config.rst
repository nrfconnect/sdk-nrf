.. _mcuboot_config:

MCUboot Devicetree configuration
################################

.. contents::
   :local:
   :depth: 2

There are instances, in which it is crucial to synchronize the MCUboot configuration with other images.
A simple example of such situation is the partitioning scheme, where the primary and secondary slots must be defined in the MCUboot image scope and also must be present in the application image scope, so the update candidate binaries are transferred to the appropriate memory areas.
Another example of such parameter are UUID values, used to identify the images - the value passed to the signing script must be exactly the same as the UUID expected by the bootloader code.

There are two ways of providing configiration parameters to the MCUboot image:
 - Kconfig options, which are commonly used to configure software components
 - Devicetree, which is commonly used to describe the hardware configuration of the system

Since the partitioning scheme is heavily dependent on the memory layout, defined in the Devicetree, it is more consistent to add the MCUboot configuration to the Devicetree as well, so all the parameters related to the memory layout are defined in one place.
Once the MCUboot images are listed in the MCUboot configuation node, it is possible to extend those entries with additional parameters, such as UUID values.

The MCuboot is not the only bootloader solution provided in the NCS.
To support further use cases, the MCUboot configuration node is not defined directly, but in a bootchain conffiguration node, allowing to describe multi-stage booting scenarios in the future.

Bootchain configuration node
****************************

The bootchain configuration node is optional and must be defined in the root scope of the Devicetree.

For example, the following node describes a bootchain with a single stage, which is the MCUboot:

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

The MCUboot configuration node must be defined as a child of the bootchain configuration node, and it must have the compatible property set to ``nordic,mcuboot``.

The configuration node must point to the MCUboot code partition using the ``partitions`` property, contain a list of images, and may enable additional features.

The list of images is defined as a child node of the MCUboot configuration node.
Each image node must have the compatible property set to ``nordic,mcuboot-image`` contain the following properties:

* ``image-index`` - the index of the image in the MCUboot image list. The image with the index 0 is the image that is directly booted by the MCUboot.
* ``partitions`` - a list of partitions assigned to the image. The first partition in the list is the primary slot, and the second partition is the secondary slot.

The following example shows the MCUboot configuration node with a single image and the code partition assigned to the ``boot_partition``:

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

The UUID checks can be enabled by defining the following optional properties in the MCUboot configuration node:

* ``uuid-vid-required`` - enables image vendor UUID checks.
  If enabled, all images must contain the vendor UUID TLV with the value representing the vendor of the image.

* ``uuid-cid-required`` - enables image class UUID checks.
  If enabled, all images must contain the class UUID TLV with the value representing the class of the image.

By enabling the UUID checks, all entries in the image list must be extended with additional properties:

* ``uuid-vid`` - A list of the vendor UUID values or vendor domain name to calculate the vendor UUID assigned to the image. The list must contain a value for each image slot (usually two).

* ``uuid-cid`` - A list of the class UUID values or class string to calculate the class UUID assigned to the image. The list must contain a value for each image slot (usually two).
  The main purpose for having multiple class UUID values is to distinguish between the slots in the Direct XIP modes.
  When using the SWAP modes, those values must be the same for both slots, so the image will be accepted as the update candidate (slot 1) as well as the bootable image (slot 0).

The following example shows the MCUboot configuration node with the vendor and class UUID checks enabled:

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

To use the Devicetree configuration, it is necessary to include the bootchain configuration node in all of the images.
The easiest way to do it is to define this node in the ``dts`` directory of the application and include in the the overlay files, dedicated for each image.

For example, the following node can be defined in the ``dts/bootchain.dtsi`` file:

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

Then, it can be included in the ``boards/<board_name>/overlay`` file for the main image:

.. code-block:: devicetree

   #include "../dts/bootchain.dtsi"

As well as in the radio image overlay file, for example, in the ``sysbuild/ipc_radio.overlay`` file for the ``ipc_radio`` image:

.. code-block:: devicetree

   #include "../dts/bootchain.dtsi"
