.. _bt_mesh_ug_reserved_ids:

Reserved vendor model IDs and opcodes
#####################################

.. contents::
   :local:
   :depth: 2

This page contains information about vendor model IDs and opcodes allocated by Nordic Semiconductor.

As a Bluetooth® Special Interest Group (SIG) member, Nordic Semiconductor has been assigned the Company ID *0x0059*.
You can find this information on the `Bluetooth SIG company identifiers`_ page.

Reserved vendor model IDs
*************************

The table below lists all vendor-assigned model identifiers allocated by Nordic Semiconductor:

   +-----------------------------+-----------------+
   | Model name                  | Vendor Model ID |
   +=============================+=================+
   | Simple OnOff Server         | 0x0000          |
   +-----------------------------+-----------------+
   | Simple OnOff Client         | 0x0001          |
   +-----------------------------+-----------------+
   | Rssi Server                 | 0x0005          |
   +-----------------------------+-----------------+
   | Rssi Client                 | 0x0006          |
   +-----------------------------+-----------------+
   | Rssi Util                   | 0x0007          |
   +-----------------------------+-----------------+
   | Thingy52 Server             | 0x0008          |
   +-----------------------------+-----------------+
   | Thingy52 Client             | 0x0009          |
   +-----------------------------+-----------------+
   | Chat Client                 | 0x000A          |
   +-----------------------------+-----------------+
   | Distance Measurement Server | 0x000B          |
   +-----------------------------+-----------------+
   | Distance Measurement Client | 0x000C          |
   +-----------------------------+-----------------+
   | LE Pairing Initiator        | 0x000D          |
   +-----------------------------+-----------------+
   | LE Pairing Responder        | 0x000E          |
   +-----------------------------+-----------------+

Reserved opcodes
****************

The table below lists the manufacturer-specific opcodes allocated by Nordic Semiconductor:

   +-----------------------------------+--------+
   | Message name                      | Opcode |
   +===================================+========+
   | Simple OnOff Set                  | 0x01   |
   +-----------------------------------+--------+
   | Simple OnOff Get                  | 0x02   |
   +-----------------------------------+--------+
   | Simple OnOff Set Unreliable       | 0x03   |
   +-----------------------------------+--------+
   | Simple OnOff Status               | 0x04   |
   +-----------------------------------+--------+
   | Rssi Ack                          | 0x05   |
   +-----------------------------------+--------+
   | Rssi Send Data                    | 0x06   |
   +-----------------------------------+--------+
   | Rssi Util Request Database Beacon | 0x07   |
   +-----------------------------------+--------+
   | Rssi Util Send Database Beacon    | 0x08   |
   +-----------------------------------+--------+
   | Thingy52 RGB Set                  | 0x09   |
   +-----------------------------------+--------+
   | Chat Message                      | 0x0A   |
   +-----------------------------------+--------+
   | Chat Private Message              | 0x0B   |
   +-----------------------------------+--------+
   | Chat Message Reply                | 0x0C   |
   +-----------------------------------+--------+
   | Chat Presence                     | 0x0D   |
   +-----------------------------------+--------+
   | Chat Presence Get                 | 0x0E   |
   +-----------------------------------+--------+
   | Distance Measurement Server       | 0x0F   |
   +-----------------------------------+--------+
   | Distance Measurement Client       | 0x10   |
   +-----------------------------------+--------+
   | LE Pairing Related Messages       | 0x11   |
   +-----------------------------------+--------+
