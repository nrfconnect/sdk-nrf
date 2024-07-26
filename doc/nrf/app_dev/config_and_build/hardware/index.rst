.. _configuring_devicetree:

Configuring devicetree
######################

The |nRFVSC| is the recommended tool for editing :ref:`configure_application_hw`.
The extension offers several layers of `Devicetree integration <Devicetree support overview_>`_, ranging from summarizing devicetree settings in a menu and providing devicetree language support in the editor, to the Devicetree Visual Editor, a GUI tool for editing devicetree files.
Follow the steps in `How to create devicetree files`_ and use one of the following options to edit the :file:`.dts`, :file:`.dtsi`, and :file:`.overlay` files:

* `Devicetree Visual Editor <How to work with Devicetree Visual Editor_>`_
* `Devicetree language support`_

Like Kconfig fragment files, devicetree files can also be provided as overlays.
The devicetree overlay files use the :ref:`normalized board target name <app_boards_names>` and the file extension :file:`.overlay` (for example, ``nrf54h20dk/nrf54h20/cpuapp`` becomes :file:`nrf54h20dk_nrf54h20_cpuapp.overlay`).
When they are placed in the :file:`boards` folder and the devicetree overlay file name matches the board target (after normalization), the build system automatically selects and applies the overlay.
To select them manually, see :ref:`cmake_options`.

The following guides provide information about configuring specific aspects of hardware support related to devicetree.
Read them together with Zephyr's :ref:`zephyr:hardware_support` and :ref:`zephyr:dt-guide` guides, and the official `Devicetree Specification`_.
In particular, :ref:`zephyr:set-devicetree-overlays` explains how the base devicetree files are selected.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   pin_control
   use_gpio_pin_directly
