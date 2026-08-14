.. _tac5112_sample:

TAC5112 stereo audio codec
##########################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to configure and drive a Texas Instruments TAC5112 low-power stereo audio codec over I2C, using the Zephyr :ref:`audio codec API <zephyr:audio_codec_api>`.
When an audio data path is available, the sample also plays a 1 kHz test tone over TDM/I2S and runs an ADC-to-DAC loopback.

The sample uses only the public codec API, so the same source runs unmodified against any codec that provides the ``ti,tac5112`` compatible.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample requires the `nRF Audio EB`_ (This needs to point to the release page for the nRF Audio EB once publicly released), which carries the TAC5112 codec and connects to the development kit expansion header.

Overview
********

The sample brings up the codec and runs the following demonstration sequence:

1. Register a fault callback with :c:func:`audio_codec_register_error_callback`.
#. Configure the audio serial interface as an I2S target at 48 kHz, stereo, 16-bit.
   This also brings the codec out of reset and sleep, and runs the recommended automatic PLL and clock configuration.
#. Set the output volume to 0 dB and unmute.
#. Power up the DAC with :c:func:`audio_codec_start_output`.
#. Stream a 1 kHz test tone for approximately 1 second, when an audio data path is wired.
#. Run an ADC-to-DAC loopback for approximately 10 seconds, passing audio captured on the codec inputs back out to its outputs.
   During the loopback, the sample ramps the output volume (0 → -40 → 0 dB) and cycles mute and unmute, so those controls are audible on the live signal.
#. Power the DAC down with :c:func:`audio_codec_stop_output` and clear any latched faults.

When no audio data path is present, the sample runs a control-only variant of the demo: it exercises the volume ramp (0 → -20 → 0 dB) and a mute and unmute cycle over I2C, and skips the tone and loopback.

Audio data path
===============

The tone and loopback run only when the devicetree provides a ``tac5112-i2s`` alias that points at the audio serial peripheral wired to the codec.
On the nRF54LM20 DK, the nRF Audio EB shield enables the SoC TDM node and sets this alias, equivalent to the following devicetree:

.. code-block:: devicetree

   &tdm {
       status = "okay";
       pinctrl-0 = <&tdm_tac5112_default>;
       pinctrl-1 = <&tdm_tac5112_sleep>;
       pinctrl-names = "default", "sleep";
   };

   / {
       aliases {
           tac5112-i2s = &tdm;
       };
   };

The audio path uses the ``nordic,nrf-tdm`` peripheral, driven through the standard Zephyr I2S API.
The nRF TDM is the clock controller and the codec is the ASI target, deriving its clocks from BCLK through its PLL, so no MCLK is required.
The loopback captures on the TDM RX line (codec ADC) and sends the same buffers straight back on the TDM TX line (codec DAC), sharing one BCLK and FSYNC.

If the ``tac5112-i2s`` alias is absent, the audio path code compiles out and the sample runs the control-only demo described in `Overview`_.

Fault handling
==============

The codec asserts its interrupt on GPIO1, wired to the ``fault-gpios`` line (active low).
The driver enables the output short-circuit and virtual-ground fault interrupts in :c:func:`audio_codec_configure` and services the interrupt from its workqueue.
The sample registers ``codec_fault_handler()``, which logs the reported faults (over-current, DC/virtual-ground, and under-voltage) and calls :c:func:`audio_codec_clear_errors` to acknowledge them.

Pin assignments
===============

The nRF Audio EB provides the following connections on the nRF54LM20 DK expansion header (P17).
Refer to the shield definition for the authoritative wiring; only the fault line is fixed by this sample.

.. list-table::
   :header-rows: 1

   * - Bus
     - Signal
     - EXP pin
     - nRF54LM20 GPIO
   * - I2C
     - SCL
     - F0
     - P1.07
   * - I2C
     - SDA
     - F1
     - P1.06
   * - TDM
     - BCLK
     - C1
     - P1.03
   * - TDM
     - FSYNC/WCLK
     - C0
     - P1.00
   * - TDM
     - Codec DIN
     - C3
     - P1.13
   * - TDM
     - Codec DOUT
     - C4
     - P1.05
   * - IRQ
     - Codec INT
     - D2
     - P3.02 (``fault-gpios``, active low)

Configuration
*************

The sample enables the codec through the following Kconfig options, set in :file:`prj.conf`:

* :kconfig:option:`CONFIG_AUDIO`
* :kconfig:option:`CONFIG_AUDIO_CODEC`
* :kconfig:option:`CONFIG_AUDIO_TAC5112`

The I2C control bus, the GPIO fault line, and the I2S/TDM audio path are enabled with :kconfig:option:`CONFIG_I2C`, :kconfig:option:`CONFIG_GPIO`, and :kconfig:option:`CONFIG_I2S`.

Building and running
********************

.. |sample path| replace:: :file:`samples/drivers/audio/tac5112`

.. include:: /includes/build_and_run.txt

You must specify the nRF Audio EB as a shield at build time.
For example, to build for the nRF54LM20 DK:

.. code-block:: console

   west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp --shield nrf_audio_eb samples/drivers/audio/tac5112

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe that the sample configures the codec and runs the tone and loopback sequence, and that the log output matches the sample output shown below.
#. During the loopback, verify that the volume changes and the mute and unmute transitions are audible on the codec outputs.

Sample output
-------------

The sample shows the following output:

.. code-block:: console

   *** Booting nRF Connect SDK ... ***
   <inf> tac5112_sample: TAC5112 audio codec sample
   <inf> tac5112_sample: codec configured: I2S target, 48000 Hz, 16-bit, 2 ch
   <inf> tac5112_sample: output enabled
   <inf> tac5112_sample: streaming 1000 Hz tone for ~1 s
   <inf> tac5112_sample: ADC->DAC loopback for 10 s (varying output volume + mute)
   <inf> tac5112_sample: loopback: output volume 0 dB
   <inf> tac5112_sample: loopback: output volume -10 dB
   <inf> tac5112_sample: loopback: output volume -20 dB
   <inf> tac5112_sample: loopback: output volume -30 dB
   <inf> tac5112_sample: loopback: output volume -40 dB
   <inf> tac5112_sample: loopback: output volume -30 dB
   <inf> tac5112_sample: loopback: output volume -20 dB
   <inf> tac5112_sample: loopback: output volume -10 dB
   <inf> tac5112_sample: loopback: mute
   <inf> tac5112_sample: loopback: unmute
   <inf> tac5112_sample: loopback: output volume 0 dB
   <inf> tac5112_sample: output disabled
   <inf> tac5112_sample: TAC5112 sample complete

Dependencies
************

This sample uses the following Zephyr APIs:

* :ref:`zephyr:audio_codec_api`
* :ref:`zephyr:i2c_api`
* :ref:`zephyr:i2s_api`
* :ref:`zephyr:gpio_api`

.. _nRF Audio EB: https://www.nordicsemi.com/
