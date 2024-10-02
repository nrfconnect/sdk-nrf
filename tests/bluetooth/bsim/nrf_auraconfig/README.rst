.. _nrf_auraconfig_test:

nRF Auraconfig test
###################

.. contents::
   :local:
   :depth: 2

This test code is a shim to allow for Zephyr's :ref:`zephyr:bsim` testing of the :ref:`nrf_auraconfig` sample.
It is not intended for use in production code.
Compared to other tests, these run as integration tests involving the Controller, the Host, and the application layer.

Requirements
************

The test supports the :ref:`nrf5340bsim<nrf5340bsim>` board.

Building and running
********************

These tests are run as part of nRF Connect SDK CI with specific configurations.
They should not be treated as a reference for general use, as they are designed for this CI only and fault injection purposes.

For more information about BabbleSim tests, see the :ref:`documentation in Zephyr <zephyr:bsim>`.
