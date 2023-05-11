.. _crypto_ecjpake:

Crypto: EC J-PAKE
#################

.. contents::
   :local:
   :depth: 2

The EC J-PAKE sample demonstrates how to do password-authenticated key exchange using
the elliptic curve (EC) version of the password-authenticated key exchange by
juggling (J-PAKE) protocol.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample performs the following operations:

1. Initializes the Platform Security Architecture (PSA) API.
#. Goes through the steps for J-PAKE on server and client sides.
#. Verifies that the derived keys are the same.

Building and running
********************

.. |sample path| replace:: :file:`samples/crypto/ecjpake`


Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Compile and program the application.
#. Observe the logs from the application using a terminal emulator.
