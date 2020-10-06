.. _uart_nrf_sw_lpuart:

Low power UART driver
#####################

The low power UART driver implements the standard asynchronous UART API that can be enabled with the :option:`CONFIG_UART_ASYNC_API` configuration option.

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
It is configured in the device tree as a child node of the UART instance that extends standard UART configuration with REQ and RDY lines.
Additionally, the standard UART configuration must have flow control and flow control pins disabled.

See the following configuration example:

.. code-block:: DTS

	&uart1 {
		compatible = "nordic,nrf-uarte";
		status = "okay";
		rx-pin = <44>;
		tx-pin = <45>;
		baudrate = <1000000>;
		/delete-property/ rts-pin;
		/delete-property/ cts-pin;
		/delete-property/ hw-flow-control;

		lpuart: nrf-sw-lpuart {
			compatible = "nordic,nrf-sw-lpuart";
			status = "okay";
			label = "LPUART";
			req-pin = <46>;
			rdy-pin = <47>;
		};
	};

The low power UART configuration includes:

* :option:`CONFIG_NRF_SW_LPUART_MAX_PACKET_SIZE`: Maximum RX packet size

Usage
*****

You can access and control the low power UART using the Asynchronous UART API.

Data is sent using :c:func:`uart_tx`.
The transfer will timeout if the receiver does not acknowledge its readiness.

The receiver is enabled by calling :c:func:`uart_rx_enable`.
After that call, the receiver is set up and set to idle (low power) state.

See :ref:`lpuart_sample` sample for an implementation of this driver.
