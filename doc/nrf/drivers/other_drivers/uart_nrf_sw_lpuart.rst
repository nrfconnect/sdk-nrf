.. _uart_nrf_sw_lpuart:

Low power UART driver
#####################

.. contents::
   :local:
   :depth: 2

The low power UART driver implements the standard *asynchronous UART API* that can be enabled with the :kconfig:option:`CONFIG_UART_ASYNC_API` configuration option.
Alternatively, you can also enable the *interrupt-driven UART API* using the :kconfig:option:`CONFIG_NRF_SW_LPUART_INT_DRIVEN` configuration option.

The protocol used by this driver implements two control lines, instead of standard hardware flow control lines, to allow for disabling the UART receiver during the idle period.
This results in low power consumption, as you can shut down the high-frequency clock when UART is in idle state.

Control protocol
****************

You can use the protocol for duplex transmission between two devices.

The control protocol uses two additional lines alongside the standard TX and RX lines:

* the *Request* line (REQ),
* the *Ready* line (RDY).

The REQ line of the first device is connected with the RDY line of the second device.
It is configured as an output and set to low.

The RDY line is configured as input without pull-up and the interrupt is configured to detect the transition from low to high state.

The driver implements the control protocol in the following way:

#. The transmitter initiates the transmission by calling :c:func:`uart_tx`.
#. The driver reconfigures the REQ line to input with pull-up and enables the detection of high to low transition.
#. The line is set to high due to the pull-up register, and that triggers an interrupt on the RDY line.
#. On that interrupt, the driver starts the UART receiver.
#. When the receiver is ready, the driver reconfigures the RDY line to output low.
#. Then, the driver reconfigures the RDY line to input with pull-up and enables the interrupt on detection of high to low transition.
#. This sequence results in a short pulse on the line, as the line goes low and high.
#. The initiator detects the pulse and starts the standard UART transfer.
#. When the transfer is completed, the driver reconfigures the REQ line to the initial state: output set to low.
   This results in a line state change.
#. As the line goes low, the receiver detects the change.
   This indicates that the UART receiver can be stopped.

Once the receiver acknowledges the transfer, it must be capable of receiving the whole transfer until the REQ line goes down.

This requirement can be fulfilled in two ways:

* By providing a buffer of the size of the maximum packet length.
* By continuously responding to the RX buffer request event.
  The latency of the event handling must be taken into account in that case.
  For example, a flash page erase on some devices might have a significant impact.

Configuration
*************

The low power UART driver is built on top of the standard UART driver extended with the control lines protocol.
It is configured in the devicetree as a child node of the UART instance that extends standard UART configuration with REQ and RDY lines.
Additionally, the standard UART configuration must have flow control and flow control pins disabled.

See the following configuration example:

.. code-block:: devicetree

	&pinctrl {
		uart1_default_alt: uart1_default_alt {
			group1 {
				psels = <NRF_PSEL(UART_TX, 1, 13)>,
					<NRF_PSEL(UART_RX, 1, 12)>;
			};
		};

		uart1_sleep_alt: uart1_sleep_alt {
			group1 {
				psels = <NRF_PSEL(UART_TX, 1, 13)>,
					<NRF_PSEL(UART_RX, 1, 12)>;
				low-power-enable;
			};
		};
	};

	&uart1 {
		compatible = "nordic,nrf-uarte";
		status = "okay";
		baudrate = <1000000>;
		pinctrl-0 = <&uart1_default_alt>;
		pinctrl-1 = <&uart1_sleep_alt>;
		pinctrl-names = "default", "sleep";
		/delete-property/ hw-flow-control;

		lpuart: nrf-sw-lpuart {
			compatible = "nordic,nrf-sw-lpuart";
			status = "okay";
			req-pin = <46>;
			rdy-pin = <47>;
		};
	};

The low power UART configuration includes:

* :kconfig:option:`CONFIG_NRF_SW_LPUART_MAX_PACKET_SIZE`: Sets the maximum RX packet size.

* :kconfig:option:`CONFIG_NRF_SW_LPUART_INT_DRIVEN`: Enables the interrupt-driven API.
  When enabled, the asynchronous API cannot be used.

* :kconfig:option:`CONFIG_NRF_SW_LPUART_DEFAULT_TX_TIMEOUT`: Sets the timeout value, in milliseconds.
  It is used in :c:func:`uart_poll_out` and :c:func:`uart_fifo_fill` when the interrupt-driven API is enabled.

* :kconfig:option:`CONFIG_NRF_SW_LPUART_INT_DRV_TX_BUF_SIZE`: Set the size of the internal buffer created and used by :c:func:`uart_fifo_fill`.
  For optimal performance, it should be able to fit the longest possible packet.

Usage
*****

You can access and control the low power UART using the asynchronous UART API.

Data is sent using :c:func:`uart_tx`.
The transfer will timeout if the receiver does not acknowledge its readiness.

The receiver is enabled by calling :c:func:`uart_rx_enable`.
After that call, the receiver is set up and set to the idle (low power) state.

Alternatively, you can access the low power UART using the interrupt-driven UART API.

See :ref:`lpuart_sample` sample for the implementation of this driver.
