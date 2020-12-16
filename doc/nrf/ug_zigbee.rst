.. _ug_zigbee:

Zigbee
######

.. zigbee_ug_intro_start

Zigbee is a portable, low-power software networking protocol that provides connectivity over a mesh network based on the IEEE 802.15.4 radio protocol.
It also defines an application layer that provides interoperability among all Zigbee devices.

The |NCS| provides support for developing Zigbee applications based on the third-party precompiled ZBOSS stack.
This stack is included as the :ref:`nrfxlib:zboss` library in nrfxlib.
In combination with the integrated Zephyr RTOS, Zigbee in |NCS| allows for development of low-power connected solutions.

.. zigbee_ug_intro_end

See :ref:`zigbee_samples` for the list of available Zigbee samples.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_zigbee_supported_features.rst
   ug_zigbee_architectures.rst
   ug_zigbee_memory.rst
   ug_zigbee_configuring.rst
   ug_zigbee_configuring_libraries.rst
   ug_zigbee_tools.rst
