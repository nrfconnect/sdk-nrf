.. _ug_bt:

Bluetooth
#########

Bluetooth® is a short-range wireless communication standard. The standard is managed by Bluetooth SIG and the technology
is found in most phones, laptop computers and tablets. Operating in the 2.4 GHz frequency band the technology can be
used world wide and supports a wide range of use cases.

Nordic Semiconductor products support the power efficient Bluetooth LE protocol. The nRF Connect SDK  provides
qualified Bluetooth core stack, profiles and application examples for typical use cases.

The following section describes Bluetooth solution areas and architecture.
It also contains descriptions of Bluetooth LE Controller and Bluetooth Mesh, including the guidelines on how to qualify a product that uses these subsystems.

To enable Bluetooth LE in your application, you can use the standard HCI-based architecture, where the Bluetooth Host libraries (:ref:`zephyr:bluetooth`) are included in your application or run Bluetooth API functions as remote procedure calls using :ref:`ble_rpc`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   bt_solutions.rst
   bt_stack_arch.rst
   ble/index.rst
   bt_mesh/index.rst
   bt_qualification/index.rst
