.. _nrf_audio_eb_smoke:

nRF-Audio-EB smoke test
#######################

.. contents::
   :local:
   :depth: 2

The nRF-Audio-EB smoke test brings up every interface that the :ref:`nrf_audio_eb` shield enables on the current development kit and prints a result line for each.
It verifies wiring and driver bring-up, not audio quality.

Overview
********

The sample walks the shield's interfaces in sequence over the console:

* Blinks the RGB LED (red, green, blue).
* Reads the line-in-detect input.
* Probes the TAC5112 codec and reports whether it acknowledges.
* Mounts a FAT file system on the SD card, then prints its size and root listing.
* Captures one block from the PDM microphone and reports the peak amplitude.
* Configures the audio serial port and clocks out a test tone.

The sample is board-neutral: it addresses the audio serial port through the ``i2s-codec-tx`` alias and the microphone through the ``dmic0`` alias, and each interface is compiled out on kits whose shield overlay does not provide it.
On both nRF5340 kits the shield exposes only the codec and the audio serial port, so the LED, line-in, PDM and SD steps are skipped there.

Because there is no in-tree TAC5112 driver, the audio step only proves that the serial port initializes and drives BCLK, LRCK and SDOUT.
Audible output requires configuring the codec registers, which the sample does not do.

Requirements
************

The sample supports the following development kits, each fitted with the nRF-Audio-EB and built with ``SHIELD=nrf_audio_eb``.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Development kit
     - Board target
   * - nRF54LM20 DK
     - ``nrf54lm20dk/nrf54lm20/cpuapp``
   * - nRF5340 Audio DK
     - ``nrf5340_audio_dk/nrf5340/cpuapp``
   * - nRF5340 DK
     - ``nrf5340dk/nrf5340/cpuapp``

See the :ref:`nrf_audio_eb` shield documentation for the per-kit connector and pin assignments.

Building and running
********************

The shield and the ``ti,tac5112`` bindings are hosted in this out-of-tree repository, so point the build at the repository root with both ``BOARD_ROOT`` and ``DTS_ROOT``.

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20/cpuapp samples/audio_eb_smoke -- \
       -DBOARD_ROOT=/home/grwa/shield -DDTS_ROOT=/home/grwa/shield -DSHIELD=nrf_audio_eb

Substitute ``nrf5340_audio_dk/nrf5340/cpuapp`` or ``nrf5340dk/nrf5340/cpuapp`` for the other kits.

Testing
=======

After programming the sample, complete the following steps:

1. Connect to the kit with a terminal emulator at 115200 baud.
2. Reset the kit.
3. Observe the result lines printed for each interface.

The expected output on the nRF54LM20 DK, with an SD card and microphone present, is similar to the following:

.. code-block:: console

   [LED]  RGB blink ... done
   [LINE] logical level = 0 (deasserted)
   [I2C]  ACK: codec present (page reg = 0x00)
   [SD]   mount /SD: (SPI mode, 1-bit)
     mounted: 30432768 KB total, 30432000 KB free
   [PDM]  captured 3200 bytes, peak amplitude = 742
   [I2S]  TX ok ...

On the nRF5340 kits, the LED, line-in, PDM and SD lines report that the interface is not present on the board.

Dependencies
************

This sample uses the following Zephyr APIs and subsystems:

* :ref:`GPIO <zephyr:gpio_api>`
* :ref:`I2C <zephyr:i2c_api>`
* :ref:`SPI <zephyr:spi_api>`
* :ref:`I2S <zephyr:i2s_api>`
* The DMIC (PDM) audio API (``include/zephyr/audio/dmic.h``)
* :ref:`File system <zephyr:file_system_api>` with FAT and the SDMMC disk driver (SD card over SPI)
