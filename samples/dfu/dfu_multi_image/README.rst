.. _dfu_multi_image_sample:

DFU Multi-Image Sample
#################

.. contents::
   :local:
   :depth: 2

This sample demonstrates the use of the Device Firmware Update (DFU) multi image functionality in the |NCS|.
Currently, the only backend it supports are DFU targets for MCUboot.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::


Overview
********

For simplicity, the sample does not include a transport layer for uploading a new image.
Instead, a partition labeled ``dfu_multi_image_helper`` is reserved in the board overlays.

.. note::
   You must manually upload an image to this partition using ``nrfutil device``.


Once the image has been uploaded, you can use the shell commands from the ``dfu_multi_image`` group provided by this sample to test DFU multi image operations.

User interface
**************

The sample's interface is implemented using the shell, which is accessible via the serial port.

Building and running
********************

.. |sample path| replace:: :file:`samples/dfu/dfu_multi_image`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, perform the following steps:


1. |connect_terminal_specific|
#. Reset the development kit.
#. Observe the following message on the terminal of the application core::

   ``Starting dfu_multi_image sample, build time: <BUILD TIME>``

   And the following message on the terminal of the network core::

   ``Network core build time: <BUILD_TIME>``

   ``<BUILD TIME>`` indicates the build time.
   It will be used later to verify the update.

#. Build a second version of the sample.
   For simplicity, it is assumed to be built in the :file:`build_v2` directory.


#. Use ``arm-zephyr-eabi-objcopy`` to link the contents of ``dfu_multi_image.bin`` to the appropriate address:

   .. code-block:: console

      arm-zephyr-eabi-objcopy -I binary -O ihex --change-address <DFU_MULTI_IMAGE_HELPER_ADDRESS> build_v2/dfu_multi_image.bin package_v2.hex

   Replace ``<DFU_MULTI_IMAGE_HELPER_ADDRESS>`` with the address of the ``dfu_multi_image_helper`` partition.
   Specifically, the address values are the following:

   +-------------------+------------------+
   | Development Kit   | Address          |
   +===================+==================+
   | nRF5340 DK       | ``0xb8000``       |
   +-------------------+------------------+

#. Upload the second version of the images to the device using ``nrfutil device``:

   .. code-block:: console

      nrfutil device program --firmware package_v2.hex  --options chip_erase_mode=ERASE_NONE

#. Reset the development kit.

#. Perform the update using one of the following methods:

   * Single-step update, running the following command::

        dfu_multi_image full_update <package_size>

     ``<package_size>`` is the size, in bytes, of the file :file:`build_v2/dfu_multi_image.bin`.

   * Step-by-step update:
     #. Write the image either at once or in chunks:

            * To write the entire image at once, run::

                dfu_multi_image write 0 0 <image_size>

            <image_size> is the size, in bytes, of the file :file:`build_v2/dfu_multi_image.bin`.

        * To write the image in chunks, run the following commands as many times as there are chunks::

                dfu_multi_image write <read_offset> <write_offset> <chunk_size>

             The ``<read_offset>`` is the offset in the ``dfu_multi_image_helper`` partition from which to read the chunk.
             The ``<write_offset>`` is the offset to pass to the dfu_multi_image library, representing the offset in the package.
             This is useful when skipping some of the images.
             See the documentation of the ``dfu_multi_image_write`` function in :file:`dfu_multi_image.h` for more details.

      #. ``dfu_multi_image done success``.
      #. ``dfu_multi_image schedule_update``.
      #. Reboot the device.

#. After the update completes, observe the following message on the terminal of the application core::

   ``Starting dfu_multi_image sample, build time: <BUILD TIME>``

   and the following message on the terminal of the network core::

   ``Network core build time: <BUILD_TIME>``

   The build time should reflect the new version in both cases.

#. (Optional) To make the update permanent, use the ``dfu_mult_image mcuboot_confirm`` command.
   Without this step, the device will revert to the previous image on the next reboot.
   For nRF5340 the radio core is confirmed automaticall, which is not the case for the application core.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_dfu_target`
* :ref:`lib_dfu_multi_image`
* MCUBOOT
