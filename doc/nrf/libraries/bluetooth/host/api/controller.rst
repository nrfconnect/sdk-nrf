.. _bluetooth_controller:

Bluetooth Controller
####################

.. contents::
   :local:
   :depth: 2

The Bluetooth Controller API provides host-side access to controller-specific configuration.
It currently includes functionality for setting the controller public address before Bluetooth is
enabled.
Use these APIs when the application must configure controller behavior prior to calling
:c:func:`bt_enable`.

API Reference
*************

| Header file: :file:`include/bluetooth/host/controller.h`

.. doxygengroup:: bt_ctrl
