.. _nrf_audio_eb:

nRF-Audio-EB shield
###################

.. contents::
   :local:
   :depth: 2

Overview
********

The nRF-Audio-EB shield brings up the nRF-Audio-EB audio expansion board, which carries a Texas Instruments TAC5112 stereo audio codec.
It is a multi-board shield: the SoC-specific devicetree lives in per-board overlays under :file:`boards/`, so a single shield definition serves several development kits.
The base :file:`nrf_audio_eb.overlay` is a neutral stub; |NCS| applies it for every board and then layers the matching :file:`boards/<board>.overlay` on top.

The shield enables the following interfaces (subject to what each carrier's connector exposes):

* Audio over the Zephyr I2S API, through the ``i2s-codec-tx`` alias (the TDM peripheral on the nRF54LM20, the I2S peripheral on the nRF5340).
* Codec control over I2C, exposed as the ``audio_codec`` node (:dtcompatible:`ti,tac5112`).
* Digital microphone over the DMIC API, through the ``dmic0`` alias (nRF54LM20 only).
* An SD card over SPI (SPI mode, 1-bit), using the SDHC subsystem with :dtcompatible:`zephyr,sdhc-spi-slot` (nRF54LM20 only).
* An RGB LED (:dtcompatible:`gpio-leds`) and a line-in-detect input (:dtcompatible:`gpio-keys`) (nRF54LM20 only).

Requirements
************

This shield supports the following development kits.

.. list-table:: Supported carriers
   :header-rows: 1
   :widths: 30 35 35

   * - Development kit
     - Board target
     - Connector
   * - nRF54LM20 DK
     - ``nrf54lm20dk/nrf54lm20/cpuapp``
     - P17 (2x13 header, mates 1:1)
   * - nRF5340 Audio DK
     - ``nrf5340_audio_dk/nrf5340/cpuapp``
     - P10 (external hardware codec header)
   * - nRF5340 DK
     - ``nrf5340dk/nrf5340/cpuapp``
     - Arduino headers P3 and P4

Audio codec
===========

|NCS| and Zephyr do not provide an in-tree driver for the TAC5112.
The shield ships two bus bindings for the same ``compatible = "ti,tac5112"`` value, and the build system selects the one that matches the parent bus of the ``audio_codec`` node:

* :file:`dts/bindings/audio/ti,tac5112.yaml` for I2C control (used by all three carriers above).
* :file:`dts/bindings/audio/ti,tac5112-spi.yaml` for SPI control (SPI mode 1, that is CPOL = 0 and CPHA = 1, up to 25 MHz), for SPI-wired setups such as the Audio DK P5 debug header.

The application configures the codec registers itself.
Obtain the bus with ``I2C_DT_SPEC_GET(DT_NODELABEL(audio_codec))`` (or ``SPI_DT_SPEC_GET`` for SPI control) and the side-band lines with ``GPIO_DT_SPEC_GET``.
The 7-bit I2C address is ``0x50`` when the ADDR pin is strapped to ground; the other straps give ``0x51``, ``0x52`` or ``0x53``.
The codec runs in PLL mode, recovering its clocks from BCLK, so MCLK is routed but is not used for clocking.

Pin assignments
***************

nRF54LM20 DK (P17)
==================

The nRF-Audio-EB P2 header (2x13 socket) mates 1:1 with the nRF54LM20 DK P17 header.
The power pins align in both (P17 VDD on 1 and 20, GND on 2 and 19), which confirms that P17 pin *N* connects to P2 pin *N*.
All rows below are confirmed against both pinout tables.

.. list-table:: nRF54LM20 DK P17 to nRF-Audio-EB
   :header-rows: 1
   :widths: 12 22 30 36

   * - Pin
     - nRF54LM20 GPIO
     - nRF-Audio-EB signal
     - Shield use
   * - 5
     - P1.02
     - TDM_FSYNC
     - I2S LRCK
   * - 6
     - P1.03
     - TDM_SCK
     - I2S BCLK (dedicated clock pin)
   * - 7
     - P1.04
     - TDM_MCK
     - I2S MCK
   * - 8
     - P1.13
     - TDM_SDIN
     - I2S SDIN (codec to SoC)
   * - 9
     - P3.00
     - SPI_MOSI
     - SD card MOSI (``spi21``)
   * - 10
     - P3.01
     - SPI_MISO
     - SD card MISO (``spi21``)
   * - 11
     - P0.03
     - PDM_CLK
     - PDM clock
   * - 12
     - P0.04
     - PDM_DAT
     - PDM data
   * - 13
     - P3.02
     - SPI_CS
     - SD card chip-select (``spi21`` cs-gpios)
   * - 14
     - P3.03
     - SPI_CLK
     - SD card SCK (``spi21``)
   * - 15
     - P1.07
     - SCL
     - I2C SCL (``i2c22``)
   * - 16
     - P1.06
     - SDA
     - I2C SDA (``i2c22``)
   * - 17
     - P1.05
     - TDM_OUT
     - I2S SDOUT (SoC to codec)
   * - 18
     - P3.06
     - LINE_IN_DETECT
     - Line-in-detect input
   * - 21
     - P2.00
     - CODEC_INT
     - Codec interrupt (gpio2)
   * - 22 - 24
     - P2.01 - P2.03
     - LD1R / LD1G / LD1B
     - RGB LED red / green / blue (gpio2)

Audio uses the TDM peripheral (the nRF54LM20 has no I2S peripheral), driven through the standard I2S API.

The SD card is driven by the **hardware SPIM21** in SPI mode (1-bit), exposed through the SDHC subsystem with :dtcompatible:`zephyr,sdhc-spi-slot`.
Its pins are on port P3: SCK P3.03, MOSI P3.00, MISO P3.01, and a chip-select GPIO on P3.02.
The card is exposed as disk ``"SD"`` through the Disk Access subsystem.

This works because of the nRF54L power domains. Port P2 is the MCU domain, drivable only by ``spi00`` (SPIM00) -- whose 128 MHz base clock and /126 prescaler floor give a minimum SCK of ~1 MHz, too fast for the SD stack's fixed 400 kHz init clock (nrfx returns ``-EINVAL``). Ports P1/P3 are the PERI domain, drivable by the standard-speed SPIMs (16 MHz base), which reach ~127 kHz and therefore 400 kHz.
The revised board routing places the SD bus on P3 so a standard-speed SPIM can drive it directly; the RGB LEDs and codec IRQ move to P2, where plain (CPU-driven) GPIO is unaffected by the domain rule.

.. note::
   P3.00--03 are the DK's ``nordic_expansion_spi`` pins (SPIM22), but SERIAL22
   is used by the codec's ``i2c22``, so the SD card is driven by the free
   ``spi21`` on those P3 pins via pinctrl. The overlay disables ``spi00`` to
   free the P2 pins for the RGB LEDs / codec IRQ. Native 4-bit SD (SDIO) is not
   available (no hardware SDHC).

nRF5340 Audio DK (P10)
======================

P10 is the Audio DK external hardware codec header.
Its audio and side-band pins are the same SoC pins that reach the onboard CS47L63, routed through the P0.21 HW_CODEC_SELECT multiplexer; P10 additionally breaks out the nRF I2C bus for codec control.
GPIO assignments are taken from the nRF5340 Audio DK Hardware User Guide and the PCA10121 schematic.

.. list-table:: nRF5340 Audio DK P10 to nRF-Audio-EB
   :header-rows: 1
   :widths: 12 22 22 44

   * - Pin
     - Signal
     - nRF5340 GPIO
     - Shield use
   * - 3
     - RESET
     - P0.18
     - Codec ``reset-gpios`` (active low)
   * - 4
     - IRQ
     - P0.19
     - Codec ``int-gpios`` (active low)
   * - 5
     - GPIO
     - P0.20
     - Codec ``codec-gpios``
   * - 6
     - MCLK
     - P0.12
     - I2S MCK
   * - 7
     - BCLK
     - P0.14
     - I2S SCK
   * - 8
     - FSYNC
     - P0.16
     - I2S LRCK
   * - 9
     - DOUT
     - P0.15
     - I2S SDIN (codec to SoC)
   * - 10
     - DIN
     - P0.13
     - I2S SDOUT (SoC to codec)
   * - 15
     - SDA
     - P1.02
     - I2C SDA (``i2c1``)
   * - 16
     - SCL
     - P1.03
     - I2C SCL (``i2c1``)

The overlay removes the onboard CS47L63, places the TAC5112 on ``i2c1`` at ``0x50`` (clear of the board's INA231 monitors at 0x40/0x41/0x44/0x45 and the microphone at 0x60), reuses ``i2s0`` unchanged, and flips the P0.21 HW_CODEC_SELECT hog from ``output-low`` to ``output-high`` to select the external codec.
P10 pins 11 to 14 (CS, MISO, SCK, MOSI) are the alternative SPI-control path and are unused for I2C control.

nRF5340 DK (P3 and P4)
======================

The nRF5340 DK has no codec connector, so the nRF-Audio-EB is wired to the Arduino digital headers P3 (D0 to D7) and P4 (D8 to D15).

.. caution::
   Because the nRF-Audio-EB is a 2x13 socket, connecting it to the Arduino headers requires an adapter or harness.
   Which nRF-Audio-EB signal reaches which Arduino pin is therefore defined by that adapter, so the signal-to-pin rows below must be verified against the actual wiring.
   The nRF5340 GPIO values are the confirmed Arduino-header nexus of the board.

.. list-table:: nRF5340 DK Arduino P3/P4 to nRF-Audio-EB
   :header-rows: 1
   :widths: 26 14 14 22 24

   * - nRF-Audio-EB signal
     - Arduino
     - Header
     - nRF5340 GPIO
     - Shield use
   * - TDM_MCK
     - D2
     - P3
     - P1.04
     - I2S MCK
   * - TDM_SCK
     - D3
     - P3
     - P1.05
     - I2S SCK
   * - TDM_FSYNC
     - D4
     - P3
     - P1.06
     - I2S LRCK
   * - TDM_SDIN
     - D5
     - P3
     - P1.07
     - I2S SDIN
   * - TDM_OUT
     - D6
     - P3
     - P1.08
     - I2S SDOUT
   * - CODEC_RESET
     - D7
     - P3
     - P1.09
     - Codec ``reset-gpios``
   * - CODEC_INT
     - D8
     - P4
     - P1.10
     - Codec ``int-gpios``
   * - CODEC_GPIO
     - D9
     - P4
     - P1.11
     - Codec ``codec-gpios``
   * - SDA
     - D14
     - P4
     - P1.02
     - I2C SDA (``i2c1``)
   * - SCL
     - D15
     - P4
     - P1.03
     - I2C SCL (``i2c1``)

Usage
*****

Set ``SHIELD`` to ``nrf_audio_eb`` when building an application.
Because the shield and the codec bindings are hosted in this out-of-tree repository, point the build at the repository root with both ``BOARD_ROOT`` (for the shield) and ``DTS_ROOT`` (for the ``ti,tac5112`` bindings).

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20/cpuapp -- \
       -DBOARD_ROOT=/home/grwa/shield -DDTS_ROOT=/home/grwa/shield -DSHIELD=nrf_audio_eb

   west build -b nrf5340_audio_dk/nrf5340/cpuapp -- \
       -DBOARD_ROOT=/home/grwa/shield -DDTS_ROOT=/home/grwa/shield -DSHIELD=nrf_audio_eb

   west build -b nrf5340dk/nrf5340/cpuapp -- \
       -DBOARD_ROOT=/home/grwa/shield -DDTS_ROOT=/home/grwa/shield -DSHIELD=nrf_audio_eb

The per-board overlay file name must match the board target, in the form :file:`<board>_<soc>_<core>.overlay`.

The shield's :file:`Kconfig.defconfig` enables :kconfig:option:`CONFIG_I2S`, :kconfig:option:`CONFIG_I2C`, :kconfig:option:`CONFIG_SPI`, :kconfig:option:`CONFIG_DISK_ACCESS` and :kconfig:option:`CONFIG_DISK_DRIVER_SDMMC`.
To mount a FAT file system on the SD card, add :kconfig:option:`CONFIG_FILE_SYSTEM` and :kconfig:option:`CONFIG_FAT_FILESYSTEM_ELM` in your application.

.. note::
   The SD card runs in SPI mode (1-bit). Native 4-bit SD (SDIO) is not
   available on the nRF54LM20: there is no hardware SDHC controller, and the
   FLPR sQSPI soft peripheral is a flash/PSRAM MSPI controller, not an SD host.

References
**********

- `nRF5340 Audio DK product page <https://www.nordicsemi.com/Products/Development-hardware/nRF5340-Audio-DK>`_
- `nRF5340 DK product page <https://www.nordicsemi.com/Products/Development-hardware/nRF5340-DK>`_
- `TAC5112 product page <https://www.ti.com/product/TAC5112>`_
