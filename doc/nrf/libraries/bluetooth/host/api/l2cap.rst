.. _bt_l2cap:

Logical Link Control and Adaptation Protocol (L2CAP)
####################################################

.. contents::
   :local:
   :depth: 2

The L2CAP layer enables connection-oriented channels that you can enable using the :kconfig:option:`CONFIG_BT_L2CAP_DYNAMIC_CHANNEL` Kconfig option.
These channels support segmentation and reassembly transparently.
They also support credit-based flow control making it suitable for data streams.

You can also define fixed channels using the :c:macro:`BT_L2CAP_FIXED_CHANNEL_DEFINE` macro.
Fixed channels are initialized upon connection and do not support segmentation.
Here is an example of how to define a fixed channel:

.. code-block:: c

   static struct bt_l2cap_chan fixed_chan[CONFIG_BT_MAX_CONN];

   /* Callbacks are assumed to be defined prior. */
   static struct bt_l2cap_chan_ops ops = {
       .recv = recv_cb,
       .sent = sent_cb,
       .connected = connected_cb,
       .disconnected = disconnected_cb,
   };

   static int l2cap_fixed_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
   {
       uint8_t conn_index = bt_conn_index(conn);

       *chan = &fixed_chan[conn_index];

       **chan = (struct bt_l2cap_chan){
           .ops = &ops,
       };

       return 0;
   }

   BT_L2CAP_FIXED_CHANNEL_DEFINE(fixed_channel) = {
       .cid = 0x0010,
       .accept = l2cap_fixed_accept,
   };

Channel instances are represented by the :c:struct:`bt_l2cap_chan` structure containing the callbacks in the :c:struct:`bt_l2cap_chan_ops` structure.
This structure indicates when the channel has been connected, disconnected, or when the encryption has changed.
In addition, it also contains the ``recv`` callback that is called whenever data has been received.
Data received this way can be marked as processed by returning ``0`` or using the :c:func:`bt_l2cap_chan_recv_complete` function if processing is asynchronous.

.. note::
   The ``recv`` callback is called directly from RX thread.
   It is not recommended to be blocked for long periods of time.

For sending data, the :c:func:`bt_l2cap_chan_send` function can be used to note that it may block if no credits are available, and resuming as soon as more credits are available.

You can register servers using the :c:func:`bt_l2cap_server_register` function passing the :c:struct:`bt_l2cap_server` structure that indicates which ``psm`` it should listen to, the required security level ``sec_level``, and the callback ``accept`` that is called to authorize incoming connection requests and allocate channel instances.

You can initiate client channels using the :c:func:`bt_l2cap_chan_connect` function and disconnected using the :c:func:`bt_l2cap_chan_disconnect` function.
The later can also disconnect channel instances created by servers.

API Reference
*************

| Header file: :file:`include/bluetooth/host/l2cap.h`

.. doxygengroup:: bt_l2cap
