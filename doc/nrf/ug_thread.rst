.. _ug_thread:

Working with Thread
###################

The |NCS| provides support for developing applications using the Thread protocol.
This support is based on the OpenThread stack, which is integrated into Zephyr.

For information about how to start working with Thread in |NCS|, see the following pages:

.. toctree::
   :maxdepth: 1

   ug_thread_overview.rst
   ug_thread_configuring.rst

See :ref:`openthread_samples` for the list of available Thread samples.

When working with Thread in |NCS|, you can use the following tools during Thread application development:

* `nRF Thread Topology Monitor`_ - This desktop application helps to visualize the current network topology.
* `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ - Tool for analyzing network traffic during development.
* `wpantund`_ - Utility for providing a native IPv6 interface to a Network Co-Processor.
* `pyspinel`_ - Tool for controlling OpenThread Network Co-Processor instances through command line interface.

Using Thread tools is optional.

----

Copyright disclaimer
    |Google_CCLicense|
