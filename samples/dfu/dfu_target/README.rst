.. _dfu_target_sample:

DFU Target
##########

.. contents::
   :local:
   :depth: 2

This sample demonstrates the use of the Device Firmware Update (DFU) target functionality in the |NCS|.
Currently, it only supports DFU targets for MCUboot.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::


Overview
********

For simplicity, the sample does not include a transport layer for uploading a new image.
Instead, a partition labeled ``dfu_target_helper`` is reserved in the board overlays.

.. note::
   You must manually upload an image to this partition using ``nrfutil device``.


Once the image has been uploaded, you can use the shell commands from the ``dfu_target`` group provided by this sample to test DFU target operations.

User interface
**************

The sample's interface is implemented using the shell, which is accessible via the serial port.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/dfu_target`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, perform the following steps:


1. |connect_terminal_specific|
#. Reset the development kit.
#. Observe the following message on the terminal::

   ``Starting dfu_target sample, build time: <BUILD TIME>``

   ``<BUILD TIME>`` indicates the build time.
   It will be used later to verify the update.

#. Build a second version of the sample.
   For simplicity, it is assumed to be built in the :file:`build_v2` directory.


#. Use ``arm-zephyr-eabi-objcopy`` to link the contents of ``zephyr.signed.bin`` to the appropriate address:

   .. code-block:: console

      arm-zephyr-eabi-objcopy -I binary -O ihex --change-address <DFU_TARGET_HELPER_ADDRESS> build_v2/dfu_target/zephyr/zephyr.signed.bin  image_v2.hex

   Replace ``<DFU_TARGET_HELPER_ADDRESS>`` with the address of the ``dfu_target_helper`` partition, found in the board overlay file.
   Specifically, the address values are the following:

   +-------------------+------------------+
   | Development Kit   | Address          |
   +===================+==================+
   | nRF52840 DK       | ``0xa8000``      |
   +-------------------+------------------+
   | nRF54H20 DK       | ``0xe092000``    |
   +-------------------+------------------+
   | nRF54L15 DK       | ``0xf2000``      |
   +-------------------+------------------+

#. Upload the second version of the image to the device using ``nrfutil device``:

   .. code-block:: console

      nrfutil device program --firmware image_v2.hex  --options chip_erase_mode=ERASE_NONE

#. Reset the development kit.

#. Perform the update using one of the following methods:

   * Single-step update, running the following command::

        dfu_target full_update <image_size>

     ``<image_size>`` is the size, in bytes, of the file :file:`build_v2/dfu_target/zephyr/zephyr.signed.bin`.

   * Step-by-step update:

     #. Get the image type::

           dfu_target image_type

        This command returns ``1`` for MCUBOOT.

     #. Initialize the image::

           dfu_target init <image_type> <image_size>

        <image_size> is the size, in bytes, of the file :file:`build_v2/dfu_target/zephyr/zephyr.signed.bin`.

     #. Write the image either at once or in chunks:

            * To write the entire image at once, run::

                dfu_target write 0 <image_size>

        * To write the image in chunks, run the following commands as many times as there are chunks::

                dfu_target write <offset> <chunk_size>

             The ``<offset>`` is the offset in the ``dfu_target_helper`` partition from which to read the chunk.

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
* :ref:`MCUboot <mcuboot_index_ncs>`
