.. _ei_ncs_sample:

Edge Impulse wrapper
####################

.. contents::
   :local:
   :depth: 2

The Edge Impulse wrapper sample demonstrates the functionality of the :ref:`ei_ncs`.
It shows how to use the wrapper to run a custom `Edge Impulse`_ machine learning model.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns, nrf52840dk_nrf52840, nrf52dk_nrf52832

Overview
********

The sample performs the following operations:

* Initializes the Edge Impulse wrapper.
* Provides input data to the wrapper.
* Starts prediction using the machine learning model.
* Receives results in a callback and displays them.

Configuration
*************

|config|

Setup
=====

Before running the sample, you must complete the following setup:

1. Configure the Edge Impulse wrapper by completing the following steps:

   a. Prepare your own machine learning model using `Edge Impulse studio`_.
   #. Set the :option:`CONFIG_EDGE_IMPULSE_URI` to URI of your machine learning model.

   See the :ref:`ei_ncs` page for detailed configuration steps.
#. Define the input data for the machine learning model in :file:`samples/ei_ncs/src/include/input_data.h`.
   You can find the example input data as :guilabel:`Raw features` in the :guilabel:`Live classification` tab of your Edge Impulse project.
   Load one of the existing test samples and copy the raw features into array defined into the :file:`input_data.h` file.

   .. tip::
      If you provide more input data than a single input window can hold, the prediction will be triggered multiple times.
      The input window will be shifted by one input frame between subsequent predictions.
      Prediction will be retriggered until there is no more input data.

Building and running
********************
.. |sample path| replace:: :file:`samples/ei_ncs`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output similar to the following is logged on UART:

   .. parsed-literal::
      :class: highlight

      *** Booting Zephyr OS build v2.4.0-ncs1-3484-g21046b8cdb4e  ***
      Prediction started...

      Classification results
      ======================
      Label: updown
      Value: 1.00
      Anomaly: 0.47

The observed classification results depend on machine learning model and input data.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`ei_ncs`
