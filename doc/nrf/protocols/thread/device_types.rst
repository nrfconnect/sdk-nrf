.. _thread_device_types:

Thread device types
###################

.. contents::
   :local:
   :depth: 2

Thread devices can be either Full Thread Devices (FTD) or Minimal Thread Devices (MTD).
The devices are divided based on their ability to exist in a network without a parent, determined by whether the device maintains a routing table (doesn't need a parent) or not (needs a parent).
FTDs are the devices that maintain a routing table, while MTDs do not, forwarding all their messages to a parent instead.

Another type of Thread device classification is based on the device's ability to route messages:

* Routers can route messages, and they can function as parents to other Thread devices.
* End Devices cannot route messages, and they cannot function as parents.

Based on this classification, FTDs can be either Routers or End Devices, while MTDs can only be End Devices.

For additional information about Thread device types, see the documentation for `Device Types on OpenThread portal`_.

.. _thread_types_ftd:

Full Thread Devices
*******************

In order to maintain the routing table, FTDs keep their radio on at all times.
An important difference between FTDs and MTDs is that FTDs subscribe to special "all-routers" multicast addresses (``ff02::2`` and ``ff03::2``, see `IPv6 multicast addressing`_).
Because of these reasons, they typically consume more power than MTDs.

Full Thread Devices can be further divided into three categories:

* Router
* Full End Device (FED)
* Router Eligible End Device (REED)

.. list-table:: Full Thread Device categories
   :widths: 15 10 10 15
   :header-rows: 1

   * -
     - Router
     - Full End Device
     - Router Eligible End Device
   * - Maintains a routing table
     - Yes
     - Yes
     - Yes
   * - Routes messages
     - Yes
     - No
     - When working as a Router
   * - Can extend the network
     - Yes
     - No
     - When working as a Router

Routers can be regarded as the backbone of a Thread network.
They maintain the routing table and forward messages to other devices.
Routers, unlike End Devices, can also be used to extend the network range and a Router is required for a Minimal End Device to join the network.
The maximum number of Routers in a single Thread network is 32.

FEDs maintain a routing table, but they can't route messages.
This means they don't require a parent (Router) to function, but they cannot become parents themselves.

REEDs maintain a routing table and, unlike FEDs, they can become Routers if needed.
If a REED is the only device in range of an End Device trying to join the network, it will promote itself to a Router (see `Joining an existing Thread network`_ in the OpenThread documentation for more information).
Conversely, when a Router has no children it can downgrade itself to a REED.

.. _thread_types_mtd:

Minimal Thread Devices
**********************

Minimal Thread Devices (MTDs) are devices that do not maintain a routing table and are typically low-power devices that are not always on.
They can only be End Devices, and they always need a parent to function.
They forward all their messages to their parent.

Minimal Thread Devices can be further divided into three categories:

* Minimal End Device (MED)
* Sleepy End Device (SED)
* Synchronized Sleepy End Device (SSED)

.. list-table:: Minimal Thread Device categories
   :widths: 15 10 10 15
   :header-rows: 1

   * -
     - Minimal End Device
     - Sleepy End Device
     - Synchronized Sleepy End Device
   * - Maintains a routing table
     - No
     - No
     - No
   * - Radio on
     - Always
     - Periodically
     - Periodically
   * - New messages
     - Parent immediately forwards the messages
     - The SED polls for new messages when it wakes up
     - Parent forwards the messages during designated transmission window
   * - No new messages
     - No data transmission
     - Parent indicates no pending messages
     - No data transmission

MEDs are the most basic MTDs, and their radio is always on.
They don't keep a routing table, but otherwise operate like :ref:`FEDs <thread_types_ftd>`.

SEDs try to limit their power consumption by sleeping most of the time, waking up periodically to poll for messages from their parent.
After waking up, they send a data request to their parent.
If the parent has any pending messages, it will send them to the SED.
Otherwise, the parent will send a response indicating no pending messages.

SSEDs operate similarly to Sleepy End Devices, but they are synchronized with their parent.
They wake up at the same time as their parent, eliminating the need for polling for messages.
If the parent has messages for the SSED, it sends them during the designated transmission window.
The SSED allows the transmission to finish if radio activity is detected during the transmission window.
Conversely, if there is no radio activity during the specified duration of transmission window, this indicates that the parent has no messages for the SSED and the SSED returns to sleep.
The SSED synchronization results in lower power consumption compared to an SED, primarily because the SSED doesn't need to poll for messages, keeping transmission windows short.

For more information about SSED activity, see the :ref:`thread_sed_ssed` page.

.. _thread_types_configuring:

Type configuration
******************

See :ref:`thread_ug_device_type` for information about how to configure Thread device type.
