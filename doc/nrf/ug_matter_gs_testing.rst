.. _ug_matter_gs_testing:
.. _ug_matter_configuring_env:
.. _ug_matter_configuring:

Testing Matter in the |NCS|
###########################

When you build any of the available Matter samples to the supported development kits, you automatically build the Matter stack for the nRF Connect platform.
The development kit and the application running Matter stack that is programmed on the development kit together form the Matter accessory device.

The |NCS| supports Matter stack that is built on top of a low-power, 802.15.4-compatible Thread network, or on top of a Wi-Fi network.
To control the Matter accessory device remotely over either of these networks, you need to set up a Matter controller on a mobile or a PC.
Matter controller is a role within the Matter environment.
This controller interacts with the accessory devices using the following protocols:

* Bluetooth® LE during the commissioning process - to securely pass the network credentials and provision the accessory device into the Thread network during commissioning.
  At this stage, the controller has the commissioner role.
  When the commissioning has completed, the device has joined the IPv6 network and is equipped with all information needed to securely operate in the Matter network with other IPv6 devices.
  For more information about the commissioning process, see :ref:`ug_matter_network_topologies_commissioning`.
* Regular IPv6 communication after the accessory device joins the Thread or Wi-Fi network - to interact with each other by exchanging application messages.
  For example, to report temperature measurements of a sensor.

The following figure shows the protocol types and the available Matter controllers.

.. figure:: images/matter_setup_controllers_generic.png
   :width: 600
   :alt: Protocol types and controllers used by Matter

   Protocol types and controllers used by Matter

Matter development environment depends on the network type:

Matter over Thread
  To enable the IPv6 communication with the Matter accessory device over the Thread network, the Matter controller requires the Thread Border Router.
  This is because the Matter controller types described on this page do not have an 802.15.4 Thread interface.
  The Border Router bridges the Thread network with the network interface of the controller, for example Wi-Fi.

  In the Matter over Thread development environment, the Matter controller can be set up in one of the following ways:

  * On a separate device from the device running the Thread Border Router.
  * On the same device as the Thread Border Router.

  Watch the following video for an overview of the Matter over Thread setup.

.. raw:: html

   <h1 align="center">
   <iframe id="Matter_over_Thread_overview"
           title="Matter over Thread environment video overview"
           width="560"
           height="315"
           src="https://www.youtube.com/embed/XWWJYraNZPs">
   </iframe>
   </h1>

Matter over Wi-Fi
  To enable the IPv6 communication with the Matter accessory device over the Wi-Fi network, you need a Wi-Fi Access Point, for example a Wi-Fi router, so that the Matter controller can interact with any Wi-Fi device directly.

The following subpages describe how to set up different development environments.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_matter_configuring_controller.rst
   ug_matter_gs_testing_thread_separate_otbr.rst
   ug_matter_gs_testing_thread_one_otbr.rst
   ug_matter_gs_testing_wifi_pc.rst
   ug_matter_gs_testing_wifi_mobile.rst
