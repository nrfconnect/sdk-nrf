.. _pdn_client_sample:

nRF9160: PDN Client
###################

The PDN client sample demonstrates how to control PDN connection
with the nRF9160 modem by using PDN library.


Overview
********

The PDN Client sample to connect, activate, deactivate and close
the PDN connection with specific APN.


Requirements
************

* The following development board:

  * |nRF9160DK|

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/pdn_client`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the PDN Client sample to your board, test
the sample by performing the following steps:

1. Connect the USB cable.
2. Open a terminal emulator.
3. power on or reset your nRF9160 DK to see a sample output


Sample output
=============

The sample shows the following output:

.. code-block:: console

   The PDN client sample started
   Starting the modem... OK
   Wait 5sec that modem is up and running...
   Connecting to APN 'my.app.custom.apn'... OK
   Activating the PDN... OK
   PDN state=0 and ID=1
   Deactivate and disconnect the PDN... OK
   Sample completed, exiting...


Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`at_cmd_readme`
  * :ref:`at_notif_readme`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`


References
**********

`AT Commands Reference Guide`_
