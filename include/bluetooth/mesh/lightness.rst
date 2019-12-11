.. _bt_mesh_lightness_readme:

Light Lightness models
######################

The Light Lightness models allow remote control and configuration of dimmable lights on a mesh device.

The Light Lightness models can represent light in the following ways:

- *Actual*: Lightness is represented on a perceptually uniform lightness scale.
- *Linear*: Lightness is represented on a linear scale.

The application can select whether to use the Actual or Linear representation.
To do so, use :option:`CONFIG_BT_MESH_LIGHTNESS_ACTUAL` or :option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` configuration options, respectively, in the API at compile time.
By default, the Actual representation is used.
Internally, the models will always support both representations, so nodes with different representations can be be used interchangeably.

.. note::
    The relationship between the *Actual* and the *Linear* representations is the following:

    *Light (Linear) = (Light (Actual))*:sup:`2`

    Bindings with other states are always made to the *Actual* representation.

The following Light Lightness models are available:

- :ref:`bt_mesh_lightness_srv_readme`
- :ref:`bt_mesh_lightness_cli_readme`

.. _bt_mesh_lightness_srv_readme:

Light Lightness Server
======================

The Light Lightness Server represents a single light on a mesh device.
It should be instantiated in the light fixture node.

The Light Lightness Server adds the following new model instances in the composition data, in addition to the extended :ref:`bt_mesh_lvl_srv_readme` and :ref:`bt_mesh_ponoff_srv_readme` models:

- Light Lightness Server
- Light Lightness Setup Server

These model instances share the states of Light Lightness Server, but accept different messages.
This allows fine-grained control of the access rights for the Light Lightness states, as these model instances can be bound to different application keys.

The Light Lightness Server only provides write access to the Light state for the user, in addition to read access to all the meta states.

The Light Lightness Setup Server provides write access to the Default Light and Light Range meta states.
This allows the configurator devices to set up the range and the default value for the Light state.

States
******

**Light**: ``u16_t``
    The Light state represents the emitted light level of an element, and ranges from ``0`` to ``65535``.
    The Light state is bound to the Generic Level State of the extended :ref:`bt_mesh_lvl_srv_readme`:

    ::

      Light (Actual) = Generic Level + 32768

    The Generic OnOff state of :ref:`bt_mesh_onoff_srv_readme` (extended through the :ref:`bt_mesh_ponoff_srv_readme`) is derived from the Light state:

    ::

      Generic OnOff = (Light > 0)

    Conversely, if the Generic OnOff state is changed to ``Off``, the Light state is set to ``0``.
    If the Generic OnOff state is changed to ``On`` and the Default Light state is set, the Light state is set to the value of the Default Light state.
    If the Generic OnOff state is changed to ``On`` and the Default Light state is not set, the Light state is set to the last known non-zero value.

    The Light state power up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    - :cpp:enumerator:`BT_MESH_ON_POWER_UP_OFF <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_OFF>`:
      The Light state is set to ``0`` on power up.
    - :cpp:enumerator:`BT_MESH_ON_POWER_UP_ON <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_ON>`:
      The Light state is set to Default Light on power up, or to the last known non-zero Light state if the Default Light is not set.
    - :cpp:enumerator:`BT_MESH_ON_POWER_UP_RESTORE <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_RESTORE>`:
      The Light state is set to the last known Light level (zero or non-zero).

    The user is expected to hold the state memory and provide access to the state through the :cpp:type:`bt_mesh_lightness_srv_handlers` handler structure.

**Default Light**: ``s16_t``
    The Default Light state is a meta state that controls the default non-zero Light level.
    It is used when the light is turned on, but its exact level is not specified.

    The memory for the Default Light state is held by the model, and the application may receive updates on state changes through the :cpp:member:`bt_mesh_lightness_srv_handlers::default_update` callback.

    The Default Light state uses the configured lightness representation.

**Light Range**: :cpp:type:`bt_mesh_lightness_range`
    The Light Range state is a meta state that determines the accepted Light level range.

    If the Light level is set to a value outside the current Light Range, it is moved to fit inside the range.

    If the Light Range changes to exclude the current Light level, the Light level should be changed accordingly.

    .. note::
        The Light level may always be set to zero, even if this is outside the current Light Range.

    The memory for the Light Range state is held by the model, and the application may receive updates on state changes through the :cpp:member:`bt_mesh_lightness_srv_handlers::range_update` callback.

    The Light Range state uses the configured lightness representation.

Extended models
****************

The Light Lightness Server extends the following models:

- :ref:`bt_mesh_lvl_srv_readme`
- :ref:`bt_mesh_ponoff_srv_readme`

As the states of both extended models are bound to states in the Light Lightness Server, the states of the extended models are not exposed directly to the application.

Persistent storage
*******************

The Light Lightness Server stores the following information:

    * Any changes to the Default Light and Light Range states.
    * The last known non-zero Light level.
    * Whether the light is on or off.

This information is used to reestablish the correct Light level when the device powers up.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/lightness_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_srv.c`

.. doxygengroup:: bt_mesh_lightness_srv
   :project: nrf
   :members:

----

.. _bt_mesh_lightness_cli_readme:

Light Lightness Client
======================

The Light Lightness Client model remotely controls the state of a Light Lightness Server model.

Contrary to the Server model, the Client only creates a single model instance in the mesh composition data.
The Light Lightness Client can send messages to both the Light Lightness Server and the Light Lightness Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/lightness_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_cli.c`

.. doxygengroup:: bt_mesh_lightness_cli
   :project: nrf
   :members:

----

Common types
=============

| Header file: :file:`include/bluetooth/mesh/lightness.h`

.. doxygengroup:: bt_mesh_lightness
   :project: nrf
   :members:
