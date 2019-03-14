.. _nrf_coap_client_sample:

nRF9160: nRF CoAP Client sample
###############################

The nRF CoAP Client sample demonstrates how to receive data from a public CoAP server with an nRF9160 SiP.

Overview
*********

The sample connects to a public CoAP test server, sends periodic GET request for a test resource that is available on the server, and prints the data that is received.

Requirements
************

* The following development board:

  * nRF9160 DK board (PCA10090)

* :ref:`secure_partition_manager` must be programmed on the board.

  The sample is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
  Therefore, it requires the :ref:`secure_partition_manager` that prepares the required peripherals to be available for the application.

Building and running
********************

This sample can be found under :file:`samples/nrf9160/coap_client` in the |NCS| folder structure.

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board.
It can be programmed independently from the Secure Partition Manager firmware.

See :ref:`gs_programming` for information about how to build and program the application.
Note that you must program two different applications as described in the following section.

Programming the sample
======================

When you connect the nRF9160 DK board to your computer, you must first make sure that the :ref:`secure_partition_manager` sample is programmed:

1. Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller.
#. Build the :ref:`secure_partition_manager` sample and program it.
#. Build the nRF CoAP Client sample (this sample) and program it.
#. Verify that the sample was programmed successfully by connecting to the serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Connect the USB cable and power on your nRF9160 DK.
#. Open a teminal emulator and observe that the kit prints the following information::

       The nRF CoAP client sample started
#. Observe that the kit sends periodic CoAP GET requests after it gets LTE connection.
   The kit sends requests to the configured server (``CONFIG_COAP_SERVER_HOSTNAME``) for a configured resource (``CONFIG_COAP_RESOURCE``).
#. Observe that the kit either prints the reponse data received from the server or indicates a time-out.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * ``drivers/lte_link_control``
  * ``subsys/net/lib/coap``

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
