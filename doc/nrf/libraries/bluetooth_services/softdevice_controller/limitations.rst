.. _softdevice_controller_limitations:

Limitations
###########

When working with the SoftDevice Controller, you should be aware of the following limitations.
In addition, see :ref:`nrf:known_issues` for temporary issues that will be fixed in future releases.

YOPAN-87: The RSSI value reported by the SoftDevice Controller requires additional temperature compensation
  The RSSI value reported by the SoftDevice Controller is the raw value from the radio peripheral.
  Some SoCs require compensation of the RSSI value based on the chip temperature.
  See the Errata document for the respective SoC for detailed information.

  **Workaround:**
  For the nRF53 Series, you can retrieve the chip temperature by reading the value of the temperature peripheral on the network core.
  To do this with the SoftDevice Controller, use the Zephyr HCI VS Read Chip Temperature command (``BT_HCI_OP_VS_READ_CHIP_TEMP``).

  For the nRF52 Series, you can use the Zephyr sensor API instead of the HCI command to retrieve the chip temperature.

  You can then use the retrieved temperature value to compensate the raw RSSI value, following the workaround in the Errata document.

DRGN-8628: Setting Filter_Duplicates in LE Set Scan Enable and LE Set Extended Scan Enable commands does not take effect
  The host receives all advertisements from advertisers in advertising report events even if Filter_Duplicates is set in LE Set Scan Enable and LE Set Extended Scan Enable.
  Filter_Duplicates cannot guarantee that all duplicate advertising reports are filtered out, because the controller has a limited "cache" of known advertising reports.

  **Workaround:** Filter out the duplicate advertising reports in the application.

DRGN-8476: Long packets not supported in connections on Coded PHY
  In connections, the Link Layer payload size is limited to 27 bytes on LE Coded PHY.

DRGN-9083: AAR populated with zero IRK
  If the application has set an all zeroes IRK for a device in the resolving list, then a resolvable address that can be resolved with the all zeroes IRK will be reported to the application as that device in the advertisement report or the connected event.

DRGN-11297: Maximum connection interval of 10 ms before entering LLPM-mode
  The maximum connection interval that can be active when switching to a connection interval of 1 ms is 10 ms.

  **Workaround:** An application that needs to use a higher interval than 10 ms must perform two connection updates to use 1 ms connection interval:

  * A first update to 10 ms connection interval.
  * A second update to 1 ms connection interval.

DRGN-10305: Scan window is limited to 16 seconds
  If the scanner is configured with a scan window larger than 16 seconds, the scanner will truncate the scan window to 16 seconds.

DRGN-8569: SEVONPEND flag must not be modified
  Applications must not modify the SEVONPEND flag in the SCR register when running in priority levels higher than 6 (priority level numerical values lower than 6), as this can lead to undefined behavior.

DRGN-6362: Synthesized low frequency clock source not tested
  Synthesized low frequency clock source is not tested or intended for use with the SoftDevice Controller.

DRGN-12259: HCI Receiver and Transmitter Test commands not available
  The HCI Receiver and Transmitter Test commands are not available.

  **Workaround:** To perform a radio test, use a DTM application:

  * For nRF52, use the DTM application in the nRF5 SDK.
  * For nRF53, use :ref:`direct_test_mode`.

DRGN-15989: In some cases, the SDC can connect to the peer it is already connected to
  To prevent this, the application can remove the already connected peer from the Filter Accept List, or from the Resolving List, or not use that peer's address in Create Connection command.

DRGN-17724: The AoA transmitter is not supported in LLPM-mode
  The AoA transmitter can show unspecified behavior when being used in LLPM-mode.
