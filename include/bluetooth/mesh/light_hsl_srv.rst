.. _bt_mesh_light_hsl_srv_readme:

Light HSL Server
################

.. contents::
   :local:
   :depth: 2

The Light HSL Server represents a single colored light.
It should be instantiated in the light fixture node.

The Light HSL Server provides functionality for working on the individual hue, saturation and lightness channels in a combined message interface, controlled by a :ref:`bt_mesh_light_hsl_cli_readme`.

The Light HSL Server adds the following new model instances in the composition data, in addition to the extended :ref:`bt_mesh_lightness_srv_readme` model:

* Light HSL Server
* Light HSL Setup Server

These model instances share the states of the Light HSL Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Light HSL states, as these model instances can be bound to different application keys:

* The Light HSL Server only provides write access to the HSL state for the user, in addition to read access to all the meta states.
* The Light HSL Setup Server provides write access to the corresponding :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` models' meta states.
  This allows the configurator devices to set up the range and the default value for the various HSL states.

Model composition
*****************

The Light HSL Server requires a :ref:`bt_mesh_light_hue_srv_readme` and a :ref:`bt_mesh_light_sat_srv_readme` model to be instantiated in any of the following elements.
As each of the Hue, Saturation and Lightness Server models extend the :ref:`bt_mesh_lvl_srv_readme` model, they cannot be instantiated on the same element.

The recommended configuration for the Light HSL Server model is to instantiate the HSL Server (with its extended Lightness Server model) on one element, then instantiate the corresponding Light Hue Server model on the subsequent element, and the corresponding Light Saturation Server model on the next element after that:

.. table::
   :align: center

   =================  =================  =======================
   Element N          Element N+1        Element N+2
   =================  =================  =======================
   Lightness Server   Light Hue Server   Light Saturation Server
   Light HSL Server
   =================  =================  =======================

In the application code, this would look like this:

.. code-block:: c

   static struct bt_mesh_light_hsl_srv hsl_srv =
   	BT_MESH_LIGHT_HSL_SRV_INIT(&hue_cb, &sat_cb, &light_cb);

   static struct bt_mesh_elem elements[] = {
   	BT_MESH_ELEM(
   		1, BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_HSL_SRV(&hsl_srv)),
   		BT_MESH_MODEL_NONE),
   	BT_MESH_ELEM(
   		2, BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_HUE_SRV(&hsl_srv.hue)),
   		BT_MESH_MODEL_NONE),
   	BT_MESH_ELEM(
   		3, BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_SAT_SRV(&hsl_srv.sat)),
   		BT_MESH_MODEL_NONE),
   };

.. note::
   The :c:struct:`bt_mesh_light_hsl_srv` contains the memory for its corresponding Light Hue Server and Light Saturation Server.
   When instantiating these models in the composition data, the model entries must point to these substructures.

The Light HSL Server does not contain any states on its own, but instead operates on the underlying Hue, Saturation and Lightness Server model's states.
Because of this, the Light HSL Server does not have a message handler structure, but will instead defer its messages to the individual submodels' handler callbacks.

States
******

None.

Extended models
****************

The Light HSL Server extends the following models:

* :ref:`bt_mesh_lightness_srv_readme`

Additionally, the Light HSL Server model requires a :ref:`bt_mesh_light_hue_srv_readme` and a :ref:`bt_mesh_light_sat_srv_readme` to be instantiated on any of the following elements, using the :c:member:`bt_mesh_light_hsl_srv.hue` and :c:member:`bt_mesh_light_hsl_srv.sat` structures.

Persistent storage
*******************

The Light HSL Server does not store any data persistently, but will control the underlying Light Hue Server and Light Saturation Server models' state when the device is powered up.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/light_hsl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_hsl_srv.c`

.. doxygengroup:: bt_mesh_light_hsl_srv
   :project: nrf
   :members:
