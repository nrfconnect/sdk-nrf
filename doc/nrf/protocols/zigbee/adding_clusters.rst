:orphan:

.. _ug_zigee_adding_clusters:

Adding ZCL clusters to application
##################################

.. note::
   The contents of this user guide are inaccurate and will be updated in one of the upcoming releases.
   The information about the update will be posted in the release notes.
   For the time being, do not use this guide.

.. contents::
   :local:
   :depth: 2

Once you are familiar with Zigbee in the |NCS| and you have tested some of the available :ref:`zigbee_samples`, you can use the :ref:`Zigbee template <zigbee_template_sample>` sample to create your own application with a custom set of ZCL clusters.
For example, you can create a sensor application that uses a temperature sensor with an on/off switch, with the sensor periodically updating its measured value when it is active.

Adding ZCL clusters to an application consists of expanding the Zigbee template sample with new application clusters.
By default, the template sample includes only mandatory Zigbee clusters, which is the Basic and Identify clusters, required to identify a device within a Zigbee network.

Cluster is a representation of a single functionality within a Zigbee node.
Each cluster contains attributes that are stored in the device's memory and commands that can be used to modify or read the state of the device, including the cluster attributes.
Clusters appropriate for a single device type such as a sensor or a light bulb are organized into an addressable container that is called an endpoint.
For more information about the ZCL terminology, see `Common ZCL terms and definitions`_ in the ZBOSS stack user guide.

An application can implement appropriate callback functions to be informed about specific cluster state changes.
These functions can be used to alter the device's behavior when the state of a cluster is changing as a result of some external event.

Read the following sections for detailed steps about how to expand the Zigbee template sample.

.. _ug_zigee_adding_clusters_requirements:

Requirements
************

To take advantage of this guide, you need to be familiar with :ref:`ug_zigbee_architectures` and :ref:`ug_zigbee_configuring`, and have built and tested at least one of the available :ref:`zigbee_samples`.

To test the functionalities implemented in this user guide, you need the following hardware and software:

* One compatible development kit for programming the :ref:`Zigbee template <zigbee_template_sample>` sample
* One compatible development kit for programming the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample

You can also optionally use `nRF Sniffer for 802.15.4`_ configured for capturing Zigbee packets with Wireshark (see `Configuring Wireshark for Zigbee`_) to gather some of the required information, such as the short address of a Zigbee node.
However, this guide does not describe Wireshark usage.

.. _ug_zigee_adding_clusters_copying_template:

.. rst-class:: numbered-step

Copy Zigbee template sample
***************************

Use the :ref:`Zigbee template <zigbee_template_sample>` sample as the base for adding new ZCL clusters:

1. Make sure that you meet the requirements for building the sample.
#. Build and test the sample as described on its documentation page.
#. Copy the contents of the :file:`samples/zigbee/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/zigbee/sensor`.

.. _ug_zigee_adding_clusters_adding_clusters:

.. rst-class:: numbered-step

Add On/Off Switch and Temperature Sensor functionalities
********************************************************

As mentioned in the introduction, each functionality of a Zigbee node is defined as a cluster, with its unique group of attributes, commands, or functions.
Clusters can be grouped into devices with unique device identifiers.
Moreover, clusters can have either server or client role, which defines the way they are used.

To learn more about the ZCL clusters and their terminology, see the following documentation available to the `Connectivity Standards Alliance`_ members:

* Home Automation Profile specification
* ZCL library specification, Client/Server Model section

Extending the Zigbee node's functionalities with an On/Off Switch and a Temperature Sensor requires adding On/Off Switch and Temperature Sensor logical devices defined in the Home Automation Profile specification.

.. _ug_zigee_adding_clusters_add_switch:

Add On/Off Switch device
========================

To create an On/Off Switch device, we are going to declare the following clusters:

* Identify
* Basic
* On/Off Switch Configuration
* On/Off
* Scenes
* Groups

We are going to create a cluster list that specifies an On/Off Switch device using the `ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST`_ macro, which declares a static array of clusters.
Before we declare the cluster list, we are going to create clusters with attributes that can be manipulated, while the macro is going to seamlessly declare clusters that lacks attributes:

* Identify and Basic - These clusters are already declared in the Zigbee template sample.
* On/Off Switch Configuration - This cluster we are going to create manually.
* On/Off, Scenes, and Groups - These clusters are going to be declared by the macro.

Complete the following steps:

1. Open the :file:`main.c` file of the copied template sample.
   You can find this file in the directory to which you :ref:`copied the sample <ug_zigee_adding_clusters_copying_template>`.
#. Define variables for the On/Off Switch Configuration cluster's attributes and declare attribute list for them by embedding these attributes into the ``zb_device_ctx`` structure:

   .. code-block:: C++

        struct zb_device_ctx {
                zb_zcl_basic_attrs_t     basic_attr;
                zb_zcl_identify_attrs_t  identify_attr;
                zb_uint8_t               on_off_switch_type_attr;
                zb_uint8_t               on_off_switch_actions_attr;
        };

        ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(
                on_off_switch_attr_list,
                &dev_ctx.on_off_switch_type_attr, &dev_ctx.on_off_switch_actions_attr);

   At this point, you have all the clusters required to declare the On/Off Switch cluster list.
#. Create the On/Off Switch cluster list using the Basic and Identify clusters from the Zigbee template sample and the On/Off Switch Configuration cluster you've just created, as shown in the following snippet:

   .. code-block:: C++

        ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST(on_off_switch_clusters,
                on_off_switch_attr_list, basic_attr_list, identify_attr_list);

   You can read more about this step in the `Declaring attributes`_ section of the ZBOSS stack documentation.
#. Choose and declare the endpoint for the On/Off Switch device.
   For example:

   .. code-block:: C++

        // Exemplary 11. endpoint will be used for On/Off Switch cluster
        #define ON_OFF_SWITCH_ENDPOINT          11

        ZB_HA_DECLARE_ON_OFF_SWITCH_EP(on_off_switch_ep, ON_OFF_SWITCH_ENDPOINT, on_off_switch_clusters);

   Every cluster in the On/Off cluster list that you have declared in the previous step is going to use the same endpoint.

   .. note::
        Every endpoint has an associated Simple Descriptor, which contains a variety of information, such as the application profile identifier, the number of input and output clusters, or the device version.
        Simple Descriptors are used to find and identify specific devices in the Zigbee network, for example to bind a light switch with a light bulb.
        Declaring an endpoint for a device (in this case, the On/Off Switch device) actually defines a Simple Descriptor for the endpoint.
        You can read more about Simple Descriptors in the official `Zigbee Cluster Library specification`_ from `Connectivity Standards Alliance`_.

   You can read more about this step in the `Declaring endpoint`_ and `Declaring simple descriptors`_ sections of the ZBOSS stack documentation.
#. Create an application context with all declared endpoints, given that the Zigbee template sample declares the device context for a single endpoint.
   Modify this declaration, so that the device can have another endpoint for the On/Off device:

   .. code-block:: C++

        ZBOSS_DECLARE_DEVICE_CTX_2_EP(app_template_ctx, on_off_switch_ep, app_template_ep);


   You can read more about this step in the `Declaring Zigbee device context`_ and `Declaring Zigbee device context with multiple endpoints`_ sections of the ZBOSS stack documentation.
#. Make sure that the device context is registered in the ``main`` function, within the `ZB_AF_REGISTER_DEVICE_CTX`_ macro:

   .. code-block:: C++

        ZB_AF_REGISTER_DEVICE_CTX(&app_template_ctx);

   This creates a link between the application device context and the internal ZBOSS structures.
   You can read more about this step in the `Registering device context`_ section of the ZBOSS stack documentation.

.. _ug_zigee_adding_clusters_add_temp:

Add Temperature Sensor device
=============================

The process of adding Temperature Sensor device is similar to :ref:`adding the On/Off switch device <ug_zigee_adding_clusters_add_switch>`.

To create a Temperature Sensor device, we are going to declare the following clusters:

* Identify and Basic - These clusters are already declared in the Zigbee template sample.
* Temperature Measurement - This cluster we are going to create manually.

This time, we are going to use the `ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST`_ macro.

Complete the following steps:

1. In the :file:`main.c` file of the copied template sample, extend the ``zb_device_ctx`` structure with the Temperature Measurement attributes and declare the attributes list.

   In case of Temperature Measurement cluster, variables needed to hold its attributes are declared in the ``zb_zcl_temp_measurement_attrs_t`` structure, which is defined in :file:`addons/zcl/zb_zcl_temp_measurement_addons.h`.
   Some of clusters have its attributes combined into helper structures in :file:`addons/zcl/zb_zcl_*_addons.h`.
   The following snippet shows how to include the header and add a new field in ``zb_device_ctx``, and then declare the attribute list:

   .. code-block:: C++

        #include <addons/zcl/zb_zcl_temp_measurement_addons.h>


        struct zb_device_ctx {
                zb_zcl_basic_attrs_t            basic_attr;
                zb_zcl_identify_attrs_t         identify_attr;
                zb_uint8_t                      on_off_switch_type_attr;
                zb_uint8_t                      on_off_switch_actions_attr;
                zb_zcl_temp_measurement_attrs_t temp_measure_attrs;
        };


        ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_measurement_attr_list,
						    &dev_ctx.temp_measure_attrs.measure_value,
						    &dev_ctx.temp_measure_attrs.min_measure_value,
						    &dev_ctx.temp_measure_attrs.max_measure_value,
						    &dev_ctx.temp_measure_attrs.tolerance);

#. Create a Temperature Sensor device by declaring its cluster list using Basic, Identify, and the newly created Temperature Measurement clusters:

   .. code-block:: C++

        ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(temperature_sensor_clusters, basic_attr_list, identify_attr_list, temp_measurement_attr_list);

#. Choose and declare the endpoint for the Temperature Sensor device:

   .. code-block:: C++

        #define TEMPERATURE_SENSOR_ENDPOINT  12

        ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP(temperature_sensor_ep, TEMPERATURE_SENSOR_ENDPOINT, temperature_sensor_clusters);

#. Declare the device context for the created endpoint by modifying the device context declaration, so that the device can have another endpoint:

   .. code-block:: C++

        ZBOSS_DECLARE_DEVICE_CTX_3_EP(app_template_ctx, temperature_sensor_ep, on_off_switch_ep, app_template_ep);

At this point, we have added On/Off Switch and Temperature Sensor functionalities to the application.

.. _ug_zigee_adding_clusters_testing:

.. rst-class:: numbered-step

Verify cluster changes
**********************

To verify the existence of the On/Off Switch and Temperature Sensor clusters in the device, we are going to send ZDO commands to read Simple Descriptors.
For this purpose, we are going to use the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` to create a simple Zigbee network with the node that is programmed with the extended application.

Prepare network coordinator for testing
=======================================

To prepare the network coordinator for testing the newly extended application based on the Zigbee template, complete the following steps:

1. Make sure you meet the requirements to use the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample.
#. Enable the Zigbee shell in the network coordinator sample by adding the following line to :file:`network_coordinator/prj.conf` file:

   .. code-block::

      CONFIG_ZIGBEE_SHELL=y

#. Build the sample and program it to the development kit.
#. Open the serial port and issue the ``help`` command.
   The following output appears:

   .. include:: ../../libraries/zigbee/shell.rst
      :start-after: zigbee_help_output_start
      :end-before: zigbee_help_output_end

This output means that the Zigbee shell is enabled on the network coordinator node device.
You can read more about the Zigbee shell on its :ref:`documentation page <lib_zigbee_shell>`.

Run the extended application
============================

We are now going to add the extended application node device to the Zigbee network.
Complete the following steps:

1. Make sure that the network coordinator node device is running.
#. Build the extended application and program it to a compatible development kit, which is one of the development kits compatible with the template sample.
#. |connect_kit|
#. |connect_terminal|
#. Observe the output.
   The device connects to the Zigbee network when a notification similar to the following one appears:

   .. code-block::

      I: Joined network successfully (Extended PAN ID: f4ce36e005691785, PAN ID: 0xf7a7)

   The Extended PAN ID and the PAN ID in your notification will be different.

Verify cluster operation
========================

Reading Simple Descriptors that we have implemented when :ref:`adding new clusters <ug_zigee_adding_clusters_adding_clusters>` can help us verify that everything is working as it should.
You can read the descriptors in different ways, but for the purpose of this guide we are going to use Zigbee Device Object commands (ZDO commands) issued from the network coordinator node with the Zigbee shell enabled.
ZDO is an interface for accessing lower layers of the Zigbee network.
Issuing ZDO commands allows us to check different information about the network.

For this guide, you can use one of the following options:

* Issue ZDO commands to find the specific cluster in the Zigbee network, if you don't know the device's short address.
* Issue ZDO commands to read the list of clusters contained on a device, if you know the device's short address.

Both options are described in the following sections.

Looking for a specific cluster in the Zigbee network
----------------------------------------------------

You can send a match descriptor request when you want to find a specific cluster in the network.
This request is a broadcast command that expects profile and cluster IDs as return values.
When using the network coordinator node with the Zigbee shell enabled, you can send the ZDO match descriptor request in the following way:

1. Make sure that the network coordinator is still connected with the serial port.
#. Issue the ``zdo help`` command to print the information about the ``zdo`` commands, including the match descriptor command.
   The following output appears:

   .. code-block::

        [...]

        match_desc - Send match descriptor request.
                Usage: match_desc <h:16-bit destination_address> <h:requested
                address/type> <h:profile ID> <d:number of input clusters> [<h:input
                cluster IDs> ...] <d:number of output clusters> [<h:output cluster
                IDs> ...] [-t | --timeout d:number of seconds to wait for answers]

        [...]

#. Send the :ref:`zdo_match_desc` command to find a device with the On/Off cluster (ID: ``0x0006``) from the Home Automation profile (ID: ``0x0104``) using the following command:

   .. code-block:: console

      zdo match_desc 0xfffd 0xfffd 0x0104 0 1 0x06

   The following output appears:

   .. code-block:: console

      Sending broadcast request.

      src_addr=8083 ep=11

Reading a clusters list of a specific Zigbee node
-------------------------------------------------

To read the cluster list existing on a Zigbee node, you can use the Simple Descriptor Request command.
This command requires the short address and the endpoint as arguments.

Complete the following steps to read the cluster list of a Zigbee node:

1. Send the match descriptor request to learn the short address of device, as described in `Looking for a specific cluster in the Zigbee network`_:

   .. code-block:: console

      zdo match_desc 0xfffd 0xfffd 0x0104 0 1 0x06

   The following output appears, where ``8083`` is the short address of the Zigbee node and ``11`` is the endpoint number of the On/Off switch device:

   .. code-block:: console

      Sending broadcast request.

      src_addr=8083 ep=11

   Alternatively, you can check the short address by looking at the network coordinator logs showed during device association or by sniffing the communication and reading packets in Wireshark (see `Configuring Wireshark for Zigbee`_).
#. Send the :ref:`zdo_simple_desc_req` command to the Zigbee node using the short address and the endpoint number:

   .. code-block:: console

      zdo simple_desc_req 0x8083 11

   The output similar to the following one appears:

   .. code-block:: console

      src_addr=0x8083 ep=11 profile_id=0x0104 app_dev_id=0x0 app_dev_ver=0x0
      in_clusters=0x0000,0x0003,0x0007 out_clusters=0x0006,0x0005,0x0004,0x0003

   In this notification, the simple descriptor contains Basic Cluster, Identify Cluster and On/Off Switch Configuration with server roles (``in_clusters``) and Identify, Groups, Scenes, and On/Off configured with client roles (``out_clusters``).

Further expanding the custom application
****************************************

You can further expand the application with more features, such as OTA support.

.. _ug_zigee_adding_clusters_ota:

Adding OTA
==========

To extend the sample with OTA support, we would have to complete steps similar to :ref:`adding On/Off Switch and Temperature Sensor functionalities <ug_zigee_adding_clusters_adding_clusters>`.
Then, we would have to implement the ZCL device callback to control the process of collecting chunks of new firmware.
This is described more broadly in the following sections.

Fortunately, we can use the :ref:`lib_zigbee_fota` library to handle the majority of these implementation steps.
To add OTA support to the extended application, follow the steps in :ref:`ug_zigbee_configuring_components_ota`.


.. _ug_zigee_adding_clusters_passing_events:

Passing ZCL events to the application
=====================================

Declaring and registering a set of clusters that defines a Zigbee node makes these clusters discoverable across the Zigbee network and ready to communicate with another nodes.
For example, the communication between a light switch node and a light bulb node uses ZCL commands to change the attributes of the On/Off device embedded in the light bulb.
Altering the attribute does nothing more than changing values of a specific variable.
The application is supposed to react to these changes and produce appropriate behavior, but we need to inform it about these changes first.

To inform the application about attributes changes, you can pass ZCL events to it with a callback that follows generic callback definition (referred to as *ZCL callback*).
This is shown in the following snippet:

.. code-block:: C++

   typedef void (zb_callback_t)(zb_uint8_t param);

The ``param`` argument passed to the callback contains information about the changed attributes.
This argument is actually a ZBOSS buffer that contains the `zb_zcl_device_callback_param_t`_ structure whose definition fragment is as follows:

.. code-block:: C++

        /* For the full definition please refer to zboss_api_zcl.h */
        typedef struct zb_zcl_device_callback_param_s
        {
                /** Type of device callback */
                zb_zcl_device_callback_id_t device_cb_id;
                zb_uint8_t endpoint;
                zb_zcl_attr_access_t attr_type;

                /** Return status (see zb_ret_t) */
                zb_ret_t status;

                /** Callback custom data */
                union
                {
                        zb_zcl_set_attr_value_param_t  set_attr_value_param;
                        #if defined (ZB_ZCL_SUPPORT_CLUSTER_ON_OFF)
                        /* Off with effect command, On/Off cluster */
                        zb_zcl_on_off_set_effect_value_param_t  on_off_set_effect_value_param;
                        /* */
                        #endif
                        #if defined(ZB_ZCL_SUPPORT_CLUSTER_IDENTIFY)
                        zb_zcl_identify_effect_value_param_t  identify_effect_value_param;
                        #endif

                        /* .
                           .
                           .
                        */

                        zb_zcl_device_cmd_generic_param_t gnr;
                } cb_param;
        } zb_zcl_device_callback_param_t;

Both the On/Off Switch device and the Temperature Sensor device have client roles.
For this reason, their attributes are not supposed to change.
To show how you can use the ZCL event, read the following section.

Reading device callback parameters from a ZBOSS buffer
------------------------------------------------------

The ZCL callback implemented in the :ref:`lib_zigbee_fota` library is a good example of how to use the ZCL event.

To read the device callback parameters from a ZBOSS buffer, complete the following steps:

1. In the :file:`main.c` file of the copied template sample, use the `zb_zcl_device_callback_param_t`_ structure to get the ZCL callback parameters from the buffer in the following manner:

   .. code-block:: C++

      zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

   Once the parameters are obtained, the application can use them to perform some action based on a new attribute values.
#. Make the application check the callback ID by reading the appropriate field from the `zb_zcl_device_callback_id_t`_ structure.
   For example, the Zigbee FOTA library uses the `ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID`_ macro:

   .. code-block:: C++

      if (device_cb_param->device_cb_id != ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID) {
        	return;
      }

   Depending on the device callback ID, different data is passed to the callback and held by the ``cb_param`` field of `zb_zcl_device_callback_param_t`_.
   In general, the data associated with the callback ID is contained in the ``set_attr_value_param`` field of ``cb_param``, but some clusters have their data structure already defined.
   For example, OTA uses ``ota_value_param`` fields, as shown in the following snippet:

   .. code-block:: C++

      zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value = &(device_cb_param->cb_param.ota_value_param);

   To see the field usage associated with other clusters, refer to the :ref:`zigbee_light_bulb_sample` sample.
#. Make the ZCL callback pass the status of its execution to the caller by setting the appropriate return status in the ``status`` field of the `zb_zcl_device_callback_param_t`_ structure passed to the callback:

   .. code-block:: C++

      device_cb_param->status = RET_OK;
