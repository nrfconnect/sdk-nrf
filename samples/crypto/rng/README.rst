.. _crypto_rng:

Crypto: RNG
###########

The RNG sample shows how to produce random numbers.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample produces random numbers of 100-byte length.

It performs the following operations:

1. Initialization of the Platform Security Architecture (PSA) API.

#. Generation of five 100-byte random numbers.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/rng`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using an RTT Viewer or a terminal emulator.

.. note::
   By default, the sample is configured to use both RTT and UART for logging.
   If you are using RTT, skip the first step of the testing procedure.
