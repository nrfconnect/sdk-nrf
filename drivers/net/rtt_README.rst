.. _eth_rtt_readme:


#######################

Ethernet driver that transfers frames using SEGGER J-Link RTT.
This driver is meant to be used for debugging and testing
purposes. Additional software on PC side is required
to handle frames transferred via RTT channels.

Protocol description
********************

Frames are transferred in both directions using RTT up and down channel.
Specific RTT channel number is not assigned for this purpose,
so software on PC side have to search for channels named *ETH_RTT*.

RTT is a stream oriented protocol and Ethernet is a frames oriented
protocol, so additional encoding have to be applied to ethernet frames
before they go to RTT. First CRC is appended to the end of frame (big
endian byte order). It is calculated from entire ethernet frame using
CRC-16/CCITT with initial seed 0xFFFF and no final xoring. Next it is
encoded using encoding described in SLIP protocol (RFC 1055) adding
*END* byte on the beginning and he end of encoded frame. Following
table shows how single frame will look like after encoding.

+--------+---------------------------+---------------------------+--------+
| 1 byte | 64 or more bytes          | 2-4 bytes                 | 1 byte |
+========+===========================+===========================+========+
| END    | | Ethernet frame          | | CRC-16/CCITT big endian | END    |
|        | | with SLIP byte stuffing | | with SLIP byte stuffing |        |
+--------+---------------------------+---------------------------+--------+

PC side may want to know when device was reset. Driver sends one special
frame during driver initialization. Content of this frame is constant
and it is available in driver's source code.
