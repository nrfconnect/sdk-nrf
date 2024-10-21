.. _ug_nrf54h20_suit_push:

How to push SUIT payloads to multiple partitions
################################################

.. contents::
   :local:
   :depth: 2

In the Software Updates for Internet of Things (SUIT), you can push certain payloads separately from the SUIT envelope.
The envelope can find them in the device and use them as part of the firmware upgrade process.

In Nordic Semiconductor's implementation, this functionality is provided by a mechanism called *DFU cache partitions*.

This guide explains how DFU cache partitions work, and how to configure the build system and SUIT manifests to use them for pushing detached payloads.


Reasons to push images separately from the SUIT envelope
********************************************************

Pushing images separately from the SUIT envelope can be useful in the following scenarios:

* Pushing parts of an image to external memory.
* Storing portions of the update package in non-contiguous memory areas, separated by other data.
* Reducing the amount of data that needs to be resent after a communication failure, allowing the update to continue.
* Sending different parts of the image through multiple communication channels.

DFU Cache Partitions
********************

The payloads pushed to the device must be placed in so called “DFU cache partitions”, numbered 0..n.
The DFU cache partitions hold data using a CBOR map format, where the keys are URI strings and the values are binary images.

When the Secure Domain processes the manifest and encounters a ``suit-directive-fetch`` directive it first checks if a given URI is present in its ``suit-integrated-payloads`` section.
If it is not, it goes through all the DFU cache partitions defined in the device.
If the given URI is found as a map key, the binary data stored in the value field corresponding to that URI is fetched.

The DFU cache partition 0 is always present in the device.
It takes all of the remaining space from the ``dfu_partition`` that is not occupied by the envelope.
This partition has a limitation:
it can only be used after storing the SUIT envelope.

The DFU cache partitions from ``1`` to ``n`` are defined in the devicetree, by using the ``dfu_cache_partition_x`` node name, where x is the partition number.
The partitions can be placed both in the internal MRAM as well as in the external flash.

Extracting images
*****************

The :ref:`nrf54h_suit_sample` sample uses the SMP protocol for uploading new envelopes and is by default configured to use the push model for firmware upgrades.
To reconfigure the sample to allow for pushing images into DFU cache partitions, complete the following steps:

1. Enable the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_PUSH_TO_CACHE` Kconfig option.
   This enables the writing to the DFU cache partitions.
   Alternatively, you can enable the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL` option to enable the SUIT envelope processing in the application firmware.
   This will enable a superset of options enabled by :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_PUSH_TO_CACHE`, but will occupy more space in both MRAM and RAM memories.

#. If you intend to use any other cache partition than 0, add the DFU cache partition in the appropriate memory area in the device tree overlay:

   .. code-block:: dts

		dfu_cache_partition_N: partition@a000 {
			reg = <0xa000 DT_SIZE_K(1024)>;
		};

   Replace ``N`` with the partition number, ``a000`` with the proper offset inside the memory area, and ``1024`` with the proper size of the cache partition.

   For example, you could add the cache partition 1 with the size of 1024 kilobytes in the external flash at an offset of 0 by adding:

   .. code-block:: dts

      &mx25uw63 {
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

#. For each image that you want to push separately to the device, do the following:

   * Enable the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE` Kconfig option.
   * Optionally, modify :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_PARTITION` to select the partition where the image will be pushed (default is partition 1).
   * Optionally, modify the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI` to modify the URI used as key for the given image in the DFU cache.

#. Ensure that the URI used by the ``suit-directive-fetch`` command to fetch a given image matches the :kconfig:option:`CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI` Kconfig option.
   This is done by default when using the manifest templates provided by Nordic Semiconductor.
   For the application image URI, you can do that as follows (assuming the target name ``application`` for the image):

   .. code-block:: yaml

      - suit-directive-override-parameters:
         suit-parameter-uri: "{{ application['config']['CONFIG_SUIT_DFU_CACHE_EXTRACT_IMAGE_URI'] }}"
      - suit-directive-fetch:
        - suit-send-record-failure

#. Ensure that the envelope does not integrate the given image inside the envelope integrated payloads section.
   This is ensured by default when using the provided default SUIT envelope templates.


Pushing the images to device
****************************

See the :ref:`SUIT SMP Sample documentation <nrf54h_suit_sample>` for an example of how to push an image to a device.
