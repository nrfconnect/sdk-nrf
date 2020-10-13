.. _ug_thread_intro:

OpenThread overview
###################

The |NCS|'s implementation is based on the OpenThread stack, which is integrated into Zephyr.
The integration with the stack and the radio driver is ensured by Zephyr's L2 layer, which acts as intermediary with Thread on the |NCS| side.

OpenThread is a portable and flexible open-source implementation of the Thread networking protocol, created by Nest in active collaboration with Nordic to accelerate the development of products for the connected home.

Among others, OpenThread has the following main advantages:

* A narrow platform abstraction layer that makes the OpenThread platform-agnostic.
* Small memory footprint.
* Support for system-on-chip (SoC), network co-processor (NCP) and radio co-processor (RCP) designs.
* Official Thread certification.

For more information about some aspects of Thread, see the subpages.

You can also find more information at `OpenThread.io`_ and `Thread Group`_ pages.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_thread_supported_features.rst
   ug_thread_architectures.rst
   ug_thread_ot_integration.rst
   ug_thread_ot_memory.rst
   ug_thread_commissioning.rst
