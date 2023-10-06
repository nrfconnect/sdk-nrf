.. _ug_matter_last_fabric_removal_delegate:

Matter fabric removal
#####################

.. contents::
   :local:
   :depth: 2

.. matter_last_fabric_removal_description_start

When a Matter device is commissioned by the Matter controller, it then belongs to a specific Matter fabric.
To ensure the interoperability of multiple applications and ecosystems, the device can join the Matter fabrics created by multiple ecosystems using the multi-fabric feature.
As a result, the device can belong to multiple fabrics and can be managed by multiple Matter controllers.

To remove the device from a fabric, a Matter controller sends the leave request to it and then removes all information collected from the device.
The device receives the leave request and removes all information about the fabric it is leaving.

.. matter_last_fabric_removal_description_end

Reaction to the last Matter fabric removal
******************************************

When a device leaves the last Matter fabric it has joined, the network credentials and saved data are no longer usable.
You can define a reaction to the last fabric removal and decide whether the device should use one of the predefined behaviors, or you can implement custom scenarios.

Predefined last fabric removal behaviors
========================================

There are four predefined reactions to the last fabric removal available in the :ref:`matter_samples`.
All behaviors are implemented as a delegation for the Fabric Table module, and the chosen reaction is run as a callback on each fabric removal.

For more information, see the :file:`fabric_table_delegate.h` header file which is located in the Matter samples common directory.

You can choose one of the following reactions to the last fabric removal and instruct the device to:

   * Not react to the last fabric removal and keep the current state of the device.
   * Remove the network credentials, and keep application-specific non-volatile data.
   * Remove the network credentials, keep application-specific non-volatile data, and then reboot.
   * Remove all stored non-volatile data (both network credentials and application-specific data) and reboot.
   * Remove the network credentials, keep application-specific non-volatile data, and start advertising Bluetooth LE Matter service.

To read more about the reactions to the last fabric and learn how to set the related Kconfigs, see the :ref:`ug_matter_device_advanced_kconfigs` page.

Custom last fabric removal delegate
===================================

In some cases, the predefined reactions to the last fabric removal are not enough.
For example, only one of the predefined scenarios includes removing application-specific non-volatile data, but it also requires rebooting the device.
If the predefined reactions do not meet your requirements, you can define a custom reaction to the last fabric removal.

To implement a custom reaction you need to define a new delegation for the Fabric Table module.
The Fabric Table module is defined in the Matter stack and is responsible for managing the Matter fabrics to which the device belongs.
The module stores a list of the delegations and each registered one will be called when the device leaves a Matter fabric.
You can define multiple delegations, but you need to disable the predefined one.

Disabling the predefined reaction
---------------------------------

To disable the predefined reaction to the last fabric removal, set the :kconfig:option:`CONFIG_CHIP_LAST_FABRIC_REMOVED_NONE` Kconfig option to ``y``.

Implementing the custom Fabric Table delegation
-----------------------------------------------

To implement the custom Fabric Table delegation, complete the following points:

   * Include the :file:`app/util/attribute-storage.h` file from the Matter stack core.

      .. code-block:: c

         #include <app/util/attribute-storage.h>

   * Implement a class that inherits ``chip::FabricTable::Delegate`` and contains an overridden ``OnFabricRemoved`` method from the base class.

      .. code-block:: c

         class AppFabricTableDelegate : public chip::FabricTable::Delegate {

            private:

               void OnFabricRemoved(const chip::FabricTable &fabricTable, chip::FabricIndex fabricIndex) {}

         };

   * Implement a reaction on the last fabric removal within the body of the ``OnFabricRemoved`` method.
   * Call the ``chip::Server::GetInstance().GetFabricTable().AddFabricDelegate`` method of the Fabric Table module in the ``AppTask.Init`` method, and provide the newly created class of delegation as an argument.
     The new delegation will be added to the delegation table and allows you to call the overridden ``OnFabricRemoved`` method when the last fabric is going to be removed.

.. note::
   Not all actions implemented in the ``OnFabricRemoved`` can be called directly from the method body, and should be delegated to be done in Zephyr thread of Matter.
   This is because the device needs to confirm the removal of the fabric and send the acknowledgment to the Matter controller.
   To delegate actions to the Zephyr thread of Matter, use the ``chip::DeviceLayer::PlatformMgr().ScheduleWork`` method.

To see an example implementation of the Fabric Table delegation, see the :file:`fabric_table_delegate.h` file which is located in the Matter samples common directory.
