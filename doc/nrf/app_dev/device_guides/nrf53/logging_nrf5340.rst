.. _logging_cpunet:

Getting logging output with nRF5340 DK
######################################

.. contents::
   :local:
   :depth: 2

When connected to a computer, the nRF5340 DK emulates virtual serial ports.
The number of serial ports depends on the DK version you are using.

|serial_port_number_list|

nRF5340 DK v2.0.0 serial ports
******************************

When connected to a computer, the nRF5340 DK v2.0.0 emulates two virtual serial ports.
In the default configuration, they are set up as follows:

* The first serial port outputs the log from the network core (if available).
* The second serial port outputs the log from the application core.

nRF5340 DK v1.0.0 serial ports
******************************

When connected to a computer, the nRF5340 DK v1.0.0 emulates three virtual COM ports.
In the default configuration, they are set up as follows:

* The first serial port outputs the log from the network core (if available).
* The second (middle) serial port is routed to the **P24** connector of the nRF5340 DK.
* The third (last) serial port outputs the log from the application core.

To use the middle COM port in the nRF5340 DK v1.0.0, complete the following steps:

1. Map RX, TX, CTS and RTS pins to four different pins on the development kit, using, for example, :ref:`devicetree overlays<zephyr:devicetree-intro>`.
   See the following example, using the :dtcompatible:`nordic,nrf-uarte` bindings.

   .. code-block:: devicetree

      &pinctrl {
         uart0_default_alt: uart0_default_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 20)>,
                       <NRF_PSEL(UART_RX, 0, 21)>,
                       <NRF_PSEL(UART_RTS, 0, 18)>,
                       <NRF_PSEL(UART_CTS, 0, 16)>;
            };
         };

         uart0_sleep_alt: uart0_sleep_alt {
            group1 {
               psels = <NRF_PSEL(UART_TX, 0, 20)>,
                       <NRF_PSEL(UART_RX, 0, 21)>,
                       <NRF_PSEL(UART_RTS, 0, 18)>,
                       <NRF_PSEL(UART_CTS, 0, 16)>;
               low-power-enable;
            };
         };
      };

      &uart0 {
         status = "okay";
         compatible = "nordic,nrf-uarte";
         current-speed = <115200>;
         pinctrl-0 = <&uart0_default_alt>;
         pinctrl-1 = <&uart0_sleep_alt>;
         pinctrl-names = "default", "sleep";
      };

#. Wire the previously mapped pins to **TxD**, **RxD**, **CTS**, and **RTS** on the **P24** connector.


.. _nrf5430_tfm_log:

Log output from TF-M on nRF5340 DK
**********************************

By default, the nRF5340 DK v1.0.0 requires that you connect specific wires on the kit to receive secure logs on the host PC.
Specifically, wire the pins **P0.25** and **P0.26** of the **P2** connector to **RxD** and **TxD** of the **P24** connector respectively.

On the nRF5340 DK v2.0.0, only two virtual COM ports are available.
By default, one of the ports is used by the non-secure UART0 peripheral from the application and the other by the UART1 peripheral from the network core.

There are several options to get UART output from the secure TF-M:

* Disable the output for the network core and change the pins used by TF-M.
  The network core usually has a child image.
  To configure logging in an |NCS| image, see :ref:`ug_logging`.
  To change the pins used by TF-M, set the RXD (:kconfig:option:`CONFIG_TFM_UART1_RXD_PIN`) and TXD (:kconfig:option:`CONFIG_TFM_UART1_TXD_PIN`) Kconfig options in the application image to **P1.00** (32) and **P1.01** (33).

* You can wire the secure and non-secure UART peripherals to the same pins.
  Specifically, physically wire together the pins **P0.25** and **P0.26** to **P0.20** and **P0.22**, respectively.

* If the non-secure application, network core, and TF-M outputs are all needed simultaneously, additional UART-to-USB hardware is needed.
  You can use a second DK if available.
  Connect the **P0.25** pin to the TXD pin, and **P0.26** to the RXD pin of the external hardware.
  These pins provide the secure TF-M output, while the two native COM ports of the DK are used for the non-secure application and network core output.
