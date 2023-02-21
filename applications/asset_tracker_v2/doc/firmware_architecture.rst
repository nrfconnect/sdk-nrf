.. _asset_tracker_v2_internal_modules:

Firmware architecture
#####################

.. contents::
   :local:
   :depth: 2

The Asset Tracker v2 application has a modular structure, where each module has a defined scope of responsibility.
The application makes use of the :ref:`app_event_manager` to distribute events between modules in the system.
The Application Event Manager is used for all the communication between the modules.
A module converts incoming events to messages and processes them in a FIFO manner.
The processing happens either in a dedicated processing thread in the module, or directly in the Application Event Manager callback.

The following figure shows the relationship between the modules and the Application Event Manager.
It also shows the modules with thread and the modules without thread.

.. figure:: /images/asset_tracker_v2_module_hierarchy.svg
    :alt: Relationship between modules and the Application Event Manager

    Relationship between modules and the Application Event Manager

Internal modules
****************

The application comprises of the following modules:

* :ref:`asset_tracker_v2_app_module`
* :ref:`asset_tracker_v2_data_module`
* :ref:`asset_tracker_v2_cloud_module`
* :ref:`asset_tracker_v2_sensor_module`
* :ref:`asset_tracker_v2_location_module`
* :ref:`asset_tracker_v2_ui_module`
* :ref:`asset_tracker_v2_util_module`
* :ref:`asset_tracker_v2_debug_module`
* :ref:`asset_tracker_v2_modem_module`

Each module documentation contains information about its API, dependencies, states, and state transitions.

The application has two types of modules:

* Module with dedicated thread
* Module without thread

Every module has an Application Event Manager handler function, which subscribes to one or more event types.
When an event is sent from a module, all subscribers receive that event in the respective handler, and acts on the event in the following ways:

1. The event is converted into a message
#. The event is either processed directly or queued.

Modules can also receive events from other sources such as drivers and libraries.
For instance, the cloud module also receives events from the configured cloud backend.
The event handler converts the events to messages.
The messages are then queued in the case of the cloud module or processed directly in the case of modules that do not have a processing thread.

.. figure:: /images/asset_tracker_v2_module_structure.svg
    :alt: Event handling in modules

    Event handling in modules

For more information about each module and its configuration, see the respective :ref:`module documentation <asset_tracker_v2_subpages>`.

Thread usage
************

In addition to system threads, some modules have dedicated threads to process messages.
Some modules receive messages that might potentially take an extended amount of time to process.
Such messages are not suitable to be processed directly in the event handler callbacks that run on the system queue.
Therefore, these modules have a dedicated thread to process the messages.

Application-specific threads:

* Main thread (app module)
* Data management module
* Cloud module
* Sensor module
* Modem module

Modules that do not have dedicated threads process events in the context of system work queue in the Application Event Manager callback.
Therefore, their workloads must be light and non-blocking.

All module threads have the following identical properties by default:

* Thread is initialized at boot.
* Thread is preemptive.
* Priority is set to the lowest application priority in the system, which is done by setting :kconfig:option:`CONFIG_NUM_PREEMPT_PRIORITIES` to ``1``.
* Thread is started instantly after it is initialized in the boot sequence.

Following is the basic structure for all the threads:

.. code-block:: c

   static void module_thread_fn(void)
   {
           struct module_specific msg;

           self.thread_id = k_current_get();
           module_start(&self);

           /* Module specific setup */

           state_set(STATE_DISCONNECTED);

           while (true) {
                   module_get_next_msg(&self, &msg);

                   switch (state) {
                   case STATE_DISCONNECTED:
                           on_state_disconnected(&msg);
                           break;
                   case STATE_CONNECTED:
                           on_state_connected(&msg);
                           break;
                   default:
                           LOG_WRN("Unknown state");
                           break;
                   }

                   on_all_states(&msg);
           }
   }

.. _memory_allocation:

Memory allocation
*****************

Mostly, the modules use statically allocated memory.
Following are some features that rely on dynamically allocated memory, using the :ref:`Zephyr heap memory pool implementation <zephyr:heap_v2>`:

* Application Event Manager events
* Encoding of the data that will be sent to cloud

You can configure the heap memory by using the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`.
The data management module that encodes data destined for cloud is the biggest consumer of heap memory.
Therefore, when adjusting buffer sizes in the data management module, you must also adjust the heap accordingly.
This avoids the problem of running out of heap memory in worst-case scenarios.
