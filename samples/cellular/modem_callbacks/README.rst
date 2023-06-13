.. _modem_callbacks_sample:

Cellular: Modem callbacks
#########################

.. contents::
   :local:
   :depth: 2

The Modem callbacks sample demonstrates how to set up callbacks for Modem library initialization and shutdown calls, using the :ref:`nrf_modem_lib_readme` and how to set up a callback for changes to the modem functional mode using the :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample performs the following operations:

1. Registers callbacks during compile time for modem initialization, functional mode changes, and shutdown using the :c:macro:`NRF_MODEM_LIB_ON_INIT`, :c:macro:`LTE_LC_ON_CFUN` and :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` macros respectively.
#. Initializes the :ref:`nrfxlib:nrf_modem`.
#. Changes functional mode using the :c:func:`lte_lc_func_mode_set` function in the :ref:`lte_lc_readme` library
#. Shuts down the :ref:`nrfxlib:nrf_modem`.

This triggers the callbacks for :c:func:`on_modem_init`, :c:func:`on_cfun` and :c:func:`on_modem_shutdown` functions.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/modem_callbacks`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset your nRF9160 DK.
#. Observe that the sample starts, initializes the modem, changes functional mode and shuts down the modem.

Sample output
=============

The sample shows the following output:

.. code-block:: console

  Modem callbacks sample started
  Initializing modem library
  > Initialized with value 0
  Changing functional mode
  > Functional mode has changed to 1
  Shutting down modem library
  > Functional mode has changed to 0
  > Shutting down
  Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
