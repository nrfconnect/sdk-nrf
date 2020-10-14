.. _bt_mesh_onoff_srv_readme:

Generic OnOff Server
####################

.. contents::
   :local:
   :depth: 2

The Generic OnOff Server contains a single controllable On/Off state.

States
======

The Generic OnOff Server model contains the following state:

Generic OnOff: ``boolean``
    Generic boolean state representing an On/Off state.
    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_onoff_srv_handlers` handler structure.

    Changes to the Generic OnOff state may include transition parameters.
    When transitioning to a new OnOff state:

    * The state should be `on` during the entire transition, regardless of the target state.
      This ensures that any bound non-binary states can have non-zero values during the transition.
      Any request to read out the current OnOff state while in a transition should report the current OnOff value being `on`.
    * The ``remaining_time`` parameter should be reported in milliseconds (including an optional delay).

    If the transition parameters include a delay, the state must remain unchanged until the delay expires.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_onoff_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_onoff_srv.c`

.. doxygengroup:: bt_mesh_onoff_srv
   :project: nrf
   :members:
