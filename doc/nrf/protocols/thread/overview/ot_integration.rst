.. _openthread_integration:

OpenThread integration
######################

.. contents::
   :local:
   :depth: 2

This page explains how the OpenThread stack is integrated with Zephyr and the |NCS|.
It describes the location of components in the file structure, threads and synchronization primitives used, and the network traffic flow.

Overview
********

The OpenThread stack is integrated with the Zephyr RTOS through the L2 layer.
See :ref:`zephyr:network_stack_architecture` in the Zephyr documentation for more information.

The OpenThread integration in the L2 layer uses the nRF variant of the :ref:`zephyr:ieee802154_interface` radio driver, which is located underneath the L2 layer.
It can also optionally use Zephyr's IP stack, which is located above it.

Alternatively, you can use the OpenThread API and IPv6 stack directly.
This approach provides less flexibility when porting applications though, because the OpenThread API might not be available for all architectures.

To decide which approach to use for your application, consider the following alternatives:

Using Zephyr's OpenThread integration and IP stack
  This approach is used by default.

  Zephyr's IP stack can use Zephyr's L3 layer, which makes it easy to port the application.
  With this approach, your application can use UDP, TCP, and ICMP built on top of Zephyr's IP stack.

  The disadvantage of using Zephyr's L3 layer is the reception path.
  IP packets must traverse both the OpenThread L3 layer and Zephyr's L3 layer to reach the BSD socket.
  See the :ref:`ot_integration_traffic_flow` section for more information.

Using the OpenThread API and IPv6 stack directly
  For simple applications, using the OpenThread API and IPv6 stack directly can be a good approach.

  The OpenThread API contains built-in support for higher-level protocols like UDPv6 and CoAP.
  Using the OpenThread API directly reduces the overhead, because IP packets do not need to go through Zephyr's stack.
  Handling them directly results in a more energy-efficient application with faster packet handling.

  The disadvantage of this approach is limited portability and functionality.
  Higher-level protocols provided by OpenThread are not as full-featured compared to those implemented in Zephyr.
  For example, the CoAP implementation does not support observers and the block transfer functionality, and the TCP protocol is not implemented in OpenThread.

Threads
*******

The OpenThread network stack uses the following threads:

* ``openthread`` - Responsible for receiving IEEE 802.15.4 frames during reception.

  When the reassembled frames are an application IPv6 packet, the ``openthread`` thread calls the :c:func:`ot_receive_handler` function, which injects the packet back to the L2 layer using the :c:func:`net_recv_data` function, so that the packet can later reach Zephyr's IP stack.
  During the transmission, this thread's job is to handle the previously scheduled OpenThread tasklet that contains the message to be sent.
* ``rx_workq`` - Responsible for receiving L2 frames and directing them either to the OpenThread process or Zephyr's IP stack during reception, depending on whether the frame is an IEEE 802.15.4 frame or an IPv6 packet.
* ``tx_workq`` - Responsible for receiving the UDP packet, scheduling the OpenThread tasklet for transmission, and unlocking the ``openthread`` thread by giving the semaphore.
* ``workqueue`` - Responsible for invoking the radio driver API to schedule a transmission.
* ``802154 RX`` - Responsible for the upper half processing of the radio frame reception (that is, the core stack part).

  This thread works on objects that are :c:struct:`nrf5_802154_rx_frame` structures and are put to :c:type:`nrf5_data.rx_fifo` from the RX IRQ context.
  It is responsible for creating the :c:struct:`net_pkt` structure and passing it to the upper layer using the :c:func:`net_recv_data` function.

See the :ref:`ot_integration_traffic_flow` section for additional information.

Apart from these threads, the OpenThread stack also uses one or more :ref:`application threads <zephyr:threads_v2>`.
These threads execute the application logic.

File system and shim layer
**************************

The OpenThread network stack components are located in the following directories:

* OpenThread stack: :file:`modules/lib/openthread/`
* OpenThread shim layer:

  * Thread entry point function, callbacks, utils, L2 registration: :file:`zephyr/subsys/net/l2/openthread/`
  * OpenThread platform layer location: :file:`zephyr/subsys/net/lib/openthread/platform/`

The responsibilities of the OpenThread shim layer are as follows:

* Translating the data into Zephyr's native :c:struct:`net_pkt` structure.
* Providing the OpenThread thread body and synchronization API.
* Providing :c:func:`openthread_send` and :c:func:`openthread_recv` calls that are registered as the L2 interface API.
* Providing a way to initialize the OpenThread stack.
* Implementing callback functions used by the OpenThread stack.

The nRF IEEE 802.15.4 radio driver is located in the following directories:

* nRF IEEE 802.15.4 radio driver shim layer: :file:`zephyr/drivers/ieee802154/` (:file:`ieee802154_nrf5.c` and :file:`ieee802154_nrf5.h`)
* nRF IEEE 802.15.4 radio driver: :file:`modules/hal/nordic/drivers/nrf_radio_802154`

Radio driver's RX and TX connections
====================================

The RX connection of the radio driver is established through the interrupt handler.
The interrupt handler is registered using Zephyr's mechanism with :c:macro:`NRF_802154_INTERNAL_RADIO_IRQ_HANDLING` defined as ``0``.
The registered IRQ handler uses Zephyr's FIFO to pass the IEEE 802.15.4 frame on.
The ``802154 RX`` thread runs on the highest cooperative priority and waits for this FIFO.
When a new frame appears, the thread continues with the processing.

The TX connection of the radio driver uses the work queue, which calls the radio driver to schedule the transmission.
Then the RTC IRQ is used to send the frame over the air.

.. _ot_integration_traffic_flow:

Traffic flow
************

The traffic flow is not fully symmetrical for the reception (RX) and the transmission (TX) cases.

RX traffic flow
===============

The following figure shows the RX traffic flow when the application is using the :ref:`BSD socket API <bsd_sockets_interface>`.

.. figure:: images/zephyr_netstack_openthread-rx_sequence.svg
   :alt: OpenThread application RX data flow
   :figclass: align-center

   OpenThread application RX data flow

The numbers in the figure correspond to the step numbers in the following data receiving (RX) processing flow:

1. A network data packet is received by the nRF IEEE 802.15.4 radio driver.
#. The device driver places the received frame in the FIFO using the :c:func:`nrf_802154_received_timestamp_raw` function.
   The receive queues also act as a way to separate the data processing pipeline ("Bottom Half") from the core stack part, as the device driver is running in an interrupt context and it must do its processing as fast as possible.
#. The ``802154 RX`` radio driver thread does the core stack processing of the received IEEE 802.15.4 radio frame.
   As a result, it uses the :c:func:`net_recv_data` function to queue the frame to be processed.
#. The work queue thread ``rx_workq`` calls the registered handler for every queued frame.
   In this case, the registered handler function :c:func:`openthread_recv` checks if the frame is of the IEEE 802.15.4 type.
   If this is the case, it inserts the frame into the :c:struct:`rx_pkt_fifo` FIFO structure and returns ``NET_OK``.
#. The ``openthread`` thread gets a frame from the FIFO and processes it.
   It also handles the IP header compression and reassembly of fragmented traffic.
#. As soon as the ``openthread`` thread detects a valid IPv6 packet, it calls the registered callback function :c:func:`ot_receive_handler` since the packet needs to be handled by the higher layer.
   This callback creates a buffer for a :c:struct:`net_pkt` structure that is going to be passed to Zephyr's IP stack.
   It also calls the :c:func:`net_recv_data` function to queue the :c:struct:`net_pkt` structure to be processed by the Zephyr stack.
#. This time the :c:func:`openthread_recv` function called by the work queue returns ``NET_CONTINUE`` since a valid IPv6 packet is present and needs to be processed by Zephyr's higher layer.
#. The :c:func:`net_ipv6_input` function passes the packet to the next higher layer.
#. The packet is passed to the L3 processing.
   If the packet is IP-based, the L3 layer processes the IPv6 packet.
#. A socket handler finds an active socket to which the network packet belongs and puts it in a queue for that socket, in order to separate the networking code from the application.
#. The application receives the data and can process it as needed.

   .. tip::
      The application should use the :ref:`BSD socket API <bsd_sockets_interface>` to create a socket that will receive the data.

TX traffic flow
===============

The following figure shows the TX traffic flow when the application is using the :ref:`BSD socket API <bsd_sockets_interface>`.

.. figure:: images/zephyr_netstack_openthread-tx_sequence.svg
   :alt: OpenThread Application TX data flow
   :figclass: align-center

   OpenThread Application TX data flow

The numbers in the figure correspond to the step numbers in the following data transmitting (TX) processing flow:

1. The application uses the :ref:`BSD socket API <bsd_sockets_interface>` to send the data.

   The OpenThread API can also be directly interacted with, for example to use its CoAP implementation.
#. The application data is prepared to be sent to the kernel space and copied to internal :c:struct:`net_buf` structures.
#. A protocol header depending on the socket type is added in front of the data.
   For example, if the socket is a UDP socket, a UDP header is constructed and placed in front of the data.
#. The :c:func:`process_tx_packet` function queues a UDP :c:struct:`net_pkt` structure to be processed.
   In the call chain, the :c:func:`openthread_send` function is called.
#. The called :c:func:`openthread_send` function converts the :c:struct:`net_pkt` structure to the :c:struct:`otMessage` format and invokes the :c:func:`otIp6Send` function.
   In this step, the message is processed by the OpenThread stack.
#. The tasklet to schedule the transmission is posted and the semaphore that unlocks the ``openthread`` thread is given.
   Mac and Submac operations take place.
#. The ``openthread`` thread schedules the IEEE 802.15.4 frame to be transmitted.
#. The nRF IEEE 802.15.4 radio driver sends the packet.
