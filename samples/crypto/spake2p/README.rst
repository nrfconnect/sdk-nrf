.. _crypto_spake2p:

Crypto: Spake2+
###############

.. contents::
   :local:
   :depth: 2

The Spake2+ sample demonstrates how to do a password-authenticated key exchange using the Spake2+ protocol.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample performs the following operations:

1. Initializes the Platform Security Architecture (PSA) API.
#. Goes through the steps for Spake2+ on server and client sides.
#. Verifies that the derived keys are the same.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/spake2p`


Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
