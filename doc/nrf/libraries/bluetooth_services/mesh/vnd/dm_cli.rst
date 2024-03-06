.. _bt_mesh_dm_cli_readme:

Distance Measurement Client
###########################

.. contents::
   :local:
   :depth: 2

The Distance Measurement Client vendor model remotely controls the state of a :ref:`bt_mesh_dm_srv_readme` model.

At creation, each Distance Measurement client instance must be initialized with a memory object that can hold one or more measurement results.

.. note::
   The current implementation is classified with :ref:`experimental <software_maturity>` software maturity.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth® Mesh shell subsystem provides a set of commands to interact with the Distance Measurement Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_DM_CLI`

mesh models dm instance get-all
   Print all instances of the Distance Measurement Client model on the device.

mesh models dm instance set <ElemIdx>
   Select the Distance Measurement Client model instantiated on the specified element ID.
   This instance will be used in message sending.
   If no model instance is selected, the first model instance found on the device will be used by default.

   * ``ElemIdx`` - Element index where the model instance is found.

mesh models dm cfg [<TTL> <Timeout(100ms steps)> <Delay(µs)>]
   If no parameters are provided, get the current configuration state parameters of the server and wait for a response.
   If parameters are provided, set the configuration state parameters of the server and wait for a response.

   * ``TTL`` - Default time to live for sync messages on the target server.
   * ``Timeout`` - Default timeout for distance measurement procedure on the target server in 100 ms per step.
   * ``Delay`` - Default reflector start delay for the server in microseconds.

mesh models dm start <Mode> <Addr> [<ReuseTransaction> [<TTL> <Timeout(100ms steps)> <Delay(µs)>]]
   Start a distance measurement procedure.

   * ``Mode`` - Mode used for ranging.
   * ``Addr`` - Unicast address of the node to measure distance with.
   * ``ReuseTransaction`` - Flag indicating if the previous transaction should be used.
   * ``TTL`` - Time to live for the sync message.
   * ``Timeout`` - Timeout for distance measurement procedure in 100 ms per step.
   * ``Delay`` - Reflector start delay in microseconds.

mesh models dm result-get <EntryCnt>
   Get the last distance measurement results from the server. The returned results will start with the most recent measurement.

   * ``EntryCnt`` - Number of entries the user wish to receive.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/vnd/dm_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/vnd/dm_cli.c`

.. doxygengroup:: bt_mesh_dm_cli
   :project: nrf
   :members:
