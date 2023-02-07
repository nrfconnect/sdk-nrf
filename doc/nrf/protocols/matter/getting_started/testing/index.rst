.. _ug_matter_gs_testing:
.. _ug_matter_configuring_env:
.. _ug_matter_configuring:

Testing Matter in the |NCS|
###########################

When you build any of the available Matter samples to the supported development kits, you automatically build the Matter stack for the nRF Connect platform.
The development kit and the application running Matter stack that is programmed on the development kit together form the Matter accessory device.

The |NCS| supports Matter stack that is built on top of a low-power, 802.15.4-compatible Thread network, or on top of a Wi-Fi network.
To control the Matter accessory device remotely over either of these networks, you need to set up a Matter controller on a PC or using one of the compatible ecosystems.

|matter_controller_def_nolink|

The following figure shows the available Matter controllers in the |NCS|.

.. figure:: ../../overview/images/matter_setup_controllers_generic.png
   :width: 600
   :alt: Controllers used by Matter

   Controllers used by Matter

You can read more about the Matter controller on the :ref:`Matter network topologies <ug_matter_configuring_controller>` page.

Matter development environment also depends on the network type:

Matter over Thread
  To enable the IPv6 communication with the Matter accessory device over the Thread network, the Matter controller requires the Thread Border Router.
  This is because the Matter controller types described on this page do not have an 802.15.4 Thread interface.
  The Border Router bridges the Thread network with the network interface of the controller, for example Wi-Fi.

  In the Matter over Thread development environment, the Matter controller can be set up in one of the following ways:

  * On a separate device from the device running the Thread Border Router.
  * On the same device as the Thread Border Router.

  Watch the following video for an overview of the Matter over Thread setup.

  .. raw:: html

     <div style="text-align: center;">
     <iframe width="560" height="315" src="https://www.youtube.com/embed/XWWJYraNZPs" title="Matter over Thread environment overview video" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
     </div>

Matter over Wi-Fi
  To enable the IPv6 communication with the Matter accessory device over the Wi-Fi network, you need a Wi-Fi Access Point, for example a Wi-Fi router, so that the Matter controller can interact with any Wi-Fi device directly.

The following subpages describe how to set up different development environments, including Matter controller.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   thread_separate_otbr_linux_macos
   thread_one_otbr
   wifi_pc
