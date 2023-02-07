.. _ug_thread_intro:

OpenThread overview
###################

`OpenThread <OpenThread.io_>`_ is a portable and flexible open-source implementation of the Thread networking protocol.
It has been created by Google in active collaboration with Nordic Semiconductor, to accelerate the development of products for the connected home.

The Thread implementation in |NCS| is based on the OpenThread stack, which is integrated into Zephyr.
You can find the OpenThread stack in :file:`ncs/modules/lib/openthread`.

Among others, OpenThread has the following main advantages:

* A narrow platform abstraction layer that makes the OpenThread platform-agnostic
* Small memory footprint
* Support for system-on-chip (SoC), network co-processor (NCP), and radio co-processor (RCP) designs
* Official Thread certification

You can find more information about OpenThread at `OpenThread.io`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_thread_supported_features.rst
   ug_thread_architectures.rst
   ug_thread_communication.rst
   ug_thread_ot_integration.rst
   ug_thread_ot_memory.rst
   ug_thread_power_consumption.rst
   ug_thread_commissioning.rst
