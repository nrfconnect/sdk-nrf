.. _bt_mesh_dm_srv_readme:

Distance Measurement Server
###########################

.. contents::
   :local:
   :depth: 2

The Distance Measurement Server vendor model provides capabilities to measure distance between Bluetooth® Mesh devices within radio proximity.
The measurements are conducted through the :ref:`mod_dm` library.

At creation, each Distance Measurement Server instance must be initialized with a memory object that can hold one or more measurement results.
The maximum allowed number of entries in this memory object is restricted to 127 entries.
The Distance Measurement Server populates this memory object whenever a new measurement is conducted.
The results can be queried by a :ref:`bt_mesh_dm_cli_readme`.

.. note::
   The current implementation is classified with :ref:`experimental <software_maturity>` software maturity.

.. figure:: ../../../../../nrf/libraries/bluetooth_services/mesh/vnd/images/bt_mesh_dm_models.svg
   :alt: Sequence diagram for DM procedure

   Distance measurement procedure

A Distance measurement procedure is started by issuing a ``bt_mesh_dm_cli_measurement_start`` message from a Distance Measurement Client (DM CLI).
Within this message, the following needs to be defined by the caller:

* The target address to a server with which the receiving server will perform the measurement.
* The desired measurement mode.
* Parameters to use in the measurement:

  * Time-to-live (TTL) value.
  * Timeout for the measurement procedure.
  * Reflector start delay.

When the Distance Measurement Server 1 (DM SRV 1) receives the start message, it will start a timeout timer with either the default timeout or the timeout defined in the start message.
It will also add a distance measurement request in the reflector role using the mode and the reflector start delay defined in the start message.
If the reflector start delay is not defined and present in the start message, a default delay is used instead.

DM SRV 1 will then issue a synchronization message to the target address enclosed in the received start message.
This target address points to another Distance Measurement Server, DM SRV 2.
The synchronization message uses by default the same mode and the same timeout value as the DM SRV 1 received in the start message.

When the DM SRV 2 receives the synchronization message, it starts its own timeout timer and adds a distance measurement request in the initiator role using the mode defined in the start message.

If the ranging procedure is successful, the result is stored on both DM SRV 1 and DM SRV 2.
In addition, the DM SRV 1 sends the measured result as a response back to the Distance Measurement Client that initiated the procedure.

If the ranging procedure fails, the result is still stored on both servers.
The DM SRV 1 sends the result entry as a response to the client, indicating that something went wrong during the procedure.

.. note::
   Due to size restrictions in the result message format, measurements exceeding 65535 centimeters (UINT16_MAX) will be floored to 65535 centimeters.
   Clients receiving results with values set to 65535 should interpret the result as 65535 centimeters **or higher**.

States
======

None

Extended models
===============

None

Persistent storage
==================

The Distance Measurement Server has following runtime configuration options:

* Default time to live for sync messages.
* Default timeout for distance measurement procedure.
* Default reflector start delay.

If the :kconfig:option:`CONFIG_BT_SETTINGS` option is enabled, the Distance Measurement Server stores its configuration states persistently using a configurable storage delay.
See option :kconfig:option:`CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT`.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/vnd/dm_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/vnd/dm_srv.c`

.. doxygengroup:: bt_mesh_dm_srv
   :project: nrf
   :members:
