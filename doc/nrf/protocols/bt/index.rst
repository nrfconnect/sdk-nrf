.. _ug_bt:

Bluetooth
#########

Bluetooth® is a short-range wireless communication standard. The standard is managed by Bluetooth SIG and the technology
is found in most phones, laptop computers and tablets. Operating in the 2.4 GHz frequency band the technology can be
used world wide and supports a wide range of use cases.

Nordic Semiconductor products support the power efficient Bluetooth LE protocol. The nRF Connect SDK  provides
qualified Bluetooth core stack, profiles and application examples for typical use cases.

If you want to go through an online training course to familiarize yourself with Bluetooth Low Energy and the development of Bluetooth LE applications, enroll in the `Bluetooth LE Fundamentals course`_ in the `Nordic Developer Academy`_.

The following section describes Bluetooth solution areas and architecture, as well as the Bluetooth Mesh protocol.
It also includes guidelines on how to qualify a product that uses Bluetooth technology.

To enable Bluetooth LE in your application, you can use the standard HCI-based architecture, where the Bluetooth Host libraries (:ref:`zephyr:bluetooth`) are included in your application or run Bluetooth API functions as remote procedure calls using :ref:`ble_rpc`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   bt_solutions.rst
   bt_stack_arch.rst
   bt_mesh/index.rst
   bt_qualification/index.rst
