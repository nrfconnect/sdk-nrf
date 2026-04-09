.. bt_gatt_shell:

Bluetooth: GATT Shell
#####################

.. contents::
   :local:
   :depth: 2

The following examples assume that you have two devices already connected.

To perform service discovery on the client side, use the :code:`gatt discover` command.
This prints all services available on the GATT server.

On the server side, you can register pre-defined test services using the :code:`gatt register` command.
You will see the newly added services on the client side when running the discovery command.

You can now subscribe to the new services on the client side.
Here is an example on how to subscribe to the test service:

.. code-block:: console
   uart:~$ gatt subscribe 26 25
   Subscribed

The server can now notify the client with the :code:`gatt notify` command.

Another option available through the GATT command is initiating the MTU exchange using the :code:`gatt exchange-mtu` command.
To update the shell maximum MTU, you need to update the Kconfig options in the configuration file of the shell.
For more details, see :zephyr:code-sample:`bluetooth_mtu_update`.
