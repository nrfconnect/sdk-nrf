.. _ble_throughput:

Bluetooth: Throughput
########################

Overview
********

This sample demonstrates BLE throughput by transfering data between two boards,
or a board and a compatible mobile application. In the first scenario, the sample has
to be programmed on two separate boards that will automatically connect to each other
upon boot and print a message on the console when the test is ready. The user can then
run the test by pressing a key in the console. The progress of the test is shown on screen.
At the end, both peers will print the throughput metrics.
To repeat the test with different parameters, run menuconfig to reconfigure the BLE
parameters as necessary, then flash the new firmware image on one of the two boards.


Requirements
************

* Any Nordic board
* Connection to a computer with a serial terminal

Building and Running
********************

This sample can be found under :file:`samples/bluetooth/throughput` in the
Nordic Connect SDK tree.

See :ref:`bluetooth setup section <bluetooth_setup>` for details.
