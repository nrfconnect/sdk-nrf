.. _modem_callbacks_sample:

nRF9160: Modem callbacks
########################

.. contents::
   :local:
   :depth: 2

The Modem callbacks sample demonstrates how to setup callbacks for Modem library initialization and shutdown calls, using the :ref:`nrf_modem_lib_readme` and how to setup a callback on changes to the modem function mode using the :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

Overview
********

The sample registers three callbacks at compile time: :c:func:`on_modem_init`, :c:func:`on_modem_shutdown` and :c:func:`on_cfun` using the :c:macro:`NRF_MODEM_LIB_ON_INIT`, :c:macro:`NRF_MODEM_LIB_ON_SHUTDOWN` and :c:macro:`LTE_LC_ON_CFUN` macros respectively.
Next, the sample initializes and shuts down the :ref:`nrfxlib:nrf_modem`, triggering :c:func:`on_modem_init` and :c:func:`on_modem_shutdown`.
Then, the sample initializes the modem library again, changes function mode using the :c:func:`lte_lc_func_mode_set` function in the :ref:`lte_lc_readme` library, and shuts down the modem library again.
This triggers the :c:func:`on_modem_init`, :c:func:`on_cfun` and :c:func:`on_modem_shutdown` callbacks.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/modem_callbacks`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset your nRF9160 DK.
#. Observe that the sample starts, creates and configures a PDP context, and then activates a PDN connection.

Sample output
=============

The sample shows the following output, which may vary based on the network provider:

.. code-block:: console

  Modem callbacks sample started
  Initializing modem library
  > Initialized with value 0
  Shutting down modem library
  > Shutting down
  Initializing modem library (again)
  > Initialized with value 0
  Changing function mode
  > Function mode has changed to 1
  Shutting down modem library (again)
  > Function mode has changed to 0
  > Shutting down
  Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`
* :ref:`nrf_modem_lib_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
