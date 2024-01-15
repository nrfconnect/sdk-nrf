.. _configuring_devicetree:

Configuring devicetree
######################

The |nRFVSC| is the recommended tool for editing :ref:`configure_application_hw`.
The extension offers several layers of `Devicetree integration <Devicetree support overview_>`_, ranging from summarizing devicetree settings in a menu and providing devicetree language support in the editor, to the Devicetree Visual Editor, a GUI tool for editing devicetree files.
Follow the steps in `How to create devicetree files`_ and use one of the following options to edit the :file:`.dts`, :file:`.dtsi`, and :file:`.overlay` files:

* `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_
* `Devicetree language support`_

The following guides provide information about configuring specific aspects of hardware support related to devicetree.
Read them together with Zephyr's :ref:`zephyr:hardware_support` and :ref:`zephyr:dt-guide` guides, and the official `Devicetree Specification`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   pin_control
   use_gpio_pin_directly
