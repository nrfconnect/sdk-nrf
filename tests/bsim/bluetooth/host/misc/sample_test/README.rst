.. _bluetooth_bsim_test_sample:

Bluetooth: Example BabbleSim test
#################################

.. contents::
    :local:
    :depth: 2

This serves as template for implementing a new BabbleSim Bluetooth test.

Overview
********

BabbleSim_ is :ref:`integrated in zephyr <bsim>` and used for testing the Bluetooth stack.
The tests are implemented in ``tests/bsim/bluetooth``.
You can run them only on Linux.

This sample test uses the ``testlib`` (:zephyr_file:`tests/bluetooth/common/testlib/CMakeLists.txt`) test library.
You do not need to duplicate code that is not relevant to the test in question, such as setting up a connection, getting the GATT handle of a characteristic, and more.

Do not use the ``bs_`` prefix in files or identifiers.
It is meant to namespace the babblesim simulator code.

Reading guide
*************

It is recommended to read the documentation and code in the following order:

1. The :ref:`Bsim test documentation <bsim>`.
#. ``test_scripts/run.sh``
#. ``CMakeLists.txt``
#. ``src/dut.c`` and ``src/peer.c``
#. ``src/main.c``

.. _BabbleSim:
   https://BabbleSim.github.io
