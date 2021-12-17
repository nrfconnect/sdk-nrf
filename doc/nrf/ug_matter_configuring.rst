.. _ug_matter_configuring:

Configuring Matter
##################

When you build any of the available Matter samples to the supported development kits, you automatically build the Matter stack for the nRF Connect platform.
The development kit and the application running Matter stack that is programmed on the development kit together form the Matter accessory device.

.. raw:: html

   <div style="text-align: center;">
   <iframe width="560" height="315" src="https://www.youtube.com/embed/XWWJYraNZPs" title="Matter development environment video overview" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
   </div>

Currently, the |NCS| only supports Matter stack that is built on top of a low-power, 802.15.4-compatible Thread network.
To commission and control the Matter accessory device remotely over such a network, you need to set up a Matter controller on a mobile or a PC.
Matter controller is a role within the Matter environment.
This controller interacts with the accessory devices using the following protocols:

* Bluetooth® LE during the commissioning process - to securely pass the network credentials and provision the accessory device into the Thread network.
* Regular IPv6 communication after the accessory device joins the Thread network - to interact with each other by exchanging application messages.
  For example, to report temperature measurements of a sensor.

The following figure shows the protocol types and the available Matter controllers.

.. figure:: images/matter_protocols_controllers.svg
   :width: 600
   :alt: Protocol types and controllers used by Matter

   Protocol types and controllers used by Matter

To enable the IPv6 communication with the Matter accessory device over the Thread network, the Matter controller requires the Thread Border Router.
This is because the Matter controller types described on this page do not have an 802.15.4 Thread interface.
The Border Router bridges the Thread network with the network interface of the controller, for example Wi-Fi.

Matter development environment setup options depend on how you run the Matter controller.
You can either run it on the same device as the Thread Border Router or run the Matter controller and the Thread Border Router on separate devices.

Depending on your choice, you can set up one of the following development environments:

* Thread Border Router on Raspberry Pi and mobile Matter Controller

  .. include:: /ug_matter_configuring_env.rst
     :start-after: matter_env_ctrl_mobile_start
     :end-before: matter_env_ctrl_mobile_end

* Thread Border Router on Raspberry Pi and Matter Controller on PC

  .. include:: /ug_matter_configuring_env.rst
     :start-after: matter_env_ctrl_pc_start
     :end-before: matter_env_ctrl_pc_end

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
