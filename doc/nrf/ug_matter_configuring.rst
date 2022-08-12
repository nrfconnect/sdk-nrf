.. _ug_matter_configuring:

Configuring Matter
##################

When you build any of the available Matter samples to the supported development kits, you automatically build the Matter stack for the nRF Connect platform.
The development kit and the application running Matter stack that is programmed on the development kit together form the Matter accessory device.

The |NCS| supports Matter stack that is built on top of a low-power, 802.15.4-compatible Thread network, or on top of a Wi-Fi network.
To commission and control the Matter accessory device remotely over either of these networks, you need to set up a Matter controller on a mobile or a PC.
Matter controller is a role within the Matter environment.
This controller interacts with the accessory devices using the following protocols:

* Bluetooth® LE during the commissioning process - to securely pass the network credentials and provision the accessory device into the Thread network.
* Regular IPv6 communication after the accessory device joins the Thread or Wi-Fi network - to interact with each other by exchanging application messages.
  For example, to report temperature measurements of a sensor.

The following figure shows the protocol types and the available Matter controllers.

.. figure:: images/matter_protocols_controllers.svg
   :width: 600
   :alt: Protocol types and controllers used by Matter

   Protocol types and controllers used by Matter

Matter development environment depends on the following criteria:

* Network type:

  * To enable the IPv6 communication with the Matter accessory device over the Thread network, the Matter controller requires the Thread Border Router.
    This is because the Matter controller types described on this page do not have an 802.15.4 Thread interface.
    The Border Router bridges the Thread network with the network interface of the controller, for example Wi-Fi.
    Watch the following video for an overview of the Matter over Thread setup.

    .. raw:: html

       <div style="text-align: center;">
       <iframe width="560" height="315" src="https://www.youtube.com/embed/XWWJYraNZPs" title="Matter development environment video overview" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
       </div>


  * To enable the IPv6 communication with the Matter accessory device over the Wi-Fi network, you need a Wi-Fi Access Point, for example a Wi-Fi router, so that the Matter controller can interact with any Wi-Fi device directly.

* Matter controller setup:

  * You can run it on a separate device.
  * You can run it on the same device as the Thread Border Router (Matter over Thread only).

The following subpages describe possible development environments in more detail.

* Thread Border Router on Raspberry Pi and Matter controller on PC (CHIP Tool for Linux or macOS)

  .. include:: /ug_matter_configuring_env.rst
     :start-after: matter_env_ctrl_pc_start
     :end-before: matter_env_ctrl_pc_end

* Thread Border Router on Raspberry Pi and Matter controller for mobile (CHIP Tool for Android)

  .. include:: /ug_matter_configuring_env.rst
     :start-after: matter_env_ctrl_mobile_start
     :end-before: matter_env_ctrl_mobile_end

* Thread Border Router and Matter controller on the same device

  .. include:: /ug_matter_configuring_env.rst
     :start-after: matter_env_ctrl_one_start
     :end-before: matter_env_ctrl_one_end

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_matter_configuring_protocol.rst
   ug_matter_configuring_controller.rst
   ug_matter_configuring_env.rst
