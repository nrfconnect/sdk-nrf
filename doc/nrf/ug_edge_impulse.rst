.. _ug_edge_impulse:

Using Edge Impulse with |NCS|
#############################

.. contents::
   :local:
   :depth: 2

`Edge Impulse`_ is a development platform that can be used to enable `embedded machine learning`_ on |NCS| devices.
You can use this platform to collect data from sensors, train machine learning model, and then deploy it to your Nordic Semiconductor's device.

Overview
********

Support for `Edge Impulse`_ in |NCS| is centered around the :ref:`ei_wrapper`, which is used for integrating the Edge Impulse machine learning model into an |NCS| application.
The usage of the wrapper is demonstrated by the :ref:`ei_wrapper_sample` sample.

Before integrating the Edge Impulse machine learning model to an |NCS| application, you must prepare and deploy the machine learning model for your embedded device.
This model is prepared using the `Edge Impulse studio`_ external web tool.
It relies on sensor data that can be provided by different sources, for example data forwarder.
Check the :ref:`ei_data_forwarder_sample` sample for a demonstration of how you can send sensor data to Edge Impulse studio using `Edge Impulse's data forwarder`_.

.. _ug_edge_impulse_adding:

Adding Edge Impulse model to |NCS|
**********************************

The machine learning model is distributed as a single :file:`zip` archive that includes C++ library sources.
This file is used by the |NCS| build system to build the Edge Impulse library.
See the following section for steps required to generate this archive and add it to the build system.

.. _ug_edge_impulse_adding_preparing:

Preparing the machine learning model
====================================

To prepare the machine learning model, use `Edge Impulse studio`_ and follow one of the tutorials described in `Edge Impulse getting started guide`_.
For example, you can try the `Continuous motion recognition tutorial`_.
This tutorial will guide you through the following steps:

1. Collecting data from sensors and uploading the data to Edge Impulse studio.

   .. note::
     You can use one of the development boards supported directly by Edge Impulse or your mobile phone to collect the data.
     You can also modify the :ref:`ei_data_forwarder_sample` sample and use it to forward data from a sensor that is connected to any board available in |NCS|.

#. Designing your machine learning model (an *impulse*).
#. Deploying the machine learning model to use it on an embedded device.
   As part of this step, you must select the :guilabel:`C++ library` to generate the required :file:`zip` file that contains the source files for building the Edge Impulse library in |NCS|.

Building the machine learning model in |NCS|
============================================

After preparing the :file:`zip` archive, you can use the |NCS| build system to build the C++ library with the machine learning model.
Complete the following steps to configure the building process:

1. Make sure that the following Kconfig options are enabled:

   * :option:`CONFIG_CPLUSPLUS`
   * :option:`CONFIG_STD_CPP11`
   * :option:`CONFIG_LIB_CPLUSPLUS`
   * :option:`CONFIG_NEWLIB_LIBC`
   * :option:`CONFIG_NEWLIB_LIBC_FLOAT_PRINTF`
   * :option:`CONFIG_FPU`

#. Enable building the downloaded library by setting the :option:`CONFIG_EDGE_IMPULSE` Kconfig option.
   Setting this option also enables the :ref:`ei_wrapper`.
#. Enable and specify the Uniform Resource Identifier (URI) in the :option:`CONFIG_EDGE_IMPULSE_URI` Kconfig option.
   You can set it to one of the following values:

   * An absolute path to a file in the local file system.
     For this variant, you must download the :file:`zip` file manually and place it under path defined by the Kconfig option.
   * Any downloadable URI supported by CMake's ``file(DOWNLOAD)`` command.
     For this variant, the NCS build system will download the :file:`zip` file automatically during build.
     The :file:`zip` file is downloaded into your application's :file:`build` directory.

     If the URI requires providing an additional API key, you can provide it using the following CMake definition: :c:macro:`EI_API_KEY_HEADER`.
     The API key is provided using a format in which *key_name* is followed by *key_value*.
     For example, ``api-key:aaaabbbbccccdddd``, where ``aaaabbbbccccdddd`` is a sample *key_value*.
     See :ref:`cmake_options` for more information about defining CMake options for command line builds and SEGGER Embedded studio.

Downloading model directly from Edge Impulse studio
---------------------------------------------------

As an example of downloadable URI, you can configure the |NCS| build system to download your model directly from the Edge Impulse studio.
Complete the following steps to do this:

1. Set :option:`CONFIG_EDGE_IMPULSE_URI` to the URI from Edge Impulse studio:

   .. parsed-literal::
      :class: highlight

      CONFIG_EDGE_IMPULSE_URI="https:\ //studio.edgeimpulse.com/v1/api/*XXXXX*/deployment/download?type=zip"

   The *XXXXX* must be set to the project ID of your Edge Impulse project.
   You can check the project ID of your project in the :guilabel:`Project info` panel under :guilabel:`Dashboard`.

   .. figure:: images/ei_project_id.png
      :scale: 50 %
      :alt: Project ID in Edge Impulse studio dashboard

      Project ID in Edge Impulse studio dashboard

#. Define the :c:macro:`EI_API_KEY_HEADER` CMake option as ``x-api-key:aaaabbbbccccdddd`` to provide the x-api-key associated with yout Edge Impulse project.
   You can access your API keys under the :guilabel:`Keys` tab in the Edge Impulse project dashboard.

   .. figure:: images/ei_api_key.png
      :scale: 50 %
      :alt: API key under the Keys tab in Edge Impulse studio

      API key under the Keys tab in Edge Impulse studio
