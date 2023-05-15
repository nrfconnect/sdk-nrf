.. _thread_device_types:

Thread device types
###################

.. contents::
   :local:
   :depth: 2

Thread devices can be either Full Thread Devices (FTD) or Minimal Thread Devices (MTD).
Generally speaking, the devices are classified based on the following criteria:

* What role they have in a Thread network - whether they are router-capable (Router, Leader) or can be just an End Device.
* What is the communication scheme - whether the device keeps its radio enabled all the time or sporadically (allowing the device to sleep and save power).
* How they route messages - whether it maintains a routing table (can route messages) or not (does not route messages).
* Which Thread management functionalities they implement - whether they perform them by itself (for example, address resolution or address registration) or rely on the parent.

The role and routing criteria can be used to classify the Thread devices as follows:

* Routers can route messages, and they can function as parents to other Thread devices.
* End Devices cannot route messages, and they cannot function as parents.

Based on all these criteria, the Thread devices can be classified as follows:

.. list-table:: Thread device classification
   :header-rows: 1

   * -
     - Role
     - Communication scheme
     - Message routing method
     - Thread management functionalities
   * - Full Thread Devices (FTDs)
     - Routers (Leaders) or End Devices
     - Keep their radios enabled all the time.
     - Maintain a routing table.
     - Routers perform them by itself. End Devices classified as FTDs have some management functionalities.
   * - Minimal Thread Devices (MTDs)
     - End Devices
     - May not have their radio enabled at all times.
     - Do not maintain a routing table and forward all their messages to the parent.
     - Rely on the parent to perform them.

For additional information about Thread device types, see the documentation for `Device Types in the OpenThread portal`_.

.. _thread_types_ftd:

Full Thread Devices
*******************

FTDs keep their radio enabled at all times to be ready to receive messages asynchronously.
They typically consume more power than MTDs and are mains-powered, which allows them to have the following functionalities that MTDs do not have, among others:

* Ability to resolve IPv6 addresses
* Handling address registration
* Maintaining routing table
* Maintaining links with neighbors

Another important difference between FTDs and MTDs is that FTDs subscribe to special "all-routers" multicast addresses (``ff02::2`` and ``ff03::2``, see `IPv6 multicast addressing`_).

Full Thread Devices can be further divided into the following roles, which depend on the topology of the network:

* Router
* Full End Device (FED) or Router Eligible End Device (REED)

.. list-table:: Full Thread Device network role categories
   :header-rows: 1

   * -
     - Router
     - Full End Device (FED) or Router Eligible End Device (REED)
   * - Maintains a routing table
     - Yes
     - Yes
   * - Routes messages
     - Yes
     - No
   * - Can extend the network
     - Yes
     - No

Routers can be regarded as the backbone of a Thread network.
They maintain the routing table and forward messages to other devices.
Routers, unlike End Devices, can also be used to extend the network range and a Router is required for a Minimal End Device to join the network.
The maximum number of active Routers in a single Thread network is 32.

FEDs are End Devices that maintain a routing table, but they can't route messages.
FEDs are End Devices with FTDs capabilities to perform certain Thread management functionalities on behalf of their parent (Router), such as address resolution, effectively reducing the parent's computation power.
They are child devices that require a parent to function, but they cannot become parents themselves.

REEDs are End Devices that maintain a routing table and can be promoted to be Routers if needed.
If a REED is the only device in range of an End Device trying to join the network, it will proactively request promotion to the Router role (see `Joining an existing Thread network`_ in the OpenThread documentation for more information).

Depending on the network topology conditions, a Router may downgrade itself to a REED role, for example when it does not have any children and there is already a sufficient number of Routers in the network.

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
   :header-rows: 1

   * -
     - Minimal End Device
     - Sleepy End Device
     - Synchronized Sleepy End Device
   * - Maintains a routing table
     - No
     - No
     - No
   * - Radio
     - Always enabled
     - Periodically enabled
     - Periodically enabled
   * - New messages
     - Parent immediately forwards the messages
     - The SED polls for new messages when it wakes up
     - Parent forwards the messages during designated transmission window or when SSED explicitly polls data
   * - No new messages
     - No data transmission
     - Parent indicates no pending messages
     - No data transmission*

MEDs are the most basic MTDs, and their radio is always enabled.
They don't keep a routing table and have a :ref:`more limited range of functionalities than FEDs <thread_types_ftd>`.

SEDs try to limit their power consumption by sleeping most of the time, waking up periodically to poll for messages from their parent.
After waking up, they send a data request to their parent.
If the parent has any pending messages, it will send them to the SED (which can happen at any time, since Routers have their radio enabled all the time.)
Otherwise, the parent will send a response indicating no pending messages.

SSEDs operate similarly to Sleepy End Devices, but they are synchronized with their parent.
They wake up at designated transmission windows agreed with their their parent, which eliminates the need for polling for messages.
If the parent has messages for the SSED, it sends them during the designated transmission window.
The SSED allows the transmission to finish if radio activity is detected during the transmission window.

.. note::
    If the length of the message exceeds the length of the transmission window, the first frame is received in the designated transmission window, but the rest is transmitted using regular Data Polls, exactly like SED.

Conversely, if there is no radio activity during the specified duration of transmission window, this indicates that the parent has no messages for the SSED and the SSED returns to sleep.
The SSED synchronization results in lower power consumption compared to an SED in some scenarios, primarily because the SSED doesn't need to poll for messages, keeping transmission windows short.

For more information about SSED activity, see the :ref:`thread_sed_ssed` page.

.. _thread_types_configuring:

Type configuration
******************

See :ref:`thread_ug_device_type` for information about how to configure Thread device type.
