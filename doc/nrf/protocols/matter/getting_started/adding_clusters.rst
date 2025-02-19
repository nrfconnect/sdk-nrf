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

See the :ref:`Custom clusters <ug_matter_creating_accessory_vendor_cluster>` section on how to add a custom cluster to the Matter application.

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

1. |open_terminal_window_with_environment|
#. Navigate to your sample directory and run the following command:

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
If :c:func:`StartSensorTimer` is called, the ``Sensor Measure`` task is added to the tasks queue every *aTimeoutMs* milliseconds, until :c:func:`StopSensorTimer` is called.

Implement task handlers
-----------------------

When a task is dequeued, the ``task_executor`` module calls the task handler passed to the :c:func:`PostTask` function.
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
   This causes an invocation of :c:func:`SensorMeasureHandler` and triggers an update of the ``MeasuredValue`` attribute of the Temperature Measurement cluster.

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
   #. Read the description of :c:func:`MatterPostAttributeChangeCallback` in the :file:`ncs/modules/lib/matter/src/app/util/generic-callbacks.h` file.
   #. Implement :c:func:`MatterPostAttributeChangeCallback` in the :file:`src/zcl_callbacks.cpp` file.

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

.. _ug_matter_creating_accessory_vendor_cluster:

Custom clusters
***************

A custom cluster is a manufacturer-specific cluster that is not defined in the Matter Device Type Library Specification.
The cluster description is written in XML format and is used to generate the C++ source files that provide the cluster implementation.
You can add the cluster to the Matter data model definition file and use it in the :ref:`ug_matter_gs_tools_zap` to generate the source files.

To add a custom cluster to the Matter application, complete the following steps:

1. Create a new cluster description file in XML format.

   The file should contain cluster definitions, attributes, commands, events, enums, and structs in XML format.
   The cluster ``<code>`` is a 32-bit combination of the manufacturer code and it should be unique and not conflict with existing clusters.
   The most significant 16 bits are the manufacturer code, and the least significant 16 bits are the cluster code.
   In the example, there is a cluster with the code ``0xFFF11234``, which means ``0xFFF1`` is the Test Manufacturer ID, and ``0x1234`` is the cluster ID.
   See the :file:`<default Matter SDK location>/src/app/zap-templates/zcl/data-model/manufacturers.xml` file to learn about the manufacturer codes.

   The XML file consists of the following elements:

   * ``<enum>`` - Enumerated type definition.
   * ``<struct>`` - Structure definition.
   * ``<cluster>`` - Cluster definition.

   See the description of each element in the following tabs:

   .. tabs::

      .. tab:: ``<enum>``

         ``<enum>`` elements define the enumerated types that can be used in the cluster and consist of the following:

         * ``name`` - The unique name of the enumerated type.
         * ``<cluster code>`` - The cluster code(s) that the enumerated type is associated with.
           An enumerated type can be associated with multiple clusters by defining multiple ``<cluster code>`` elements.
           If no cluster code is specified, the enumerated type has a global scope.
         * ``type`` - The data type of the enumerated values.
         * ``<item>`` - The definition of an individual item within the enumerated type.

            * ``name`` - The name of the item.
            * ``value`` - The value assigned to the item, which must match the specified data type of the enumerated type.

         For example, the following XML code defines an enumerated type with two items:

         .. code-block:: xml

            <enum name="MyNewEnum" type="uint8">
               <cluster code="0xFFF11234" />
               <item name="EnumValue1" value="0" />
               <item name="EnumValue2" value="1" />
            </enum>

      .. tab:: ``<struct>``

         ``<struct>`` elements define the structure types that can be used in the cluster and consist of the following:

         * ``name`` - The unique name of the structure.
         * ``isFabricScoped`` - Indicates if the structure is fabric-scoped.
         * ``<cluster code>`` - The cluster code(s) that the structure is associated with.
           A structure can be associated with multiple clusters by defining multiple ``<cluster code>`` elements.
           If no cluster code is specified, the structure has a global scope.
         * ``<item>`` - The definition of an individual item within the structure.

            * ``fieldId`` - The unique ID of the item within the structure.
            * ``name`` - The name of the item.
            * ``type`` - The data type of the item.
            * ``array`` - Indicates if the item is an array.
            * ``minLength`` - The minimum length of the array, if applicable.
            * ``maxLength`` - The maximum length of the array, if applicable.
            * ``isNullable`` - Indicates if the item can be NULL.
            * ``isFabricSensitive`` - Indicates if the item is fabric-sensitive.
            * ``min`` - The minimum value of the item, if applicable.
            * ``max`` - The maximum value of the item, if applicable.

         For example, the following XML code defines a structure with one item of type octet string and length 128:

         .. code-block:: xml

            <struct name="MyStruct" isFabricScoped="true">
               <cluster code="0xFFF11234"/>
               <item fieldId="1" name="Data" type="octet_string" length="128" isFabricSensitive="true"/>
            </struct>

      .. tab:: ``<cluster>``

         ``<cluster>`` element can be only one within the file and it defines the cluster and consist of the following:

         * ``<domain>`` - The domain to which the cluster belongs.
         * ``<name>`` - The name of the cluster.
         * ``<code>`` - A 32-bit identifier for the cluster, combining the manufacturer code and a unique cluster code.
         * ``<define>`` - The C++ preprocessor macro name for the cluster, typically in uppercase with words separated by underscores.
         * ``<description>`` - A brief description of the cluster's purpose and functionality.
         * ``<attribute>`` - An attribute definition within the cluster.

            * ``side`` - Specifies whether the attribute is on the client or server side.
            * ``code`` - A unique identifier for the attribute within the cluster.
            * ``define`` - The C++ preprocessor macro name for the attribute, typically in uppercase with words separated by underscores.
            * ``type`` - The data type of the attribute.
            * ``entryType`` - The data type of array elements if the attribute is an array.
            * ``writable`` - Indicates whether the attribute can be written to.
            * ``default`` - The default value of the attribute.
            * ``optional`` - Indicates whether the attribute is optional.
            * ``name`` - The name of the attribute.

         * ``<command>`` - A command definition within the cluster.

            * ``source`` - Specifies whether the command originates from the client or server.
            * ``code`` - A unique identifier for the command within the cluster.
            * ``name`` - The name of the command.
            * ``optional`` - Indicates whether the command is optional.
            * ``disableDefaultResponse`` - Indicates whether the default response to the command is disabled.
            * ``response`` - The name of the response command, if any.
            * ``description`` - A brief description of the command's purpose and functionality.
            * ``arg`` - An argument for the command, specifying its name and type.

         * ``<event>`` - An event definition within the cluster.

            * ``source`` - Specifies whether the event originates from the client or server.
            * ``code`` - A unique identifier for the event within the cluster.
            * ``name`` - The name of the event.
            * ``optional`` - Indicates whether the event is optional.
            * ``description`` - A brief description of the event's purpose and functionality.
            * ``arg`` - An argument for the event, specifying its name and type.

         For example, the following XML code defines a cluster with one attribute, one command, and one event:

         .. code-block:: xml

            <?xml version="1.0"?>
            <cluster>
               <domain>General</domain>
               <name>MyNewCluster</name>
               <code>0xFFF11234</code>
               <define>MY_NEW_CLUSTER</define>
               <description>The MyNewCluster cluster showcases a cluster manufacturer extensions</description>
               <attribute side="server" code="0x0000" define="FLIP_FLOP" type="boolean" writable="true" default="false" optional="false">MyAttribute</attribute>
               <command source="client" code="0x02" name="MyCommand" response="MyCommandResponse" optional="false">
                     <description>
                        Command that takes two uint8 arguments and returns their sum.
                     </description>
                     <arg name="arg1" type="int8u"/>
                     <arg name="arg2" type="int8u"/>
               </command>
               <event source="server" code="0x01" name="MyEvent" optional="false">
                     <description>
                        Event that is generated by the server.
                     </description>
                     <arg name="arg1" type="int8u"/>
               </event>
            </cluster>

   .. note::
      The descriptions of the elements above show only the basic functionality.
      To see the full list of available elements for each part of the XML file, refer to the Matter Specification.

   You can use the following template for the :file:`MyCluster.xml` file that contains comments to guide you through the process:

   .. code-block:: xml

      <?xml version="1.0"?>
      <configurator>
         <enum name="MyNewEnum" type="uint8">
            <cluster code="0xFFF11234" />
            <item name="EnumValue1" value="0" />
            <item name="EnumValue2" value="1" />
         </enum>

         <struct name="MyStruct" isFabricScoped="true">
            <cluster code="0xFFF11234"/>
            <item fieldId="1" name="Data" type="octet_string" length="128" isFabricSensitive="true"/>
         </struct>

         <cluster>
            <domain>General</domain>
            <name>MyNewCluster</name>
            <code>0xFFF11234</code>
            <define>MY_NEW_CLUSTER</define>
            <description>The MyNewCluster cluster showcases a cluster manufacturer extensions</description>
            <attribute side="server" code="0x0000" define="FLIP_FLOP" type="boolean" writable="true" default="false" optional="false">MyAttribute</attribute>
            <command source="client" code="0x02" name="MyCommand" response="MyCommandResponse" optional="false">
                  <description>
                     Command that takes two uint8 arguments and returns their sum.
                  </description>
                  <arg name="arg1" type="int8u"/>
                  <arg name="arg2" type="int8u"/>
            </command>
            <event source="server" code="0x01" name="MyEvent" optional="false">
                  <description>
                     Event that is generated by the server.
                  </description>
                  <arg name="arg1" type="int8u"/>
            </event>
         </cluster>
      </configurator>

   For the guide purposes, save this file as :file:`MyCluster.xml` in the sample directory.

#. Add the cluster description file to the data model definition file and run the ZAP tool.

   The data model definition file contains all the cluster XML locations and manufacturers list.
   To work with the new custom cluster, you need to append it to the list in the existing data model definition file.

   You can use the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui` to add the cluster and run the ZAP tool, or :ref:`ug_matter_gs_tools_matter_west_commands_append` to add the cluster only without starting the ZAP tool.
   This guide has focus on the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui`.

   Run the following command:

   .. code-block::

      west zap-gui -j ./zcl.json ./MyCluster.xml

   This example command copies the original :file:`<default Matter SDK location>/src/app/zap-templates/zcl/zcl.json` file, adds the :file:`MyCluster.xml` cluster, and saves the new :file:`zcl.json` file in the sample directory.
   The newly generated :file:`zcl.json` file is used as an input to the ZAP tool.

#. Locate the new cluster in the ZAP tool.

   .. figure:: images/matter_create_accessory_custom_cluster.png
      :alt: New custom cluster in ZAP tool

      New custom cluster in ZAP tool

#. Choose whether the cluster should be enabled for the Client and Server sides.

#. Click the gear icon to open the cluster configuration and enable the attributes, commands, and events.

   a. In the :guilabel:`ATTRIBUTES` tab, ensure that you have the required attributes enabled.

      .. figure:: images/matter_create_accessory_custom_cluster_attributes.png
         :alt: Attributes of the new custom cluster in ZAP tool

         Attributes of the new custom cluster in ZAP tool

   #. In the :guilabel:`COMMANDS` tab, ensure that you have the required commands enabled.

      .. figure:: images/matter_create_accessory_custom_cluster_commands.png
         :alt: Commands of the new custom cluster in ZAP tool

         Commands of the new custom cluster in ZAP tool

   #. In the :guilabel:`EVENTS` tab, ensure that you have the required events enabled.

      .. figure:: images/matter_create_accessory_custom_cluster_events.png
         :alt: Events of the new custom cluster in ZAP tool

         Events of the new custom cluster in ZAP tool

#. Save the file and exit.

#. Run the following command to use the modified ZAP file to generate the C++ code that contains the selected clusters:

   .. code-block::

      west zap-generate

After completing these steps, the following changes will be visible within your sample directory:

* The new cluster description file :file:`MyCluster.xml`.
* The updated data model definition file :file:`zcl.json` with the new cluster and relative paths to the Matter data model directory.
* The generated C++ source files for the new cluster.
* The updated :file:`.zap` file with the new cluster configuration and relative path to the :file:`zcl.json` file.

Once the new cluster is added to the Matter application, you can call the ``zap-gui`` command without the additional ``--clusters`` argument.
However, you still need to provide the path to the :file:`zcl.json` file if you created a new one in a different location than the default one.
