.. _add_new_driver:

Adding new drivers
##################

.. contents::
   :local:
   :depth: 2

Adding drivers to your embedded project allows you to add support for additional hardware components and extend functionalities of your application.

To add a new driver to an application in the |NCS|, complete the following steps:

.. rst-class:: numbered-step

Create a devicetree binding
***************************

In most of the cases you want to make it possible to configure some properties of your driver instance.
For example, a sensor may need to allow configuring used GPIO pins or communication interface.

A driver instance can be made configurable through a devicetree node that is compatible with a specific binding.

|devicetree_bindings|

You can create a devicetree binding YAML file for your driver in the :file:`dts/bindings` directory of your project.
If applicable, you can also use one of the existing DTS bindings available in the |NCS| or Zephyr.
For implementation examples, see :file:`nrf/dts/bindings` and :file:`zephyr/dts/bindings` directories.
See also :ref:`zephyr:devicetree` documentation for more information about devicetree.

.. rst-class:: numbered-step

Create the driver files
***********************

First, create the files that will implement driver and expose driver APIs.
As a reference, you can check the |NCS| :ref:`drivers`, whose code is located at :file:`nrf/drivers`.

You will need the following files:

* :file:`CMakeLists.txt` - This file adds your driver sources to the build as a Zephyr library.
  See :ref:`app_build_system` for more information about the build system.
* :file:`Kconfig` - This file will include all custom Kconfig options for your driver, including the Kconfig option to enable and disable the driver.
  See Zephyr's :ref:`zephyr:kconfig` for more information.
* Driver's source files - These files will include the code for your driver, such as :ref:`zephyr:device_struct` device definition through the :c:macro:`DEVICE_DT_DEFINE` macro, and other elements.
  If possible, your driver should expose a generic API to simplify integrating the driver in user application.
  For example, the driver could expose sensor driver API (:c:struct:`sensor_driver_api`).
  See :file:`nrf/drivers/sensor/paw3212/paw3212.c` for an example.
  See also Zephyr's :ref:`zephyr:device_model_api` for more information, in particular the "Subsystems and API structures" section.

.. rst-class:: numbered-step

Enable the driver in your application
*************************************

To enable the driver in your application's configuration, complete the following steps:

1. Enable the necessary Kconfig options in the application's Kconfig configuration.
   This includes the Kconfig option you defined for enabling and disabling the driver.
   See :ref:`configuring_kconfig` for information about how to enable Kconfig options.
#. Create or modify a devicetree overlay file for your board to add the necessary devicetree node for your custom driver.
   This step is crucial for connecting the driver to the specific hardware on your board.
   For information about how to create or modify devicetree files, see :ref:`configuring_devicetree`.
#. Include the appropriate header file for your custom driver in your application code and use the driver APIs in the application.
   If your driver exposes a generic API (for example, :ref:`sensor driver API <zephyr:sensor>`), you can use generic headers defined for the API.

DevAcademy courses
******************

`Nordic Developer Academy`_ contains introductory courses to the |NCS| and Zephyr.
See the following course lessons to get started with driver development:

* `Lesson 6 – Serial communication (I2C)`_ in `nRF Connect SDK Fundamentals course`_ describes how to communicate with a sensor connected over I2C using I2C APIs.
* `Lesson 5 – Serial Peripheral Interface (SPI)`_ in `nRF Connect SDK Intermediate course`_ describes how to communicate with sensors over SPI in Zephyr.
* `Lesson 7 - Device driver model`_ in `nRF Connect SDK Intermediate course`_ describes how to start with adding your own sensor driver in the Exercise 1.

Related documentation
*********************

The :ref:`nrf_desktop` application describes how to :ref:`add a new motion sensor to the project <porting_guide_adding_sensor>`.

Implementation examples
***********************

Check the driver implementation examples at the following paths:

* |NCS|: :ref:`drivers`, with code located at :file:`nrf/drivers`.
* Zephyr:

  * :file:`zephyr/drivers` for driver implementation examples.
  * :zephyr:code-sample-category:`drivers` for driver usage examples, with code located at :file:`zephyr/samples/drivers`.
