.. _ug_edge_impulse:

Edge Impulse integration
########################

.. contents::
   :local:
   :depth: 2

`Edge Impulse`_ is a development platform that can be used to enable `embedded machine learning`_ on |NCS| devices.
You can use this platform to collect data from sensors, train machine learning model, and then deploy it to your Nordic Semiconductor's device.

Integration prerequisites
*************************

Before you start the |NCS| integration with Edge Impulse, make sure that the following prerequisites are completed:

* :ref:`Installation of the nRF Connect SDK <installation>`.
* Setup of the required :term:`Development Kit (DK)`.
* Creation of an `Edge Impulse studio account <Edge Impulse studio signup_>`_ and an Edge Impulse project.

Solution architecture
*********************

Support for `Edge Impulse`_ in the |NCS| is centered around the :ref:`ei_wrapper`, which is used for integrating the Edge Impulse machine learning model into an |NCS| application.
The usage of the wrapper is demonstrated by the :ref:`ei_wrapper_sample` sample, while the :ref:`nrf_machine_learning_app` application demonstrates the complete out of the box reference design.

.. _ug_edge_impulse_adding:

Integration overview
********************

Before integrating the Edge Impulse machine learning model into an |NCS| application, you must prepare and deploy the machine learning model for your embedded device.
This model is prepared using the `Edge Impulse studio`_ external web tool.
It relies on sensor data that can be provided by different sources, for example data forwarder.
Check the :ref:`ei_data_forwarder_sample` sample for a demonstration of how you can send sensor data to Edge Impulse studio using `Edge Impulse's data forwarder`_.

The machine learning model is distributed as a single :file:`zip` archive that includes C++ library sources.
This file is used by the |NCS| build system to build the Edge Impulse library.

Integration steps
*****************

Complete the following steps to generate the archive and add it to the build system:

1. :ref:`ug_edge_impulse_adding_preparing`
#. :ref:`ug_edge_impulse_adding_building`

.. _ug_edge_impulse_adding_preparing:

.. rst-class:: numbered-step

Preparing the machine learning model
====================================

To prepare the machine learning model, use `Edge Impulse studio`_ and follow one of the tutorials described in `Edge Impulse getting started guide`_.
For example, you can try the `Continuous motion recognition tutorial`_.
This tutorial will guide you through the following steps:

1. Collecting data from sensors and uploading the data to Edge Impulse studio.

   .. note::
     You can use one of the development boards supported directly by Edge Impulse or your mobile phone to collect the data.
     You can also modify the :ref:`ei_data_forwarder_sample` sample or :ref:`nrf_machine_learning_app` application and use it to forward data from a sensor that is connected to any board available in the |NCS|.

#. Designing your machine learning model (an *impulse*).
#. Deploying the machine learning model to use it on an embedded device.
   As part of this step, you must select the :guilabel:`C++ library` to generate the required :file:`zip` file that contains the source files for building the Edge Impulse library in |NCS|.

.. _ug_edge_impulse_adding_building:

.. rst-class:: numbered-step

Building the machine learning model in |NCS|
============================================

After preparing the :file:`zip` archive, you can use the |NCS| build system to build the C++ library with the machine learning model.
Complete the following steps to configure the building process:

1. Make sure that the following Kconfig options are enabled:

   * :kconfig:option:`CONFIG_CPP`
   * :kconfig:option:`CONFIG_STD_CPP11`

   .. note::
      The :kconfig:option:`CONFIG_FPU` Kconfig option is implied by default if floating point unit (FPU) is supported by the hardware.
      Using FPU speeds up calculations.

      The Edge Impulse library requires full C++ standard library implementation.
      Because of that, the :kconfig:option:`CONFIG_EDGE_IMPULSE` option selects the :kconfig:option:`CONFIG_REQUIRES_FULL_LIBCPP` option.

#. Make sure that the :kconfig:option:`CONFIG_FP16` Kconfig option is disabled.
   The Edge Impulse library is not compatible with half-precision floating point support introduced in Zephyr.
#. Enable building the downloaded library by setting the :kconfig:option:`CONFIG_EDGE_IMPULSE` Kconfig option.
   Setting this option also enables the :ref:`ei_wrapper`.
#. Enable and specify the Uniform Resource Identifier (URI) in the :kconfig:option:`CONFIG_EDGE_IMPULSE_URI` Kconfig option.
   You can set it to one of the following values:

   * An absolute or relative path to a file in the local file system.
     For this variant, you must download the :file:`zip` file manually and place it under path defined by the Kconfig option.
     The relative path is tracked from the application source directory (``APPLICATION_SOURCE_DIR``).
     CMake variables that are part of the path are expanded.
   * Any downloadable URI supported by CMake's ``file(DOWNLOAD)`` command.
     For this variant, the |NCS| build system will download the :file:`zip` file automatically during build.
     The :file:`zip` file is downloaded into your application's :file:`build` directory.

     If the URI requires providing an additional API key in the HTTP header, you can provide it using the :c:macro:`EI_API_KEY_HEADER` CMake definition.
     The API key is provided using a format in which *key_name* is followed by *key_value*.
     For example, if the URI uses ``x-api_key`` for authentication, the :c:macro:`EI_API_KEY_HEADER` can be defined as follows: ``x-api-key:aaaabbbbccccdddd``.
     The ``aaaabbbbccccdddd`` is a sample *key_value*.
     See :ref:`cmake_options` for more information about defining CMake options for command line builds and the |nRFVSC|.
     See `Downloading model directly from Edge Impulse studio`_ for details about downloading model directly from the Edge Impulse studio.

Downloading model directly from Edge Impulse studio
---------------------------------------------------

As an example of downloadable URI, you can configure the |NCS| build system to download your model directly from the Edge Impulse studio.
You can download a model from either a private or a public project.

Downloading from a private project
++++++++++++++++++++++++++++++++++

Complete the following steps to download the model from a private Edge Impulse project:

1. Set :kconfig:option:`CONFIG_EDGE_IMPULSE_URI` to the URI from Edge Impulse studio:

   .. parsed-literal::
      :class: highlight

      CONFIG_EDGE_IMPULSE_URI="https:\ //studio.edgeimpulse.com/v1/api/*XYZ*/deployment/download?type=zip"

   Set *XYZ* to the project ID of your Edge Impulse project.
   You can check the project ID of your project in the **Project info** panel under :guilabel:`Dashboard`.

   .. figure:: images/ei_project_id.png
      :scale: 50 %
      :alt: Project ID in Edge Impulse studio dashboard

      Project ID in Edge Impulse studio dashboard

#. Define the :c:macro:`EI_API_KEY_HEADER` CMake option (see :ref:`cmake_options`) as ``x-api-key:[ei api key]`` to provide the x-api-key associated with your Edge Impulse project.
   To check what to provide as the *[ei api key]* value, check your API keys under the :guilabel:`Keys` tab in the Edge Impulse project dashboard.

   .. figure:: images/ei_api_key.png
      :scale: 50 %
      :alt: API key under the Keys tab in Edge Impulse studio

      API key under the Keys tab in Edge Impulse studio

Downloading from a public project
+++++++++++++++++++++++++++++++++

Complete the following steps to download the model from a public Edge Impulse project:

1. Check the ID of the public project:

   a. Check the project ID of your project in the **Project info** panel under :guilabel:`Dashboard`.
   #. Provide this project ID in the *XYZ* field in the following URL:

      .. parsed-literal::
         :class: highlight

         https:\ //studio.edgeimpulse.com/v1/api/*XYZ*/versions/public

   #. Paste the URL into your browser.
      The ID of the public project is returned as the value of the ``publicProjectId`` field.
      For example:

      .. parsed-literal::
         :class: highlight

         {"success":true,"versions":[{"version":1,"publicProjectId":66469,"publicProjectUrl":"https://studio.edgeimpulse.com/public/66468/latest"}]}

      In this example, the *XYZ* project ID is ``66468``, while the ``publicProjectId`` equals ``66469``.

#. Set :kconfig:option:`CONFIG_EDGE_IMPULSE_URI` to the following URI from Edge Impulse studio:

   .. parsed-literal::
      :class: highlight

      CONFIG_EDGE_IMPULSE_URI="https:\ //studio.edgeimpulse.com/v1/api/*XYZ*/deployment/download?type=zip&modelType=int8"

   Set the *XYZ* to the public project ID from previous step.
   Using the example above, this would be ``66469``.

   .. note::
      This URI includes the ``modelType=int8`` parameter because from public Edge Impulse projects you can only download quantized models created with Edge Impulse's EON Compiler.

Applications and samples
************************

The following application uses the Edge Impulse integration in the |NCS|:

* :ref:`nrf_machine_learning_app` - demonstrates a complete out of the box reference design.
  The nRF Machine learning application can also be used to collect data from devices (for example, it supports Thingy:53 with an out-of-the-box accelerometer).

The following samples demonstrate the Edge Impulse integration in the |NCS|:

* :ref:`ei_wrapper_sample` sample - demonstrates the usage of the wrapper.
* :ref:`ei_data_forwarder_sample` sample - demonstrates how you can send sensor data to Edge Impulse studio using `Edge Impulse's data forwarder`_.

Library support
***************

The following |NCS| libraries support the Edge Impulse integration:

* :ref:`ei_wrapper`
