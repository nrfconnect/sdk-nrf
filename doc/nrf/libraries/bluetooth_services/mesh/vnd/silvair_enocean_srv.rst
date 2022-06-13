.. _bt_mesh_silvair_enocean_srv_readme:

Silvair EnOcean Proxy Server
############################

.. contents::
   :local:
   :depth: 2

The Silvair EnOcean Proxy Server model integrates an EnOcean switch together with :ref:`bt_mesh_onoff_srv_readme` and :ref:`bt_mesh_lvl_srv_readme` models, enabling dimming and on/off light control.
It implements the `Silvair EnOcean Switch Mesh Proxy Server`_ specification.

The model initializes the :ref:`bt_enocean_readme` library.
The EnOcean switch can be automatically commissioned/decommissioned if option :kconfig:option:`CONFIG_BT_MESH_SILVAIR_ENOCEAN_AUTO_COMMISSION` is set.

The Silvair EnOcean Proxy Server uses either one or two elements on a node.
Each element handles its own button pair and has its own corresponding :ref:`bt_mesh_onoff_srv_readme` and :ref:`bt_mesh_lvl_srv_readme` models.
These models should not be accessed by the user application as the Silvair EnOcean Proxy Server model uses them to send messages.
The publication settings of these client models can be configured to control a specific light or a group of lights.
However, the :ref:`bt_mesh_onoff_srv_readme` and :ref:`bt_mesh_lvl_srv_readme` models need to be instantiated for each element.
:c:macro:`BT_MESH_MODEL_SILVAIR_ENOCEAN_BUTTON` is provided to do this, and should be used as presented below:

.. code-block:: c

   struct bt_mesh_silvair_enocean_srv enocean;

   static struct bt_mesh_elem elements[] = {
	   BT_MESH_ELEM(
         1,
		   BT_MESH_MODEL_LIST(BT_MESH_MODEL_SILVAIR_ENOCEAN_BUTTON(&silvair_enocean, 0)),
		   BT_MESH_MODEL_LIST(BT_MESH_MODEL_SILVAIR_ENOCEAN_SRV(&silvair_enocean))),
   	BT_MESH_ELEM(
         2,
		   BT_MESH_MODEL_LIST(BT_MESH_MODEL_SILVAIR_ENOCEAN_BUTTON(&silvair_enocean, 1)),
		   BT_MESH_MODEL_NONE),
   };

The Silvair EnOcean Proxy Server does not require any message handler callbacks.

States
======

None

Extended models
===============

None

Persistent storage
==================

If :kconfig:option:`CONFIG_BT_ENOCEAN_STORE` is enabled, the Silvair EnOcean Proxy Server stores the commissioned EnOcean device address.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/vnd/silvair_enocean_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/vnd/silvair_enocean_srv.c`

.. doxygengroup:: bt_mesh_silvair_enocean_srv
   :project: nrf
   :members:
