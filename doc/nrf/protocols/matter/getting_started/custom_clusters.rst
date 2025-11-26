.. _ug_matter_creating_accessory_vendor_cluster:
.. _ug_matter_creating_custom_cluster:

Creating manufacturer-specific clusters in Matter application
#############################################################

.. contents::
   :local:
   :depth: 2

This guide describes how you can create a manufacturer-specific cluster for the :ref:`matter_template_sample` sample.
The :ref:`matter_manufacturer_specific_sample` sample already contains a custom ``NordicDevkit`` cluster that you can use as a reference.

Overview
********

A manufacturer-specific cluster is a cluster that is not defined in the Matter Device Type Library Specification.
The cluster description is written in XML format and is used to generate the C++ source files that provide the cluster implementation.
You can add the cluster to the Matter data model definition file and use it in the :ref:`ug_matter_gs_tools_zap` to generate the source files.

Requirements
************

To take advantage of this guide, you need to be familiar with the :ref:`ug_matter_architecture` and :ref:`configuration <ug_matter_configuring>`, and have built and tested at least one of the available :ref:`matter_samples`.

.. rst-class:: numbered-step

Copy Matter template sample
***************************

Use the :ref:`matter_template_sample` sample as the base for building a manufacturer-specific device as follows:

1. Make sure that you meet the requirements for building the sample.
#. Copy the contents of the :file:`samples/matter/template` directory to a new directory meant for your custom application.
   For example, :file:`samples/matter/sensor`.
#. Build and test the sample as described on its documentation page.

.. rst-class:: numbered-step

.. _ug_matter_gs_custom_clusters_create_xml_file:

Create a new cluster description file in XML format
***************************************************

You can create a new cluster description file in the following ways:

* Using the `Matter Cluster Editor app`_.
  The related tab provides the steps to create a new cluster description file.

* Manually by writing an XML file.
  The related tab explains each element of the XML file to help you create the file manually.
  You can also use the example of the XML file provided at the end of this section.

.. tabs::

   .. tab:: Using the Matter Cluster Editor app

      .. note::
         |matter_cluster_editor_preview|

      Before using the tool, you need to download and install it.
      See the `Matter Cluster Editor app`_ documentation for installation instructions.

      Once you have the tool installed, you can create a new cluster description file.
      Complete the following steps:

      1. Edit the :guilabel:`CLUSTER` tab contents.

         a. Open the :guilabel:`CLUSTER` tab.
         #. Fill in the domain, name, code, define, and description of the cluster as follows:

            .. figure:: images/matter_creating_custom_cluster_cluster_page.png
               :alt: Cluster tab

               CLUSTER tab

      #. Add a new command in the :guilabel:`COMMANDS` tab.

         a. Open the :guilabel:`COMMANDS` tab.
         #. Click :guilabel:`Add command` to open edit box.
         #. In the edit box, set the following values:

            * **Name** as ``MyCommand``
            * **Code** as ``0xFFF10000``
            * **Source** as ``client``
            * **Response** as ``MyCommandResponse``
            * **Description** as ``Command that takes two uint8 arguments and returns their sum``

         #. Click :guilabel:`Arguments`.
         #. In the new edit box, click the plus icon to create a new argument.
         #. Fill in :guilabel:`Name` as ``arg1``, :guilabel:`Type` as ``int8u``.
         #. Click the plus icon again to create second argument.
         #. Fill in :guilabel:`Name` as ``arg2``, :guilabel:`Type` as ``int8u``.

            The following figure shows the filled in edit box dialog with two arguments added:

            .. figure:: images/matter_creating_custom_cluster_arguments_page.png
               :alt: Arguments tab

               Arguments tab

         #. Click :guilabel:`Save` to save the arguments.

            The following figure shows the filled in edit box dialog with the new command added:

            .. figure:: images/matter_creating_custom_cluster_commands_page.png
               :alt: Commands tab

               Commands tab

         #. Click :guilabel:`Save` to save the command.

      #. Add a new argument in the :guilabel:`ATTRIBUTES` tab.

         a. Open the :guilabel:`ATTRIBUTES` tab.
         #. Click :guilabel:`Add attribute` to open edit box dialog.
         #. Set the following values:

            * **Name** as ``MyAttribute``
            * **Side** as ``server``
            * **Code** as ``0xFFF10000``
            * **Define** as ``MY_ATTRIBUTE``
            * **Type** as ``boolean``
            * **Writable** as ``true``

            The following figure shows the filled in edit box dialog with the new attribute added:

            .. figure:: images/matter_creating_custom_cluster_attributes_page.png
               :alt: Attributes tab

               Attributes tab

         #. Click :guilabel:`Save` to save the attribute.

      #. Add a new event in the :guilabel:`EVENTS` tab.

         a. Open the :guilabel:`EVENTS` tab.
         #. Click :guilabel:`Add event` to open edit box dialog.
         #. In the edit box, set the following values:

            * **Code** as ``0xFFF10000``
            * **Name** as ``MyEvent``
            * **Side** as ``server``
            * **Priority** as ``info``
            * **Description** as ``Event that is generated by the server``

         #. Click :guilabel:`Fields`.
         #. In the new edit box, click the plus icon to add a new field.
         #. Fill in the following values:

            * **Field Id** as ``0x1``
            * **Name** as ``arg1``
            * **Type** as ``int8u``

            The following figure shows the filled in edit box dialog with the new field added:

            .. figure:: images/matter_creating_custom_cluster_fields_page.png
               :alt: Fields tab

               Fields tab

         #. Click :guilabel:`Save` to save the field.

            The following figure shows the filled in edit box dialog with the new event added:

            .. figure:: images/matter_creating_custom_cluster_event_page.png
               :alt: Event page

               Events tab

         #. Click :guilabel:`Save` to save the event.

      #. Add a new structure in the :guilabel:`STRUCTURES` tab.

         a. Open the :guilabel:`STRUCTURES` tab.
         #. Click :guilabel:`Add structure` to open edit box dialog.
         #. In the edit box, set the following values:

            * **Name** as ``MyStruct``
            * **Is Fabric Scoped** as ``true``

         #. Click :guilabel:`Items`.
         #. In the new edit box, click the plus icon to create a new item.
         #. Fill in the following values:

            * **Field Id** as ``0x1``
            * **Name** as ``value1``
            * **Type** as ``int8u``
            * **Is Fabric Sensitive** as ``true``

            The following figure shows the filled in edit box dialog with the new item added:

            .. figure:: images/matter_creating_custom_cluster_structure_items_page.png
               :alt: Structure items tab

               Structure items tab

         #. Click :guilabel:`Save` to save the item.
         #. Click :guilabel:`Assigned clusters` to open edit box dialog.
         #. In the new edit box, click the plus icon to create a new cluster assignment.
         #. Fill in ``Code`` with the value of the cluster code defined in first step as ``0xFFF1FC01``.

            The following figure shows the filled in edit box dialog with the new cluster added:

            .. figure:: images/matter_creating_custom_cluster_assigned_clusters_page.png
               :alt: Assigned clusters tab

               Assigned clusters tab

         #. Click :guilabel:`Save` to save the cluster.

            The following figure shows the filled in edit box dialog with the new structure added:

            .. figure:: images/matter_creating_custom_cluster_structures_page.png
               :alt: Structures tab

               Structures tab

         #. Click :guilabel:`Save` to save the structure.

      #. Add a new enum in the :guilabel:`ENUMS` tab.

         a. Open the :guilabel:`ENUMS` tab.
         #. Click :guilabel:`Add enum` to open edit box dialog.
         #. Set the following values:

            * **Name** as ``MyEnum``
            * **Type** as ``int8u``

         #. Click :guilabel:`Items`.
         #. In the new edit box, click the plus icon to create a new item.
         #. Fill in the following values:

            * **Name** as ``EnumValue1``
            * **Value** as ``0``

         #. Click the plus icon to create a new item.
         #. Fill in the following values:

            * **Name** as ``EnumValue2``
            * **Value** as ``1``

            The following figure shows the filled in edit box dialog with the new items added:

            .. figure:: images/matter_creating_custom_cluster_items_enum_page.png
               :alt: Items tab

               Items tab

         #. Click :guilabel:`Save` to save the item.
         #. Click :guilabel:`Assigned clusters` to open edit box dialog.
         #. In the new edit box, click the plus icon to create a new cluster assignment.
         #. Fill in ``Code`` with the value of the cluster code defined in first step as ``0xFFF1FC01``.

            The following figure shows the filled in edit box dialog with the new cluster assignment added:

            .. figure:: images/matter_creating_custom_cluster_assigned_clusters_page.png
               :alt: Assigned clusters tab

               Assigned clusters tab

         #. Click :guilabel:`Save` to save the cluster.

            The following figure shows the filled in edit box dialog with the new enum added:

            .. figure:: images/matter_creating_custom_cluster_enums_page.png
               :alt: Enums tab

               Enums tab

         #. Click :guilabel:`Save` to save the enum.

      #. Add a new device type in the :guilabel:`DEVICE TYPE` tab.

         a. Open the :guilabel:`DEVICE TYPE` tab.
         #. Fill the fields as follows:

            .. figure:: images/matter_creating_custom_cluster_device_type_page.png
               :alt: Device type tab

               Device type tab

         #. Click :guilabel:`Add cluster assignment to device type` to open edit box dialog.
         #. Fill the Cluster fields as follows:

            .. figure:: images/matter_creating_custom_cluster_device_type_cluster_assignment_page.png
               :alt: Device type cluster assignment tab

               Device type cluster assignment tab

         #. Click :guilabel:`Save` to save the cluster assignment.

      #. Click the :guilabel:`Save cluster to file` button to save the cluster description file to the sample directory and name it as ``MyCluster.xml``.

   .. tab:: Manually create an XML file

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

               * ``side`` - Specifies whether the event originates from the client or server.
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
                  <description>The MyNewCluster cluster showcases cluster manufacturer extensions</description>
                  <attribute side="server" code="0xFFF10000" define="MY_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">MyAttribute</attribute>
                  <command source="client" code="0xFFF10000" name="MyCommand" response="MyCommandResponse" optional="false">
                     <description>Command that takes two uint8 arguments and returns their sum.</description>
                     <arg name="arg1" type="int8u"/>
                     <arg name="arg2" type="int8u"/>
                  </command>
                  <event side="server" code="0xFFF10000" name="MyEvent" priority="info" optional="false">
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

               * ``side`` - Specifies whether the event originates from the client or server.
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
                  <command source="server" code="0x01" name="ExtendedCommandResponse" optional="false" disableDefaultResponse="true">
                     <description>Response to ExtendedCommand.</description>
                     <arg name="arg1" type="int8u"/>
                  </command>
                  <event side="server" code="0x04" name="ExtendedEvent" priority="info" optional="false">
                     <description>Event that is generated by the server.</description>
                     <arg name="arg1" type="int8u"/>
                  </event>
               </clusterExtension>

         .. tab:: ``<enum>``

            ``<enum>`` elements define the enumerated types that can be used in the cluster and consist of the following attributes and child elements:

            * ``name`` - The unique name of the enumerated type.
            * ``<cluster code>`` - The cluster codes that the enumerated type is associated with.
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
            * ``<cluster code>`` - The cluster codes that the structure is associated with.
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
            * ``<profileId>`` - The profile ID reflects the current version of the Matter specification where the major version is the most significant byte (0x01), the minor version is the second most significant byte (0x04), and the dot version is the third most significant byte (0x02).

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
                  <profileId editable="false">0x0104</profileId>
                  <deviceId editable="false">0xfff10001</deviceId>
                  <class>Simple</class>
                  <scope>Endpoint</scope>
                  <clusters lockOthers="true">
                  <include cluster="MyNewCluster" client="true" server="true" clientLocked="false" serverLocked="false"/>
                     <requireAttribute>MY_ATTRIBUTE</requireAttribute>
                     <requireCommand>MyCommand</requireCommand>
                  </clusters>
               </deviceType>

      .. note::
         The descriptions of the elements show only the basic functionality.
         To see the full list of available elements for each part of the XML file, refer to the Matter Specification.

For an example, you can use the following template for the :file:`MyCluster.xml` file:

.. code-block:: xml

   <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
   <configurator>
   <cluster>
      <domain>General</domain>
      <name>MyNewCluster</name>
      <code>0xfff1fc01</code>
      <define>MY_NEW_CLUSTER</define>
      <description>The MyNewCluster cluster showcases cluster manufacturer extensions</description>
      <attribute side="server" code="0xfff10000" define="MY_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false" name="MyAttribute">
      </attribute>
      <command source="client" code="0xfff10000" name="MyCommand" optional="false">
         <description>Command that takes two uint8 arguments and returns their sum.</description>
         <arg name="arg1" type="int8u"/>
         <arg name="arg2" type="int8u"/>
      </command>
      <event side="server" code="0xfff10000" name="MyEvent" priority="info" optional="false">
         <description>Event that is generated by the server.</description>
         <arg name="arg1" type="int8u"/>
      </event>
   </cluster>
   <enum name="MyNewEnum" type="int8u">
      <cluster code="0xfff1fc01"/>
      <item name="EnumValue1" value="0"/>
      <item name="EnumValue2" value="1"/>
   </enum>
   <struct name="MyStruct" isFabricScoped="true">
      <cluster code="0xfff1fc01"/>
      <item fieldId="1" name="Data" type="octet_string" length="128" isFabricSensitive="true"/>
   </struct>
   <deviceType>
      <name>my-new-device</name>
      <domain>CHIP</domain>
      <typeName>My new device</typeName>
      <profileId editable="false">0x104</profileId>
      <deviceId editable="false">0xfff10001</deviceId>
      <class>Simple</class>
      <scope>Endpoint</scope>
      <clusters lockOthers="true">
         <include cluster="MyNewCluster" client="true" server="true" clientLocked="false" serverLocked="false"/>
      </clusters>
   </deviceType>
   <clusterExtension code="0x28">
      <attribute side="server" code="0x17" define="EXTENDED_ATTRIBUTE" type="boolean" writable="true" default="false" optional="false">ExtendedAttribute</attribute>
      <command source="client" code="0x0" name="ExtendedCommand" response="ExtendedCommandResponse" optional="false">
         <description>Command that takes two uint8 arguments and returns their sum.</description>
         <arg name="arg1" type="int8u"/>
         <arg name="arg2" type="int8u"/>
      </command>
      <command source="server" code="0x1" name="ExtendedCommandResponse" optional="false" disableDefaultResponse="true">
         <description>Response to ExtendedCommand.</description>
         <arg name="arg1" type="int8u"/>
      </command>
      <event side="server" code="0x4" name="ExtendedEvent" priority="info" optional="false">
         <description>Event that is generated by the server.</description>
         <arg name="arg1" type="int8u"/>
      </event>
   </clusterExtension>
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

      west zap-gui -j src/default_zap/zcl.json --clusters ./MyCluster.xml

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

   a. In the :guilabel:`Attributes` tab, ensure that you have the required attributes enabled.

      .. figure:: images/matter_creating_custom_cluster_attributes.png
         :alt: Attributes of the new custom cluster in ZAP tool

         Attributes of the new custom cluster in ZAP tool

   #. In the :guilabel:`Commands` tab, ensure that you have the required commands enabled.

      .. figure:: images/matter_creating_custom_cluster_commands.png
         :alt: Commands of the new custom cluster in ZAP tool

         Commands of the new custom cluster in ZAP tool

   #. In the :guilabel:`Events` tab, ensure that you have the required events enabled.

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

Add the ``-j`` or ``--zcl-json`` argument to the command to specify the path to the :file:`zcl.json` file if the file is not stored in the ``sample_directory/src/default_zap/`` subdirectory.

For example:

   .. code-block::

      west zap-generate --full -j ./zcl.json

.. important::

   In the |NCS| versions older than 3.2.0, the :file:`zcl.json` had to be stored in the ``sample_directory/src/default_zap/`` subdirectory.

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

As custom clusters are not part of the default Matter SDK, you need to additionally pass a list of all new cluster names in an ``EXTERNAL_CLUSTERS`` argument when calling ``ncs_configure_data_model``.

The following code snippet shows how to modify the Matter template :file:`CMakeLists.txt` file with the new cluster:

   .. code-block:: cmake

      project(matter-template)

      # Override zap-generated directory.
      include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)
      ncs_get_zap_parent_dir(ZAP_PARENT_DIR)

      get_filename_component(CHIP_APP_ZAP_DIR ${ZAP_PARENT_DIR}/zap-generated REALPATH CACHE)

      # Existing code in CMakeList.txt

      ncs_configure_data_model(
         EXTERNAL_CLUSTERS "MY_NEW_CLUSTER" # Add EXTERNAL_CLUSTERS flag
      )

      # NORDIC SDK APP END

If you want to add more than one cluster custom cluster, you need to add all of them to the ``EXTERNAL_CLUSTERS`` argument.

For example:

   .. code-block:: cmake

      ncs_configure_data_model(
         EXTERNAL_CLUSTERS "MY_NEW_CLUSTER" "MY_NEW_CLUSTER_2"
      )

.. rst-class:: numbered-step

Implement all the required commands in the application code
***********************************************************

You must implement the newly defined commands as a dedicated function in the application code to reflect the cluster functionality.
Name the function by combining the ``emberAf`` prefix, cluster name, command name, and ``Callback`` suffix.
The function must return a boolean value, and it takes the following parameters:

* ``CommandHandler *commandObj`` - The command handler.
* ``const ConcreteCommandPath &commandPath`` - The command path.
* ``const <command name>::DecodableType &commandData`` - The command arguments, where ``<command name>`` is the name of the command.

For example, if you define the following command in the :file:`MyCluster.xml` file:

.. code-block:: xml

   <command source="client" code="0xFFF10000" name="MyCommand" optional="false">
      <description>Command that takes two uint8 arguments and returns their sum.</description>
      <arg name="arg1" type="int8u"/>
      <arg name="arg2" type="int8u"/>
   </command>

Then, you need to implement the following command in the application code:

.. code-block:: c

   #include <app-common/zap-generated/callback.h>

   bool emberAfMyNewClusterClusterMyCommandCallback(chip::app::CommandHandler *commandObj, const chip::app::ConcreteCommandPath &commandPath,
                                                    const chip::app::Clusters::MyNewCluster::Commands::MyCommand::DecodableType &commandData)
   {
      // TODO: Implement the command.
   }

   void MatterMyNewClusterPluginServerInitCallback()
   {
      // TODO: Implement the plugin server init callback.
   }

The same applies to the extended commands.

.. note::

   Before the |NCS| v3.2.0, the extended commands callback were handled by the ``emberAf...`` functions.

   For example, if you want to extend the ``BasicInformation`` cluster with the ``ExtendedCommand`` command, you need to implement it in the application code as follows:

   .. code-block:: c

      #include <app-common/zap-generated/callback.h>

      bool emberAfBasicInformationClusterBasicInformationExtendedCommandCallback(chip::app::CommandHandler *commandObj, const chip::app::ConcreteCommandPath &commandPath,
                                                                                 const chip::app::Clusters::BasicInformation::Commands::ExtendedCommand::DecodableType &commandData)
      {
         // TODO: Implement the command.
      }

Synchronizing the ZAP files with the new Matter SDK
***************************************************

If you want to update the Matter SDK revision in your project and you have custom clusters or device types in your project, you need to call the :ref:`ug_matter_gs_tools_matter_west_commands_sync` with the additional ``-j`` and ``--clusters`` arguments.
This command updates the :file:`.zap` file with all required changes from the new Matter SDK Device Type Library Specification and the :file:`zcl.json` file with the new cluster and relative paths to the Matter data model directory.

This is needed especially when you notice an issue with opening the ZAP tool GUI.
