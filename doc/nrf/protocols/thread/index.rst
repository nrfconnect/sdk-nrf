.. _ug_thread:

Thread
######

.. thread_intro_start

Thread is a low-power mesh networking technology, designed specifically for home automation applications.
It is an IPv6-based standard that uses 6LoWPAN technology over the IEEE 802.15.4 protocol.
You can connect a Thread mesh network to the Internet with a Thread Border Router.

The |NCS| provides support for developing Thread applications based on the OpenThread stack.
The OpenThread stack is integrated into Zephyr.

.. thread_intro_end

You can use Thread in parallel with Bluetooth® Low Energy.
See :ref:`ug_multiprotocol_support` for more information.

See :ref:`openthread_samples` for the list of available Thread samples.

You can find more information about Thread on the `Thread Group`_ pages.


.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   overview/index
   configuring
   prebuilt_libs
   tools
   certification
   device_types
   sed_ssed
   tcat
