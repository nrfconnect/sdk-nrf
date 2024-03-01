.. _ug_zigbee:

Zigbee
######

.. zigbee_ug_intro_start

Zigbee is a portable, low-power software networking protocol that provides connectivity over a mesh network based on the IEEE 802.15.4 radio protocol.
It also defines an application layer that provides interoperability among all Zigbee devices.

The |NCS| provides support for developing Zigbee applications based on the third-party precompiled ZBOSS stack.
This stack is included as the :ref:`nrfxlib:zboss` library in nrfxlib (version |zboss_version|).
In combination with the integrated Zephyr RTOS, Zigbee in |NCS| allows for development of low-power connected solutions.

.. zigbee_ug_intro_end

See also :ref:`zigbee_samples` for the list of available Zigbee samples and :ref:`lib_zigbee` for the list of available Zigbee libraries.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   qsg
   supported_features
   architectures
   commissioning
   memory
   configuring
   configuring_libraries
   configuring_zboss_traces
   other_ecosystems
   tools
