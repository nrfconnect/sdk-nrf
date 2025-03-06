.. ug_matter_creating_custom_cluster:

Custom clusters
###############

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
