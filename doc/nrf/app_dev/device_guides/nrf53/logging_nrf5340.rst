.. _logging_cpunet:

Getting logging output with nRF5340 DK
######################################

.. contents::
   :local:
   :depth: 2

When connected to a computer, the nRF5340 DK emulates virtual COM ports.
The number of COM ports depends on the DK version you are using.

nRF5340 DK v2.0.0 COM ports
***************************

When connected to a computer, the nRF5340 DK v2.0.0 emulates two virtual COM ports.
In the default configuration, they are set up as follows:

* The first COM port outputs the log from the network core (if available).
* The second COM port outputs the log from the application core.

nRF5340 DK v1.0.0 COM ports
***************************

When connected to a computer, the nRF5340 DK v1.0.0 emulates three virtual COM ports.
In the default configuration, they are set up as follows:

* The first COM port outputs the log from the network core (if available).
* The second (middle) COM port is routed to the **P24** connector of the nRF5340 DK.
* The third (last) COM port outputs the log from the application core.

To use the middle COM port in the nRF5340 DK v1.0.0, complete the following steps:

1. Map RX, TX, CTS and RTS pins to four different pins on the development kit, using, for example, :ref:`devicetree overlays<zephyr:devicetree-intro>`.
   See the following example, using the :ref:`zephyr:dtbinding_nordic_nrf_uarte` bindings.

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
