.. _ug_matter_creating_accessory:

Creating Matter accessory device
################################

.. contents::
   :local:
   :depth: 2

The Matter accessory device is a basic node of the `Matter`_ network.
The accessory is formed by the development kit and the application that is running the Matter stack, which is programmed on the development kit.

Once you are familiar with Matter in the |NCS| and you have tested some of the available :ref:`matter_samples`, you can use the :ref:`Matter template <matter_template_sample>` sample to create your own custom accessory device application.
For example, you can create a sensor application that uses a temperature sensor with an on/off switch, with the sensor periodically updating its measured value when it is active.

Creating a Matter accessory device consists of adding new application clusters to the Matter template sample.
By default, the template sample includes only mandatory Matter clusters, necessary to commission the accessory into a Matter network.

Cluster is a representation of a single functionality within a Matter accessory.
Each cluster contains attributes that are stored in the device's memory and commands that can be used to modify or read the state of the device, including the cluster attributes.
Clusters appropriate for a single device type such as a sensor or a light bulb are organized into an addressable container that is called an endpoint.

An application can implement appropriate callback functions to be informed about specific cluster state changes.
These functions can be used to alter the device's behavior when the state of a cluster is changing as a result of some external event.

Read the following sections for detailed steps about how to expand the Matter template sample.

Requirements
************

To take advantage of this guide, you need to be familiar with :ref:`ug_matter_architecture` and :ref:`ug_matter_configuring`, and have built and tested at least one of the available :ref:`matter_samples`.

.. rst-class:: numbered-step

Copy Matter template sample
***************************

Use the :ref:`Matter Template <matter_template_sample>` sample as the base for building a sensor accessory device:

1. Make sure that you meet the requirements for building the sample.
#. Build and test the sample as described on its documentation page.
#. Copy the contents of the :file:`samples/matter/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/matter/sensor`.

.. rst-class:: numbered-step

Edit clusters using the ZAP tool
********************************

Adding the functionalities for an on/off switch and a sensor requires adding new clusters.

Adding new application clusters can be achieved by modifying ZAP file, which can be found as :file:`src/template.zap`.
This file can be edited using `ZCL Advanced Platform`_ (ZAP tool), a third-party tool that is a generic templating engine for applications and libraries based on Zigbee Cluster Library.
This tool is provided with the Matter submodule.

To edit clusters using the ZAP tool, complete the following steps:

1. Open the :file:`src/template.zap` for editing by running the following command:

   .. code-block::

      nrfconnect-dir/modules/lib/matter/scripts/tools/zap/run_zaptool.sh src/template.zap

   The ZAP tool's Zigbee Cluster Configurator window appears.

   .. figure:: images/matter_create_accessory_zcl_configurator.png
      :alt: Zigbee Cluster Configurator window in ZAP tool

      Zigbee Cluster Configurator window in ZAP tool

#. In the ZAP tool's :guilabel:`Create New Endpoint` menu, create a new endpoint that represents the sensor device:

   .. figure:: images/matter_create_accessory_create_new_endpoint.png
      :alt: Create New Endpoint menu in ZAP tool

      Create New Endpoint menu in ZAP tool

#. Configure the following clusters required for this endpoint:

   a. Expand the :guilabel:`General` menu and configure the On/off cluster by setting the :guilabel:`Server` option from the drop-down menu.

      .. figure:: images/matter_create_accessory_add_onoff_cluster.png
         :alt: Configuring the On/off server cluster

         Configuring the On/off server cluster

   #. Expand the :guilabel:`Measurement & Sensing` menu and configure the Temperature Measurement cluster by setting the :guilabel:`Server` option from the drop-down menu.

      .. figure:: images/matter_create_accessory_add_temperature_measurement.png
         :alt: Configuring the Temperature Measurement server cluster

         Configuring the Temperature Measurement server cluster

#. Go to the On/off cluster configuration and make sure that you have the ``OnOff`` attribute and both On and Off commands enabled:

   .. figure:: images/matter_create_accessory_enable_onoff_commands.png
      :alt: On/off cluster configuration

      On/off cluster configuration

#. Go to the Temperature Measurement cluster configuration and make sure that you have the ``MeasuredValue`` attribute enabled.
#. Save the file and exit.
#. Use the modified ZAP file to generate the C++ code that contains the selected clusters by running the following command:

   .. code-block::

      nrfconnect-dir/modules/lib/matter/scripts/tools/zap/generate.py $PWD/src/template.zap

At this point, new clusters have been added to the Matter accessory.

.. rst-class:: numbered-step

Edit the main loop of the application
*************************************

After adding clusters, you must modify the way in which the application interacts with the newly added clusters.
This is needed to properly model the sensor's behavior.

The :file:`src/app_task.cpp` file contains the main loop of the application.

Complete the steps in the following subsections to modify the main loop.

Edit the event queue
====================

The main application loop is based on a queue on which events are posted by ZCL callbacks and by other application components, such as Zephyr timers.
In each iteration, an event is dequeued and a corresponding event handler is called.

Add new events
--------------

In the template sample application, the events are representing the following actions:

* Pressing **Button 1**.
* Releasing **Button 1**.
* Factory reset timeout.

To model behaviour of a sensor, add the following new events to :c:enum:`EventType` in the :file:`src/app_event.h` file:

* Sensor activation.
* Sensor deactivation.
* Sensor measurement.

For example, the edited :c:enum:`EventType` can look as follows:

.. code-block:: C++

   enum EventType : uint8_t { FunctionPress, FunctionRelease, FunctionTimer, SensorActivate, SensorDeactivate, SensorMeasure };
   enum UpdateLedStateEventType : uint8_t { UpdateLedState = SensorMeasure + 1 };

Add sensor timer
----------------

You need to make sure that the sensor is making measurements at the required time points.
For this purpose, use a Zephyr timer to periodically post :c:struct:`SensorMeasure` events.
In the template sample, such a timer is being used to count down 6 seconds when **Button 1** is being pressed to initiate the factory reset.

To add a new timer for the measurement event, edit the :file:`src/app_task.cpp` file as follows:

.. code-block:: C++

   k_timer sSensorTimer;

   void SensorTimerHandler(k_timer *timer)
   {
           GetAppTask().PostEvent(AppEvent{ AppEvent::SensorMeasure });
   }

   void StartSensorTimer(uint32_t aTimeoutMs)
   {
           k_timer_start(&sSensorTimer, K_MSEC(aTimeoutMs), K_MSEC(aTimeoutMs));
   }

   void StopSensorTimer()
   {
           k_timer_stop(&sSensorTimer);
   }

   int AppTask::Init()
   {
           /*
           ... Original content
           */

           k_timer_init(&sSensorTimer, &SensorTimerHandler, nullptr);
           k_timer_user_data_set(&sSensorTimer, this);
           return 0;
   }

If :c:func:`StartSensorTimer()` is called, the :c:struct:`SensorMeasure` event is added to the event queue every *aTimeoutMs* milliseconds, until :c:func:`StopSensorTimer()` is called.

Implement event handlers
------------------------

When an event is dequeued, the application calls the event handler function.
Because you have added new events, you must implement the corresponding handlers.

To add new event handlers, complete the following steps:

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
   In the code fragment, the example value is updated randomly, but in a real sensor application it would be updated with the value obtained from external measurement.
#. Declare these handler functions in :file:`src/app_task.h` to make sure the application builds properly.
#. In the :file:`src/app_task.cpp` file, add cases for new events in :c:func:`DispatchEvent()`, for example:

   .. code-block:: C++

      void AppTask::DispatchEvent(const AppEvent &event)
      {
              switch (event.Type) {
              case AppEvent::FunctionPress:
                      FunctionPressHandler();
                      break;
              case AppEvent::FunctionRelease:
                      FunctionReleaseHandler();
                      break;
              case AppEvent::FunctionTimer:
                      FunctionTimerEventHandler();
                      break;
              case AppEvent::UpdateLedState:
                      event.UpdateLedStateEvent.LedWidget->UpdateState();
                      break;
              case AppEvent::SensorActivate:
                      SensorActivateHandler();
                      break;
              case AppEvent::SensorDeactivate:
                      SensorDeactivateHandler();
                      break;
              case AppEvent::SensorMeasure:
                      SensorMeasureHandler();
                      break;
              default:
                      LOG_INF("Unknown event received");
                      break;
              }
      }

Include header for cluster attribute helpers
============================================

To import helper functions for accessing cluster attributes, make sure to include the following file in the :file:`src/app_task.cpp` file:

.. code-block:: C++

   #include <app-common/zap-generated/attributes/Accessors.h>

.. rst-class:: numbered-step

Create a callback for sensor activation and deactivation
********************************************************

Handlers for the :c:struct:`SensorActivate` and :c:struct:`SensorDeactivate` events are now ready, but the events are not posted to the event queue.
The sensor is supposed to be turned on and off remotely by changing the ``OnOff`` attribute of the On/off cluster, for example using the Matter controller.
This means that we need to implement a callback function to post one of these events every time the ``OnOff`` attribute changes.

To implement the callback function, complete the following steps:

1. Create a new file, for example :file:`src/zcl_callbacks.cpp`.
2. Implement the callback in this file:

   a. Open :file:`src/zap-generated/callback-stub.cpp` to check the list of customizable callback functions, marked with ``__attribute__((weak))``.
   #. Read the description of :c:func:`MatterPostAttributeChangeCallback()`.
   #. Implement :c:func:`MatterPostAttributeChangeCallback()` in the :file:`src/zcl_callbacks.cpp` file.

For example, the implementation can look as follows:

.. code-block:: C++

   #include "app_task.h"

   #include <app-common/zap-generated/ids/Attributes.h>
   #include <app-common/zap-generated/ids/Clusters.h>
   #include <app/ConcreteAttributePath.h>

   using namespace ::chip;
   using namespace ::chip::app::Clusters;

   void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t mask, uint8_t type,
                                          uint16_t size, uint8_t * value)
   {
           if (attributePath.mClusterId != OnOff::Id || attributePath.mAttributeId != OnOff::Attributes::OnOff::Id)
                   return;

           GetAppTask().PostEvent(AppEvent(*value ? AppEvent::SensorActivate : AppEvent::SensorDeactivate));
   }

.. rst-class:: numbered-step

Add new source file to CMakeLists
*********************************

To ensure that everything builds without errors, update the :file:`CMakeLists.txt` file by adding :file:`src/zcl_callbacks.cpp` source file to the ``app`` target.

Testing the new sensor application
**********************************

.. note::
   Use the Python Controller when :ref:`setting up Matter development environment <ug_matter_configuring_mobile>`.

To check if the sensor device is working, complete the following steps:

.. include:: ../../samples/matter/template/README.rst
   :start-after: matter_template_sample_testing_start
   :end-before: #. Press **Button 1**

#. Activate the sensor by running the following command on the On/off cluster with the correct *node_ID* assigned during commissioning:

   .. parsed-literal::
      :class: highlight

      zcl OnOff On *node_ID* 1 0

#. Read the measurement several times by checking value of ``MeasuredValue`` in the Temperature Measurement cluster:

   .. parsed-literal::
      :class: highlight

      zclread TemperatureMeasurement MeasuredValue *node_ID* 1 0

#. Deactivate the sensor by running the following command on the On/off cluster with the correct *node_ID* assigned during commissioning:

   .. parsed-literal::
      :class: highlight

      zcl OnOff Off *node_ID* 1 0

#. Read the measurement again.
   The measurement should not change.
