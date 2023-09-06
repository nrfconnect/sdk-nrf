.. _central_esl_subcommands:

Central ESL subcommands
#######################

.. contents::
   :local:
   :depth: 2

.. _esl_c_subcmds:


ESL Client subcommands
**********************

``esl_c`` is the ESL Access Point client shell command and it supports the following subcommands.

.. list-table:: ESL Client subcommands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - state
     - N/A
     - Show current AP state machine.
   * - unassociated
     - connection index (0~CONFIG_BT_MAX_CONN)
     - Unassociate connected tag


.. _obj_c_subcmds:


OTS Client subcommands
**********************

``esl_c obj_c`` is the Object Transfer client shell command and it supports the following subcommands.

.. list-table:: Object Transfer client commands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - select
     - <idx> - (Min: 0, Max:Max image index of ESL tag)
     - Select Object on ESL tag with image index.
   * - read_meta
     - <idx> - (Min: 0, Max:Max image index of ESL tag)
     - Read metadata of Object.
   * - write
     - | <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
       | <tag_img_idx> (0 ~ Max image index of ESL tag)
       | <AP Image idx> (0 ~ Max image stored in AP)
     - Write image with index on AP. The images stored in AP with filename pattern **ots_image_XX**. **XX** refers to hexdecimal index.
   * - write_filename
     - | <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
       | <tag_img_idx> (0 ~ Max image index of ESL tag)
       | <AP Image filename>
     - Write Image with filename on AP. The filename could be anything transferred from SMP.


.. _acl_subcmds:


ACL subcommands
***************

``esl_c acl`` shell command is related to the ESL Access Point ACL and it supports the following subcommands.

.. list-table:: ACL commands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - disconnect
     - <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
     - Disconnect TAG
   * - security
     - | <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
       | <security level> (0 ~ 4)
     - Change ACL security level for development.
   * - write_wo_rsp
     - <on/off>  (0 = GATT use write procedure, 1 = GATT uses write without response)
     - Set write characteristics without response.
   * - read_chrc
     - <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
     - Read information characteristics from connected ESL Tag.
   * - scan
     - | <on/off>  (1/0)
       | <oneshot> (1 = scan for one device, 0 = scan continuously)
     - Scan ESL service.
   * - list_scanned
     - N/A
     - Dump scanned BLE address of ESL device.
   * - list_conn
     - N/A
     - Dump connected ESL device.
   * - connect
     - <tag idx> (0 ~ Max scanned tag)
     - Connect ESL tag by scanned list.
   * - connect_addr
     - | <address type> (0 = public, 1 = random, 2 = public_id, 3 = private_id)
       | <ble address> (xx:xx:xx:xx:xx:xx)
     - Connect ESL tag by BLE address.(No need to be scanned).
   * - bt_key_import
     - | <ble address> (xx:xx:xx:xx:xx:xx)
       | <hex_string>
     - Import serialized BT bonding key runtime.
   * - bt_key_export
     - N/A
     - Export serialized BT bonding key.
   * - connect_esl
     - <esl_addr>
     - Connect ESL service tag with esl addr.
   * - configure
     - | <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
       | <esl_addr>
     - Configure connected tag manually.
   * - discovery
     - <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
     - Discovery connected tag manually.
   * - write_esl_addr
     - | <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
       | <esl_addr>
       | <local only> (0 = only change esl addr on AP, 1 = change esl addr on AP and connecte ESL tag)
     - Write ESL address characteristic.
   * - subscribe
     - <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
     - Subscribe ECP notifiy manually.
   * - past
     - <connection index>  (0 ~ CONFIG_BT_MAX_CONN)
     - Commence PAST(Periodic Advertising Sync Transfer) to connected ESL tag .


.. _pawr_subcmds:


PAWR subcommands
****************

``esl_c pawr`` shell command is related to the ESL Access Point periodic advertising with respones (PAwR) and it supports the following subcommands.

.. list-table:: PAWR commands
   :header-rows: 1

   * - Subcommand
     - Argument
     - Description
   * - sync_buf_status
     - <esl group index>(optional) (0 ~ CONFIG_ESL_CLIENT_MAX_GROUP)
     - Dump ESL sync packet buffer status.
   * - push_sync_buf
     - | <esl group index> (0 ~ CONFIG_ESL_CLIENT_MAX_GROUP)
       | <payload_hex_string>
     - Push ESL sync packet to buffer. AP will send packet in next subevent.
   * - dump_sync_buf
     - <esl group index> (0 ~ CONFIG_ESL_CLIENT_MAX_GROUP)
     - Dump ESL sync buffer content.
   * - dump_resp_buf
     - <esl group index> (0 ~ CONFIG_ESL_CLIENT_MAX_GROUP)
     - Dump ESL response buffer content.
   * - start_pawr
     - N/A
     - Start pawr manually for development.
   * - stop_pawr
     - N/A
     - Stop pawr manually for development.
   * - update_pawr
     - | <sync_packet type> (See predefined table)
       | <esl group index> (0 ~ CONFIG_ESL_CLIENT_MAX_GROUP)
     - Send predefined ESL packet.


.. _predefined_esl_packet:

Predefined ESL sync packet
**************************

This sample contains a few predefined ESL sync packets.
The following table shows their content.

.. list-table:: ESL Sync TLV commands
   :header-rows: 1

   * - Sync packet type
     - Description
   * - 0x0
     - Broadcast Ping.
   * - 0x1
     - LED 0 flashing broadcast.
   * - 0x2
     - LED 1 flashing broadcast.
   * - 0x3
     - LED 0 on broadcast.
   * - 0x4
     - LED 0 off broadcast.
   * - 0x5
     - LED 1 on broadcast.
   * - 0x6
     - LED 0 off broadcast.
   * - 0x7
     - Display 0 image 0 broadcast.
   * - 0x8
     - Display 0 image 1 broadcast.
   * - 0x9
     - Read sensor 0 broadcast.
   * - 0xA
     - Ping default ESL_ID ~ ESL_ID+4.
   * - 0xB
     - LED 0 flashing default ESL_ID ~ ESL_ID+2.
   * - 0xC
     - LED 0 flashing default ESL_ID+3, ESL_ID+4. Fill first 2 slots with broadcast ping to make response slot begin with 2.
   * - 0xD
     - Led 1 flashing default ESL_ID ~ ESL_ID+2.
   * - 0xE
     - LED 1 flashing default ESL_ID+3, ESL_ID+4. Fill first 2 slots with broadcast ping to make response slot begin with 2.
   * - 0xF
     - Read sensor 0 default ESL_ID ~ ESL_ID+4.
   * - 0x10
     - ESLP/ESL/SYNC/BI-03-I [Response TLV Too Long]. Send 17 TLVs to default ESL_ID
   * - 0x11
     - Send 22 Pings to default ESL_ID+1 and 1 for ESL_ID at TLVs slot 11.
   * - 0x12
     - Tag default ESL_ID ~ ESL_ID+10 Display Img 0.
   * - 0x13
     - Tag default ESL_ID ~ ESL_ID+10 Display img 1.
   * - 0x14
     - Tag default ESL_ID ~ ESL_ID+10 Display img 2
