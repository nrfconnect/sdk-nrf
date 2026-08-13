.. zephyr:code-sample:: tac5112
   :name: TAC5112 stereo audio codec
   :relevant-api: audio_codec_interface

   Configure and drive a TI TAC5112 audio codec over I2C, with a TDM/I2S
   tone and an ADC-to-DAC loopback.

Overview
********

This sample uses the Zephyr :ref:`audio codec API <audio_codec_api>` to
bring up a Texas Instruments **TAC5112** low-power stereo audio codec and
run a short demonstration:

#. register a fault callback,
#. configure the audio serial interface (I2S target, 48 kHz, stereo,
   16-bit) -- which also releases the codec from reset/sleep and runs the
   recommended automatic PLL/clock configuration,
#. set output volume to 0 dB and unmute,
#. power up the DAC (``audio_codec_start_output``),
#. stream a 1 kHz test tone for ~1 s (when an audio data path is wired),
#. run an ADC-to-DAC **loopback** for ~10 s -- audio captured on the codec
   inputs is passed back out to its outputs -- while ramping the output
   volume (0 -> –40 -> 0 dB) and cycling mute/unmute so those controls are
   audible on the live signal,
#. power the DAC down (``audio_codec_stop_output``).

The codec is selected from the devicetree by its ``ti,tac5112`` compatible,
so the same application runs against real hardware or the software mock.
Only the public codec API is used.

Requirements
************

* A board with a TAC5112 on an I2C bus (and, for the tone/loopback, an
  audio serial bus), **or** ``native_sim`` with the TAC5112 I2C emulator.
* Overlays shipped with this sample:

  * ``nrf54lm20dk/nrf54lm20a/cpuapp`` -- a physical codec (for example the nRF
    Audio EB on the expansion header) controlled over ``&i2c22`` with the
    audio data path on the SoC's **TDM** peripheral. See the pin map below.

The TAC5112 driver, its ``ti,tac5112`` binding, and (for ``native_sim``)
the ``CONFIG_AUDIO_TAC5112_EMUL`` mock must be present in your tree.

nRF54LM20 DK + nRF Audio EB pin map
===================================

As wired by :file:`boards/nrf54lm20dk_nrf54lm20a_cpuapp.overlay`, via the
DK expansion header (P17). Adjust if your board/EB differs.

======  ===============  =========  ==========================
Bus     Signal           EXP pin    nRF54LM20 GPIO
======  ===============  =========  ==========================
I2C     SCL              F0         P1.07  (``i2c22``, addr 0x50)
I2C     SDA              F1         P1.06
TDM     BCLK             C1         P1.03  (``TDM_SCK_M``)
TDM     FSYNC/WCLK       C0         P1.00  (``TDM_FSYNC_M``)
TDM     codec DIN        C3         P1.13  (``TDM_SDOUT``)
TDM     codec DOUT       C4         P1.05  (``TDM_SDIN``)
IRQ     codec INT        D2         P3.02  (``fault-gpios``, active low)
======  ===============  =========  ==========================

Notes:

* The codec ADDR pin is strapped to ground, giving 7-bit I2C address 0x50.
* The audio path uses the ``nordic,nrf-tdm`` peripheral (node label
  ``tdm``), driven through the standard Zephyr I2S API. The nRF TDM is the
  clock controller (master); the codec is the ASI target and derives its
  clocks from BCLK via its PLL, so no MCLK is required.
* The codec emits its interrupt on GPIO1, wired to EXP D2 -> P3.02. The
  driver enables the output short-circuit / virtual-ground fault
  interrupts in ``audio_codec_configure()`` and services the IRQ from its
  workqueue; the sample registers ``codec_fault_handler()``, which logs the
  fault and calls ``audio_codec_clear_errors()`` to acknowledge it.
  (Clock-error interrupts are left masked for now.)

Building and running
********************

native_sim (no hardware)
========================

.. code-block:: console

   west build -p always -b native_sim samples/drivers/audio/tac5112
   west build -t run

Runs the control-only path against the mock (configure, volume ramp,
mute/unmute); the tone and loopback are skipped (no audio path).

Audio data path (TDM on nRF54LM20)
***************************************************
The tone and loopback run only when the device tree provides a
``tac5112-i2s`` alias pointing at the audio serial peripheral wired to the
codec. The nRF54LM20 DK overlay sets this to the SoC's TDM node::

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

with a ``pinctrl`` group assigning ``TDM_SCK_M`` / ``TDM_FSYNC_M`` /
``TDM_SDOUT`` / ``TDM_SDIN`` to the pins in the table above.

Sample output
*************

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
   <inf> tac5112_sample: loopback: mute
   <inf> tac5112_sample: loopback: unmute
   <inf> tac5112_sample: output disabled
   <inf> tac5112_sample: TAC5112 sample complete
