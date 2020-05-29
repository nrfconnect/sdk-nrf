.. _openthread_stack_architecture:

OpenThread stack architecture
#############################

OpenThread's portable nature makes no assumptions about platform features.
OpenThread provides the hooks to use enhanced radio and crypto features, reducing system requirements, such as memory, code, and compute cycles.
This can be done per platform, while retaining the ability to default to a standard configuration.

.. figure:: images/ot-arch_2x.png
   :alt: OpenThread architecture

   OpenThread architecture
