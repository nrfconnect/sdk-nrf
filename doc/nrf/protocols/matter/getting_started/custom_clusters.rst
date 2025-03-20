.. _ug_matter_creating_accessory_vendor_cluster:
.. _ug_matter_creating_custom_cluster:

Creating custom clusters in Matter application
##############################################

This guide describes how you can create a custom cluster for the :ref:`matter_template_sample` sample.
The :ref:`matter_manufacturer_specific_sample` sample already contains a custom ``NordicDevkit`` cluster that you can use as a reference.

Overview
********

A custom cluster is a manufacturer-specific cluster that is not defined in the Matter Device Type Library Specification.
The cluster description is written in XML format and is used to generate the C++ source files that provide the cluster implementation.
You can add the cluster to the Matter data model definition file and use it in the :ref:`ug_matter_gs_tools_zap` to generate the source files.

Requirements
************

To take advantage of this guide, you need to be familiar with the :ref:`ug_matter_architecture` and :ref:`configuration <ug_matter_configuring>`, and have built and tested at least one of the available :ref:`matter_samples`.

.. rst-class:: numbered-step

Copy Matter template sample
***************************

Use the :ref:`matter_template_sample` sample as the base for building a sensor device as follows:

1. Make sure that you meet the requirements for building the sample.
#. Copy the contents of the :file:`samples/matter/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/matter/sensor`.
#. Build and test the sample as described on its documentation page.

.. rst-class:: numbered-step

Create a new cluster description file in XML format
***************************************************

The file should contain cluster definitions, attributes, commands, events, enums, structs, and device types in XML format.
The cluster ``<code>`` is a 32-bit combination of the vendor ID and cluster ID and must be unique, not conflicting with existing clusters.
The most significant 16 bits are the vendor ID, and the least significant 16 bits are the cluster ID.
The vendor ID must be configured according to the Matter specification (section 2.5.2 Vendor Identifier).
The cluster ID for a manufacturer-specific cluster must be in the range from ``0xFC00`` to ``0xFFFE``.
The example contains a cluster with the code ``0xFFF1FC01``, which means ``0xFFF1`` is the Test Manufacturer ID, and ``0xFC01`` is the cluster ID.
See the :file:`<default Matter SDK location>/src/app/zap-templates/zcl/data-model/manufacturers.xml` file to learn about the manufacturer codes.

The XML file consists of the following elements:

* ``<cluster>`` - Cluster definition.
* ``<clusterExtension>`` - Cluster extension definition.
* ``<enum>`` - Enumerated type definition.
* ``<struct>`` - Structure definition.
* ``<deviceType>`` - Device type definition.

See the description of each element in the following tabs:

.. tabs::

   .. tab:: ``<cluster>``

      ``<cluster>`` defines the cluster and consist of the following child elements:

      * ``<domain>`` - The domain to which the cluster belongs.
      * ``<name>`` - The name of the cluster.
      * ``<code>`` - A 32-bit identifier for the cluster, combining the vendor ID and a cluster ID.
      * ``<define>`` - The C++ preprocessor macro name for the cluster, typically in uppercase with words separated by underscores.
      * ``<description>`` - A brief description of the cluster's purpose and functionality.
      * ``<attribute>`` - An attribute definition within the cluster.

         * ``side`` - Specifies whether the attribute is on the client or server side.
         * ``code`` - A unique identifier for the attribute within the cluster.
         * ``define`` - The C++ preprocessor macro name for the attribute, typically in uppercase with words separated by underscores.
         * ``type`` - The data type of the attribute.
         * ``entryType`` - The data type of array elements if the attribute is an array.
         * ``writable`` - Indicates whether the attribute can be written by a Matter controller.
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
            <code>0xFFF1FC01</code>
            <define>MY_NEW_CLUSTER</define>
            <description>The MyNewCluster cluster showcases a cluster manufacturer extensions</description>
            <attribute side="server" code="0xFFF10000" define="MY_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">MyAttribute</attribute>
            <command source="client" code="0xFFF10000" name="MyCommand" response="MyCommandResponse" optional="false">
               <description>Command that takes two uint8 arguments and returns their sum.</description>
               <arg name="arg1" type="int8u"/>
               <arg name="arg2" type="int8u"/>
            </command>
            <event source="server" code="0xFFF10000" name="MyEvent" priority="info" optional="false">
               <description>Event that is generated by the server.</description>
               <arg name="arg1" type="int8u"/>
            </event>
         </cluster>

   .. tab:: ``<clusterExtension>``

      ``<clusterExtension>`` defines the extension of an existing cluster and consist of the following attributes and child elements:

      * ``code`` - A 32-bit identifier for the existing cluster, that will be extended.
      * ``<attribute>`` - An attribute definition within the cluster.

         * ``side`` - Specifies whether the attribute is on the client or server side.
         * ``code`` - A unique identifier for the attribute within the cluster.
         * ``define`` - The C++ preprocessor macro name for the attribute, typically in uppercase with words separated by underscores.
         * ``type`` - The data type of the attribute.
         * ``entryType`` - The data type of array elements if the attribute is an array.
         * ``writable`` - Indicates whether the attribute can be written by a Matter controller.
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
         * ``priority`` - The priority of the event.
                          The valid values are ``debug``, ``info``, and ``critical``.
         * ``optional`` - Indicates whether the event is optional.
         * ``description`` - A brief description of the event's purpose and functionality.
         * ``arg`` - An argument for the event, specifying its name and type.

      For example, the following XML code extends a ``Basic Information`` cluster with one attribute, one command, and one event:

      .. code-block:: xml

         <?xml version="1.0"?>
         <clusterExtension code="0x0028">
            <attribute side="server" code="0x17" define="EXTENDED_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">ExtendedAttribute</attribute>
            <command source="client" code="0x00" name="ExtendedCommand" response="ExtendedCommandResponse" optional="false">
               <description>Command that takes two uint8 arguments and returns their sum.</description>
               <arg name="arg1" type="int8u"/>
               <arg name="arg2" type="int8u"/>
            </command>
            <event source="server" code="0x04" name="ExtendedEvent" priority="info" optional="false">
               <description>Event that is generated by the server.</description>
               <arg name="arg1" type="int8u"/>
            </event>
         </clusterExtension>

   .. tab:: ``<enum>``

      ``<enum>`` elements define the enumerated types that can be used in the cluster and consist of the following attributes and child elements:

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
            <cluster code="0xFFF1FC01" />
            <item name="EnumValue1" value="0" />
            <item name="EnumValue2" value="1" />
         </enum>

   .. tab:: ``<struct>``

      ``<struct>`` elements define the structure types that can be used in the cluster and consist of the following attributes and child elements:

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
           <cluster code="0xFFF1FC01"/>
           <item fieldId="1" name="Data" type="octet_string" length="128" isFabricSensitive="true"/>
         </struct>

   .. tab:: ``<deviceType>``

      ``<deviceType>`` elements define the device types that can be used in the cluster and consist of the following child elements:

      * ``<name>`` - The unique name of the device.
      * ``<domain>`` - The domain to which the device belongs.
      * ``<typeName>`` - The name of the device displayed in the zap tool.
      * ``<profileId>`` - The profile ID of the device.

         * ``editable`` - Indicates if the field can be modified.

      * ``<deviceId>`` - The device ID.

         * ``editable`` - Indicates if the field can be modified.

      * ``<class>`` - The class of the device.
        Can be ``Utility``, ``Simple``, or ``Node``.
      * ``<scope>`` - The scope of the device.
        Can be ``Node``, or ``Endpoint``.
      * ``<clusters>`` - The definition of an individual item within the structure.

         * ``lockOthers`` - Indicates if other clusters are locked.
         * ``<include>`` - Defines a cluster that should be included in the device.

            * ``cluster`` - The name of the cluster.
            * ``client`` - Indicates if the client role should be enabled.
            * ``server`` - Indicates if the server role should be enabled.
            * ``clientLocked`` - Indicates if the client role modification should be locked.
            * ``serverLocked`` - Indicates if the server role modification should be locked.
            * ``<requireAttribute>`` - Indicates a required attribute's define.
            * ``<requireCommand>`` - Indicates a required command's define.

      For example, the following XML code defines a structure with one item of type octet string and length 128:

      .. code-block:: xml

         <deviceType>
            <name>my-new-device</name>
            <domain>CHIP</domain>
            <typeName>My new device</typeName>
            <profileId editable="false">0x0FFF</profileId>
            <deviceId editable="false">0x001</deviceId>
            <class>Simple</class>
            <scope>Endpoint</scope>
            <clusters lockOthers="true">
            <include cluster="MyNewCluster" client="true" server="true" clientLocked="false" serverLocked="false"/>
               <requireAttribute>MY_ATTRIBUTE</requireAttribute>
               <requireCommand>MyCommand</requireCommand>
            </clusters>
         </deviceType>

.. note::
   The descriptions of the elements above show only the basic functionality.
   To see the full list of available elements for each part of the XML file, refer to the Matter Specification.

You can use the following template for the :file:`MyCluster.xml` file:

.. code-block:: xml

   <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
   <configurator>
      <cluster>
         <domain>General</domain>
         <name>MyNewCluster</name>
         <code>0xFFF1FC01</code>
         <define>MY_NEW_CLUSTER</define>
         <description>The MyNewCluster cluster showcases a cluster manufacturer extensions</description>
         <attribute side="server" code="0xFFF10000" define="MY_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">MyAttribute</attribute>
         <command source="client" code="0xFFF10000" name="MyCommand" optional="false">
            <description>Command that takes two uint8 arguments and returns their sum.</description>
            <arg name="arg1" type="int8u"/>
            <arg name="arg2" type="int8u"/>
         </command>
         <event source="server" code="0xFFF10000" name="MyEvent" priority="info" optional="false">
            <description>Event that is generated by the server.</description>
            <arg name="arg1" type="int8u"/>
         </event>
      </cluster>
      <clusterExtension code="0x0028">
         <attribute side="server" code="0x17" define="EXTENDED_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">ExtendedAttribute</attribute>
         <command source="client" code="0x00" name="ExtendedCommand" response="ExtendedCommandResponse" optional="false">
            <description>Command that takes two uint8 arguments and returns their sum.</description>
            <arg name="arg1" type="int8u"/>
            <arg name="arg2" type="int8u"/>
         </command>
         <event source="server" code="0x04" name="ExtendedEvent" priority="info" optional="false">
            <description>Event that is generated by the server.</description>
            <arg name="arg1" type="int8u"/>
         </event>
      </clusterExtension>
      <enum name="MyNewEnum" type="int8u">
         <cluster code="0xFFF1FC01" />
         <item name="EnumValue1" value="0" />
         <item name="EnumValue2" value="1" />
      </enum>
      <struct name="MyStruct" isFabricScoped="true">
         <cluster code="0xFFF1FC01"/>
         <item fieldId="1" name="Data" type="octet_string" length="128" isFabricSensitive="true"/>
      </struct>
      <deviceType>
         <name>my-new-device</name>
         <domain>CHIP</domain>
         <typeName>My new device</typeName>
         <profileId editable="false">0x0FFF</profileId>
         <deviceId editable="false">0x001</deviceId>
         <class>Simple</class>
         <scope>Endpoint</scope>
         <clusters lockOthers="true">
            <include cluster="MyNewCluster" client="true" server="true" clientLocked="false" serverLocked="false"/>
         </clusters>
      </deviceType>
   </configurator>


For further guidance, save this file as :file:`MyCluster.xml` in the sample directory.

.. rst-class:: numbered-step

Add the cluster description file to the data model definition file and run the ZAP tool
***************************************************************************************

The data model definition file contains all the cluster XML locations and manufacturers list.
To work with the new custom cluster, you need to append it to the list in the existing data model definition file.

You can use the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui` to add the cluster and run the ZAP tool, or :ref:`ug_matter_gs_tools_matter_west_commands_append` to add the cluster only without starting the ZAP tool.
This guide focuses on the :ref:`ug_matter_gs_tools_matter_west_commands_zap_tool_gui`.

1. Run the following command:

   .. code-block::

      west zap-gui -j ./zcl.json --clusters ./MyCluster.xml

   This example command copies the original :file:`<default Matter SDK location>/src/app/zap-templates/zcl/zcl.json` file, adds the :file:`MyCluster.xml` cluster, and saves the new :file:`zcl.json` file in the sample directory.
   The newly generated :file:`zcl.json` file is used as an input to the ZAP tool.

   .. note::
      Execute the command from your application's directory as the ZAP tool searches recursively for the :file:`.zap` files in the current directory.

#. Add an endpoint with the new device type in the ZAP tool.

   .. figure:: images/matter_creating_custom_cluster_new_endpoint.png
      :alt: Endpoint with My new device in ZAP tool

      Endpoint with My new device in ZAP tool

#. Locate the new cluster in the ZAP tool.

   .. figure:: images/matter_creating_custom_cluster_new_cluster.png
      :alt: New custom cluster in ZAP tool

      New custom cluster in ZAP tool

#. Choose whether the cluster should be enabled for the Client and Server sides.

#. Click the gear icon to open the cluster configuration and enable the attributes, commands, and events.

   a. In the :guilabel:`ATTRIBUTES` tab, ensure that you have the required attributes enabled.

      .. figure:: images/matter_creating_custom_cluster_attributes.png
         :alt: Attributes of the new custom cluster in ZAP tool

         Attributes of the new custom cluster in ZAP tool

   #. In the :guilabel:`COMMANDS` tab, ensure that you have the required commands enabled.

      .. figure:: images/matter_creating_custom_cluster_commands.png
         :alt: Commands of the new custom cluster in ZAP tool

         Commands of the new custom cluster in ZAP tool

   #. In the :guilabel:`EVENTS` tab, ensure that you have the required events enabled.

      .. figure:: images/matter_creating_custom_cluster_events.png
         :alt: Events of the new custom cluster in ZAP tool

         Events of the new custom cluster in ZAP tool

#. Save the file and exit.

.. rst-class:: numbered-step

Generate the C++ code that contains the selected clusters
*********************************************************

Run the following command to use the modified ZAP file to generate the C++ code that contains the selected clusters:

   .. code-block::

      west zap-generate --full

After completing these steps, the following changes will be visible within your sample directory:

* The new cluster description file :file:`MyCluster.xml`.
* The updated data model definition file :file:`zcl.json` with the new cluster and relative paths to the Matter data model directory.
* The generated C++ source files for the new cluster.
* The updated :file:`.zap` file with the new cluster configuration and relative path to the :file:`zcl.json` file.

Once the new cluster is added to the Matter application, you can call the ``zap-gui`` command without the additional ``--clusters`` argument.
However, you still need to provide the path to the :file:`zcl.json` file if you created a new one in a location different from  the default one.


.. rst-class:: numbered-step

Align CMake configuration with the new cluster
**********************************************

Generating the :file:`.zap` files with the ``--full`` option creates new source files under :file:`zap-generated/app-common`.
They need to override the default files located in the Matter SDK in the :file:`zzz_generated/app-common` directory.
To override the path, you need to set the ``CHIP_APP_ZAP_DIR`` variable in the :file:`CMakeLists.txt` file, pointing to the parent of the generated :file:`app-common` directory before initializing the Matter Data Model.

As custom clusters are not part of the default Matter SDK, you need to additionally pass a list of all new cluster names in an ``EXTERNAL_CLUSTERS`` argument when calling ``chip_configure_data_model``.

The following code snippet shows how to modify the Matter template :file:`CMakeLists.txt` file with the new cluster:

   .. code-block:: cmake

      project(matter-template)

      # Override zap-generated directory.
      get_filename_component(CHIP_APP_ZAP_DIR ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/zap-generated REALPATH CACHE)

      # Existing code in CMakeList.txt

      chip_configure_data_model(app
          INCLUDE_SERVER
          BYPASS_IDL
          GEN_DIR ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/zap-generated
          ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/template.zap
          EXTERNAL_CLUSTERS "MY_NEW_CLUSTER" # Add EXTERNAL_CLUSTERS flag
      )

      # NORDIC SDK APP END
