.. _dfu_target_sample:

DFU Target Sample
#################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the use of the DFU target functionality in the |NCS|.
Currently, it only supports DFU target for mcuboot.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

``nrfutil device`` is recommended to upload a new image to the device.

Overview
********

The sample demonstrates how to use the DFU target functionality in the |NCS|.

For simplicity, it does not include a transport layer for uploading a new image.
Instead, a partition labeled ``dfu_target_helper`` is reserved in the board overlays.
You must manually upload an image to this partition using a tool like ``nrfutil device``.

Once the image is uploaded, the shell commands from the ``dfu_target`` group provided by this sample can be used to test DFU target operations.

User interface
**************

The sample's interface is implemented using the shell, accessible via the serial port.

Building and running
********************

The DFU target sample is built the same way as any other |NCS| application or sample.

.. |sample path| replace:: :file:`samples/dfu/dfu_target`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit:


1. |connect_terminal_specific|
#. Reset the development kit.
#. Observe the following message on the terminal::

   ``Starting dfu_target sample, build time: <BUILD TIME>``

   Note the build time, as it will be used later to verify the update.

#. Build a second version of the sample.
   For simplicity it is assumed it is built in the ``build_v2`` directory.

#. Use ``arm-zephyr-eabi-objcopy`` to link the contents of ``zephyr.signed.bin`` to the appropriate address:

   .. code-block:: console

      arm-zephyr-eabi-objcopy -I binary -O ihex --change-address <DFU_TARGET_HELPER_ADDRESS> build_v2/dfu_target/zephyr/zephyr.signed.bin  image_v2.hex

   Replace ``<DFU_TARGET_HELPER_ADDRESS>`` with the address of the ``dfu_target_helper`` partition, found in the board overlay file.
   Specifically, the address values are:

   +-------------------+------------------+
   | Development Kit   | Address          |
   +===================+==================+
   | nRF52840 DK       | ``0xa8000``      |
   +-------------------+------------------+

#. Upload the second version of the image to the device using ``nrfutil device``:

   .. code-block:: console

      nrfutil device program --firmware image_v2.hex  --options chip_erase_mode=ERASE_NONE

#. Reset the development kit.

#. Perform the update using one of the following methods:

   * Single-step update:
      Use the ``dfu_target full_update`` command: ``dfu_target full_update <image_size>``, where ``<image_size>`` is the size of :file:`build_v2/dfu_target/zephyr/zephyr.signed.bin` in bytes.
   * Step-by-step update:
      a. Run ``dfu_target image_type`` to get the image type (should return 1 for MCUBOOT).
      #. ``dfu_target init <image_type> <image_size>``, where ``<image_size>`` is the size of :file:`build_v2/dfu_target/zephyr/zephyr.signed.bin` in bytes.
      #. ``dfu_target write <offset> <chunk_size>`` can be performed multiple times to write the image in chunks.
         The ``<offset>`` is the offset in the ``dfu_target_helper`` partition from which to read the chunk.
         To write the entire image at once, use ``dfu_target write 0 <image_size>``.
      #. ``dfu_target done success``.
      #. ``dfu_target schedule_update``.
      #. Reboot the device.

#. After the update completes, observe the following message on the terminal::

   ``Starting dfu_target sample, build time: <BUILD TIME>``

   The build time should reflect the new version.

#. (Optional) To make the update permanent, use the ``dfu_target mcuboot_confirm`` command.
   Without this step, the device will revert to the previous image on the next reboot.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_dfu_target`
* MCUBOOT
