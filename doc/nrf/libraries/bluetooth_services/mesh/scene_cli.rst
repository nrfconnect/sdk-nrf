.. _bt_mesh_scene_cli_readme:

Scene Client
############

.. contents::
   :local:
   :depth: 2

The Scene Client model remotely controls the state of a :ref:`bt_mesh_scene_srv_readme` model.

Unlike the Scene Server model, the Scene Client only creates a single model instance in the mesh composition data.
The Scene Client can send messages to both the Scene Server and the Scene Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Scene Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_SCENE_CLI`

mesh models scene instance get-all
	Print all instances of the Scene Client model on the device.


mesh models scene instance set <ElemIdx>
	Select the Scene Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models scene get
	Get the current state of the Scene Server.


mesh models scene register-get
	Get the full scene register of the Scene Server.


mesh models scene store <Scene>
	Store the current state as a scene and wait for a response.

	* ``Scene`` - Scene number to store.


mesh models scene store-unack <Scene>
	Store the current state as a scene without requesting a response.

	* ``Scene`` - Scene number to store.


mesh models scene delete <Scene>
	Delete the given scene and wait for a response.

	* ``Scene`` - Scene number to delete.


mesh models scene delete-unack <Scene>
	Delete the given scene without requesting a response.

	* ``Scene`` - Scene number to delete.


mesh models scene recall <Scene> [TransTime(ms) [Delay(ms)]]
	Recall the given scene and wait for a response.

	* ``Scene`` - Scene number to recall.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models scene recall-unack <Scene> [TransTime(ms) [Delay(ms)]]
	Recall the given scene without requesting a response.

	* ``Scene`` - Scene number to recall.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/scene_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/scene_cli.c`

.. doxygengroup:: bt_mesh_scene_cli
   :project: nrf
   :members:
