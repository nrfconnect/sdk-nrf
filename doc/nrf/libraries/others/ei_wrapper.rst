.. _ei_wrapper:

Edge Impulse wrapper
####################

.. contents::
   :local:
   :depth: 2

The `Edge Impulse`_ wrapper provides an implementation for running an `embedded machine learning`_ model on a device in |NCS|.
For more information about support for Edge Impulse in |NCS|, see :ref:`ug_edge_impulse`.

Overview
********

The wrapper:

* Buffers input data for the machine learning model.
* Runs the machine learning model in a separate thread.
* Provides results through a dedicated callback.

Before using the wrapper, you need to :ref:`add the machine learning model <ug_edge_impulse_adding>` to your |NCS| application.

Configuration
*************

Enable the Edge Impulse wrapper by :kconfig:option:`CONFIG_EI_WRAPPER` option.
The option is enabled by default if :kconfig:option:`CONFIG_EDGE_IMPULSE` is set.

The Edge Impulse |NCS| library can be configured with the following Kconfig options:

* :kconfig:option:`CONFIG_EI_WRAPPER_DATA_BUF_SIZE`
* :kconfig:option:`CONFIG_EI_WRAPPER_THREAD_STACK_SIZE`
* :kconfig:option:`CONFIG_EI_WRAPPER_THREAD_PRIORITY`
* :kconfig:option:`CONFIG_EI_WRAPPER_PROFILING`

For more detailed description of these options, refer to the Kconfig help.

Using Edge Impulse wrapper
**************************

After setting Edge Impulse wrapper `Configuration`_ options, you can use it in your application.
Use the following Edge Impulse wrapper API:

* Use the :c:func:`ei_wrapper_init` function to initialize the wrapper.
* Provide the input data using the :c:func:`ei_wrapper_add_data` function.
  The provided data is appended to an internal circular buffer that is located in RAM.

  .. note::
     Make sure that:

     * The number of provided input values is divisible by the input frame size for the machine learning model used.
       Otherwise, an error code is returned.
     * The value for the :kconfig:option:`CONFIG_EI_WRAPPER_DATA_BUF_SIZE` Kconfig option is big enough to temporarily store the data provided by your application.

* Call the :c:func:`ei_wrapper_start_prediction` function to shift the prediction window and start the prediction for the buffered data.
  If the whole input window is filled with data right after the shift operation, the prediction is started instantly.
  Otherwise, the prediction is delayed until the missing data is provided.

  .. note::
     The input data that goes out of the input window is dropped from the input buffer after the shift operation.
     This part of the input buffer can be reused to store new data.

The Edge Impulse wrapper runs the machine learning model in a dedicated thread.
Results are provided through a callback registered during the initialization of the wrapper.
You can call the following functions to access results:

* :c:func:`ei_wrapper_get_next_classification_result`
* :c:func:`ei_wrapper_get_anomaly`
* :c:func:`ei_wrapper_get_timing`

Refer to the API documentation for more detailed information about the API provided by the wrapper.

API documentation
*****************

| Header file: :file:`include/ei_wrapper.h`
| Source files: :file:`lib/edge_impulse/`

.. doxygengroup:: ei_wrapper
   :project: nrf
   :members:
