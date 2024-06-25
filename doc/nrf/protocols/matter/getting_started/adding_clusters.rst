.. _ug_matter_creating_accessory:
.. _ug_matter_gs_adding_cluster:

Adding clusters to Matter application
#####################################

.. contents::
   :local:
   :depth: 2

As part of this guide, you will modify the :ref:`Matter template <matter_template_sample>` sample by adding new application clusters in order to create a Matter sensor device that measures temperature and can be turned on and off.
The sensor will periodically generate the simulated temperature sensor value and update the corresponding cluster attributes.
This application will form a Matter device within a Matter network.

.. note::
   The sensor sample used in this instruction is used here as an example, and does not follow the Matter Device Type Library Specification.
   When creating an official product, follow the Matter Device Type Library Specification.


.. note::
   Make sure you are familiar with Matter in the |NCS| and you have tested some of the available :ref:`matter_samples` before you work with this user guide.

.. _ug_matter_creating_accessory_overview:

Overview
********

The Matter device is a basic node of the `Matter`_ network.
The device is formed by the development kit and the application that is running the Matter stack, which is programmed on the development kit.

Each Matter application consists of the following layers:

* Matter stack that provides the Matter core components.
* Data Model layer in the form of clusters, which contains commands and attributes that are to be accessible over the Matter network.
  This layer can be further broken down into the following groups:

  * Utility clusters - These clusters represent management and diagnostic features of a Matter node.
    They are common for all Matter nodes.
  * Application clusters - These clusters represent functionalities specific to a given application.

* Application logic, such as turning on and off a light bulb in response to certain commands.

Creating a Matter device consists of adding new application clusters to the Matter template sample.
By default, the template sample includes only mandatory Matter clusters, necessary to commission the device into a Matter network.

.. figure:: images/matter_template_sample.svg
   :alt: Creating Matter device

   Creating Matter device

Cluster is a Data Model building block in Matter.
It is a representation of a single functionality within a Matter device, such as turning a device on and off.
Each cluster contains attributes, commands, and events, which can be mandatory or optional.
Attributes are stored in the device's memory, while commands can be used to modify or read the state of the device, including the cluster attributes.

Clusters appropriate for a single device type such as a sensor or a light bulb are organized into an addressable container that is called an *endpoint*.
Most utility clusters are required to be on the endpoint with ID ``0``.
Application clusters are usually assigned to endpoints with IDs ``1`` and higher.

An application can implement appropriate callback functions to be informed about specific cluster state changes.
These functions can be used to alter the device's behavior when the state of a cluster is changing as a result of some external event.

For more information about the Data Model layer, see :ref:`ug_matter_architecture_overview_dm` section on the Matter architecture documentation page.

Requirements
************

To take advantage of this guide, you need to be familiar with :ref:`ug_matter_architecture` and :ref:`ug_matter_configuring`, and have built and tested at least one of the available :ref:`matter_samples`.

.. _ug_matter_creating_accessory_copy:

.. rst-class:: numbered-step

Copy Matter template sample
***************************

Use the :ref:`Matter Template <matter_template_sample>` sample as the base for building a sensor device:

1. Make sure that you meet the requirements for building the sample.
#. Build and test the sample as described on its documentation page.
#. Copy the contents of the :file:`samples/matter/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/matter/sensor`.

.. _ug_matter_creating_accessory_edit_zap:

.. rst-class:: numbered-step

Edit clusters using the ZAP tool
********************************

Adding the functionalities for an on/off switch and a sensor requires adding new clusters.

Adding new application clusters can be achieved by modifying ZAP file, which can be found as :file:`src/template.zap`.
This is a JSON file that contains the data model configuration of clusters, commands, and attributes that are enabled for a given application.
It is not used directly by Matter applications, but it is used to generate the source files for handling given clusters.

The ZAP file can be edited using `ZCL Advanced Platform`_ (ZAP tool), a third-party tool that is a generic templating engine for applications and libraries based on Zigbee Cluster Library.

This guide uses the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool` to install and run the ZAP tool GUI, and generate the data model's C++ source files.

To edit clusters using the ZAP tool, complete the following steps:

1. Navigate to your sample directory and run the following command:

   .. code-block::

      west zap-gui


   .. note::
      The ZAP tool UI may vary depending on the ZAP version.
      The following steps should be considered as guidelines.


   The ZAP tool's Matter Cluster Configurator window appears.

   .. figure:: images/matter_create_accessory_zcl_configurator.png
      :alt: Zigbee Cluster Configurator window in ZAP tool

      Zigbee Cluster Configurator window in ZAP tool

   By default, the window displays all available clusters.
   These can be filtered to show :guilabel:`Only Enabled` clusters.
   At this stage, only one endpoint is available (Endpoint 0).
#. In the ZAP tool, click :guilabel:`ADD NEW ENDPOINT`.
#. In the :guilabel:`Create New Endpoint` menu, create a new endpoint that represents the temperature sensor device type:

   .. figure:: images/matter_create_accessory_create_new_endpoint.png
      :alt: Create New Endpoint menu in ZAP tool

      Create New Endpoint menu in ZAP tool

   The new endpoint is created with both the Descriptor and Identify clusters enabled.
#. Configure the On/Off cluster for this endpoint, as it will be used in this example:

   a. In the :guilabel:`Search Clusters` menu, find the On/Off cluster.
   #. Set the :guilabel:`Server` option for the On/Off cluster.

      .. figure:: images/matter_create_accessory_add_onoff_cluster.png
         :alt: Configuring the On/off server cluster

         Configuring the On/off server cluster

   #. In the :guilabel:`Configure` column, click the gear icon to open the cluster's configuration.
   #. In the :guilabel:`ATTRIBUTES` tab, make sure that you have the ``OnOff`` attribute enabled.
   #. In the :guilabel:`COMMANDS` tab, make sure that you have both On and Off commands enabled:

   .. figure:: images/matter_create_accessory_enable_onoff_commands.png
      :alt: On/off cluster configuration

      On/off cluster configuration

#. Configure the Temperature Measurement cluster required for this endpoint:

   a. Expand the :guilabel:`Measurement & Sensing` menu and configure the Temperature Measurement cluster by setting the :guilabel:`Server` option from the drop-down menu.

      .. figure:: images/matter_create_accessory_add_temperature_measurement.png
         :alt: Configuring the Temperature Measurement server cluster

         Configuring the Temperature Measurement server cluster

   #. Go to the Temperature Measurement cluster configuration and make sure that you have the ``MeasuredValue`` attribute enabled.

#. Save the file and exit.
#. Use the modified ZAP file to generate the C++ code that contains the selected clusters by running the following command:

   .. code-block::

      west zap-generate

At this point, new clusters have been added to the Matter device.

.. note::
   On the first run the ZAP tool creates a :file:`.zap` directory to store cached information for the following runs.
   The default directory location is the user's home directory and it can be overridden by adding ``--stateDirectory`` and the location path to the invoked ZAP commands.

   Introducing significant changes to the ZAP tool configuration, such as updating the tool version or changing which ZCL templates are used, can result in unexpected issues with the application when previously cached information in the :file:`.zap` directory is used.
   The behavior of the application in such a case is undefined and it depends on the difference between the new configuration and the old cached data.
   For example, it could result in problems with displaying specific information in the UI, generating new configuration, or even application crashes.
   The solution is to remove the :file:`.zap` directory to clear the cached information.

.. _ug_matter_creating_accessory_edit_loop:

.. rst-class:: numbered-step

Edit the main loop of the application
*************************************

After adding clusters, you must modify the way in which the application interacts with the newly added clusters.
This is needed to properly model the sensor's behavior.

The :file:`src/app_task.cpp` file contains the main loop of the application.
Complete the steps in the following subsections to modify the main loop.

Add new tasks
=============

The main application uses a task queue managed by the ``task_executor`` common module, on which tasks are posted by ZCL callbacks and by other application components, such as Zephyr timers.
In each iteration, a task is dequeued and a corresponding task handler is called.

To model the behavior of the sensor, you should add new tasks in the following subsections:

* ``Sensor Activate`` - For sensor activation.
* ``Sensor Deactivate`` - For sensor deactivation.
* ``Sensor Measure`` - For sensor measurement update.

Add sensor timer
----------------

You need to make sure that the sensor is making measurements at the required time points.
For this purpose, use a Zephyr timer to periodically post ``Sensor Measure`` tasks.
In the template sample, such a timer is being used to count down 6 seconds when **Button 1** is being pressed to initiate the factory reset.

To add a new timer for the measurement task, edit the :file:`src/app_task.cpp` file as follows:

.. code-block:: C++

   k_timer sSensorTimer;

   void SensorTimerHandler(k_timer *timer)
   {
           Nrf::PostTask([] { AppTask::SensorMeasureHandler(); });
   }

   void StartSensorTimer(uint32_t aTimeoutMs)
   {
           k_timer_start(&sSensorTimer, K_MSEC(aTimeoutMs), K_MSEC(aTimeoutMs));
   }

   void StopSensorTimer()
   {
           k_timer_stop(&sSensorTimer);
   }

   CHIP_ERROR AppTask::Init()
   {
           /*
           ... Original content
           */

           k_timer_init(&sSensorTimer, &SensorTimerHandler, nullptr);
           k_timer_user_data_set(&sSensorTimer, this);
           return Nrf::Matter::StartServer();
   }

The timer must be initialized in the ``Init()`` method of the ``AppTask`` class.
If :c:func:`StartSensorTimer()` is called, the ``Sensor Measure`` task is added to the tasks queue every *aTimeoutMs* milliseconds, until :c:func:`StopSensorTimer()` is called.

Implement task handlers
-----------------------

When a task is dequeued, the ``task_executor`` module calls the task handler passed to the :c:func:`PostTask()` function.
Because you need to handle new tasks, you must implement the corresponding handlers.

To add new task handlers, complete the following steps:

1. Edit the :file:`src/app_task.cpp` file as follows:

   .. code-block:: C++

      void AppTask::SensorActivateHandler()
      {
              StartSensorTimer(500);
      }

      void AppTask::SensorDeactivateHandler()
      {
              StopSensorTimer();
      }

      void AppTask::SensorMeasureHandler()
      {
              chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
                      /* endpoint ID */ 1, /* temperature in 0.01*C */ int16_t(rand() % 5000));
      }

   With this addition, when the sensor is active, the timer expiration event happens every half a second.
   This causes an invocation of :c:func:`SensorMeasureHandler()` and triggers an update of the ``MeasuredValue`` attribute of the Temperature Measurement cluster.

   .. note::
      In the code fragment, the example value is updated randomly, but in a real sensor application it would be updated with the value obtained from external measurement.

#. Declare these handler functions as ``static`` in the ``public`` scope of ``AppTask`` class in :file:`src/app_task.h` to make sure the application builds properly.

Include header for cluster attribute helpers
============================================

To import helper functions for accessing cluster attributes, make sure to include the following file in the :file:`src/app_task.cpp` file:

.. code-block:: C++

   #include <app-common/zap-generated/attributes/Accessors.h>

.. _ug_matter_creating_accessory_callback:

.. rst-class:: numbered-step

Create a callback for sensor activation and deactivation
********************************************************

Handlers for the ``Sensor Activate`` and ``Sensor Deactivate`` tasks are now ready, but the tasks are not posted to the task queue.
The sensor is supposed to be turned on and off remotely by changing the ``OnOff`` attribute of the On/off cluster, for example using the Matter controller.
This means that we need to implement a callback function to post one of these tasks every time the ``OnOff`` attribute changes.

To implement the callback function, complete the following steps:

1. Create a new file, for example :file:`src/zcl_callbacks.cpp`.
2. Implement the callback in this file:

   a. Open :file:`ncs/modules/lib/matter/src/app/util/generic-callback-stubs.cpp` to check the list of customizable callback functions, marked with ``__attribute__((weak))``.
   #. Read the description of :c:func:`MatterPostAttributeChangeCallback()` in the :file:`ncs/modules/lib/matter/src/app/util/generic-callbacks.h` file.
   #. Implement :c:func:`MatterPostAttributeChangeCallback()` in the :file:`src/zcl_callbacks.cpp` file.

For example, the implementation can look as follows:

.. code-block:: C++

   #include "app_task.h"
   #include "app/task_executor.h"

   #include <app-common/zap-generated/ids/Attributes.h>
   #include <app-common/zap-generated/ids/Clusters.h>
   #include <app/ConcreteAttributePath.h>

   using namespace ::chip;
   using namespace ::chip::app::Clusters;

   void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type,
                                          uint16_t size, uint8_t * value)
   {
            if (attributePath.mClusterId != OnOff::Id || attributePath.mAttributeId != OnOff::Attributes::OnOff::Id)
                   return;

            if (*value) {
                   Nrf::PostTask([] { AppTask::SensorActivateHandler(); });
            } else {
                   Nrf::PostTask([] { AppTask::SensorDeactivateHandler(); });
            }
   }

In this implementation, the ``if`` part filters out events other than those that belong to the On/Off cluster.
Then, the callback posts the task for the sensor, namely ``Sensor Activate`` if the current value of the attribute is not zero.

.. _ug_matter_creating_accessory_add_source:

.. rst-class:: numbered-step

Add new source file to CMakeLists
*********************************

To ensure that everything builds without errors, update the :file:`CMakeLists.txt` file by adding :file:`src/zcl_callbacks.cpp` source file to the ``app`` target.

.. _ug_matter_creating_accessory_test:

Testing the new sensor application
**********************************

.. note::
   Use CHIP Tool for Linux or macOS when :ref:`setting up Matter development environment <ug_matter_gs_testing_thread_separate_otbr_linux_macos>`.

To check if the sensor device is working, complete the following steps:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Commission the device into a Matter network by following the guides linked on the :ref:`ug_matter_configuring` page for the Matter controller you want to use.
   The guides walk you through the following steps:

   * Only if you are configuring Matter over Thread: Configure the Thread Border Router.
   * Build and install the Matter controller.
   * Commission the device.
     You can use the :ref:`matter_template_network_mode_onboarding` listed earlier on this page.
   * Send Matter commands.

   At the end of this procedure, the LED indicating the state of the Matter device programmed with the sample starts presenting the Solid On state.
   This indicates that the device is fully provisioned, and has established a CASE session with the controller.
#. Activate the sensor by running the following command on the On/off cluster with the correct *node_ID* assigned during commissioning:

   .. parsed-literal::
      :class: highlight

      ./chip-tool onoff on *node_ID* 1

#. Read the measurement several times by checking value of ``MeasuredValue`` in the Temperature Measurement cluster:

   .. parsed-literal::
      :class: highlight

      ./chip-tool temperaturemeasurement read measured-value *node_ID* 1

#. Deactivate the sensor by running the following command on the On/off cluster with the correct *node_ID* assigned during commissioning:

   .. parsed-literal::
      :class: highlight

      ./chip-tool onoff off *node_ID* 1

#. Read the measurement after the device has received the turning-off command.

#. Read the measurement again.
   The measurement should not change.
