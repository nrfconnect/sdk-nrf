.. _lib_eth_rtt:

Ethernet over RTT driver
########################

.. contents::
   :local:
   :depth: 2

This driver transfers Ethernet frames using the SEGGER J-Link debugger Real-Time Transfer feature.

It is a network driver visible as a completely independent network interface.
It is meant to be used mainly during development, for debugging, testing, and profiling purposes.

It requires a software application on the PC side, developed specifically to handle the data stream transferred using RTT channels.

Protocol description
********************

The driver transfers the frames in both directions, using the RTT up and down channels.
It does not assign any specific RTT channel number for this purpose.
To find the channels used, the software on the PC side has to search for the channels named ``ETH_RTT``.

Since RTT is a stream-oriented protocol and Ethernet is a frame-oriented protocol, the driver has to apply additional encoding to the Ethernet frames to use RTT:

* Appending the CRC to the end of the frame in big-endian byte order.
  It calculates the CRC from the entire Ethernet frame, using CRC-16/CCITT, ``0xFFFF`` as initial seed, and no final xoring.
* Encoding it as described in the `RFC1055 Serial Line Internet Protocol (SLIP)`_ specification.
* Adding an ``END`` byte both at the beginning and at the end of the encoded frame.

The following table shows how a single frame will look like after the encoding.

+--------+----------------------------------------+-------------------------------------------------+--------+
| 1 byte | 64 or more bytes                       | 2-4 bytes                                       | 1 byte |
+========+========================================+=================================================+========+
| END    | Ethernet frame with SLIP byte stuffing | CRC-16/CCITT big-endian with SLIP byte stuffing | END    |
+--------+----------------------------------------+-------------------------------------------------+--------+

The driver sends one special frame during driver initialization to let the PC know when the device is being reset.
The content of this frame is constant and it is available in the driver source code.

Initialization
**************

You need to define one instance of the device in devicetree like this:

.. code-block:: devicetree

  / {
    eth-rtt {
      compatible = "segger,eth-rtt";
    };
  };

Driver can then be enabled by using the :kconfig:option:`CONFIG_ETH_RTT` Kconfig option.

API documentation
*****************

| Source file: :file:`drivers/net/eth_rtt.c`

After the eth_rtt driver has been initialized, the application will see it as an Ethernet connection.
To use that, the application can use `Zephyr Network APIs`_.
