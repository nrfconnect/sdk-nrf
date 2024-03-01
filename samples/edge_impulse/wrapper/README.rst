.. _ei_wrapper_sample:

Edge Impulse: Wrapper
#####################

.. contents::
   :local:
   :depth: 2

The Edge Impulse wrapper sample demonstrates the functionality of the :ref:`ei_wrapper`.
It shows how to use the wrapper to run a custom `Edge Impulse`_ machine learning model when :ref:`integrating Edge Impulse with the |NCS| <ug_edge_impulse>`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample:

1. Initializes the Edge Impulse wrapper.
#. Provides input data to the wrapper.
#. Starts predictions using the machine learning model.
#. Displays the prediction results to the user.

By default, the sample uses a pretrained machine learning model and input data representing a sine wave.

Configuration
*************

|config|

Using your own machine learning model
=====================================

To run the sample using a custom machine learning model, you must complete the following setup:

1. Configure the Edge Impulse wrapper by completing the following steps:

   a. Prepare your own machine learning model using `Edge Impulse studio`_.
   #. Set the :kconfig:option:`CONFIG_EDGE_IMPULSE_URI` to URI of your machine learning model.

   See the :ref:`ei_wrapper` page for detailed configuration steps.
#. Define the input data for the machine learning model in :file:`samples/edge_impulse/wrapper/src/include/input_data.h`.
#. Check the example input data in your Edge Impulse studio project:

   a. Go to the :guilabel:`Live classification` tab.
   #. In the **Classifying existing test sample** panel, select one of the test samples.
   #. Click :guilabel:`Load sample` to display the raw data preview.

      .. figure:: ../../../doc/nrf/images/ei_loading_test_sample.png
         :scale: 50 %
         :alt: Loading test sample input data in Edge Impulse studio

         Loading test sample input data in Edge Impulse studio

      The classification results will be displayed, with raw data preview.

      .. figure:: ../../../doc/nrf/images/ei_raw_features.png
         :scale: 50 %
         :alt: Raw data preview in Edge Impulse studio

         Raw data preview in Edge Impulse studio

#. Copy information from the **Raw features** list into an array defined in the :file:`input_data.h` file.

.. note::
    If you provide more input data than a single input window can hold, the prediction will be triggered multiple times.
    The input window will be shifted by one input frame between subsequent predictions.
    The prediction will be retriggered until there is no more input data.

Building and running
********************

.. |sample path| replace:: :file:`samples/edge_impulse/wrapper`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output similar to the following is logged on UART:

   .. parsed-literal::
      :class: highlight

      *** Booting Zephyr OS build v2.6.99-ncs1-14-g15be5e615498  ***
      Machine learning model sampling frequency: 52
      Labels assigned by the model:
      - idle
      - sine
      - triangle

      Prediction started...

      Classification results
      ======================
      Value: 1.00     Label: sine
      Value: 0.00     Label: triangle
      Value: 0.00     Label: idle
      Anomaly: -0.07


The observed classification results depend on machine learning model and input data.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`ei_wrapper`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
