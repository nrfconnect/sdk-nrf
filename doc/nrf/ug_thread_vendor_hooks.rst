..
  \input \begin \section \setlength \documentstyle \chapter
  Avoid pylint issues by making libmagic think this is a latex file!
.. _ug_thread_vendor_hooks:

Vendor hooks
############

.. contents::
   :local:
   :depth: 2

Vendor hooks are a software mechanism that allows adding custom Spinel commands.
Such vendor-specific commands can be used to add support for enhanced application features.
For example, they allow you to add radio or crypto functionalities, whose settings are managed from the host device.

In |NCS|, your implementation of the vendor hooks file is compiled by OpenThread within the NCP component.
The only information needed for compilation is the file location.
In this way, you can implement new features in the application or elsewhere without having to modify OpenThread components.

Vendor hooks are strictly connected to the `Spinel protocol`_ used in serial communication between host device and the MCU board working in :ref:`Thread NCP or RCP architecture <thread_architectures_designs_cp>`.
Spinel uses serial frames with commands that describe the activity to perform (for example, get or set), and with properties that define the object on which the activity is to be performed.
You can use the available commands and properties, but also define your set of instructions to provide new features by using the vendor hooks.

.. note::
    Both devices used for communication must support user-defined commands and properties to achieve the expected result.
    See the following sections for information about how to add your features on the host and the NCP/RCP devices.


ID range of general vendor properties
*************************************

The Spinel protocol associates specific properties with numeric identifiers.
The ID range reserved for the vendor properties spans from ``0x3c00`` to ``0x4000``.

You are not allowed to use IDs from outside this range for your properties, as this might negatively affect the Spinel protocol operation, and the vendor hooks mechanism will not work.


Host device configuration
*************************

In most cases, the host device uses tools that support the Spinel protocol (like `wpantund`_ or `Pyspinel`_) to communicate with the NCP/RCP device.
You can, however, use another tool as long as it supports the Spinel protocol.

Adding properties in Pyspinel
=============================

Pyspinel is the recommended tool for adding properties, given its ease-of-use.

To configure new properties in Pyspinel, you must update the following files:

* :file:`const.py` - This file contains the ``VENDOR_SPINEL`` class, which stores definitions of the identifier values of vendor properties.
  Use this file to add properties.
* :file:`codec.py` - This file contains definitions of handler methods called on property detection.
  Use this file to add definitions of handlers for your properties.
* :file:`vendor.py` - This file contains methods to capture property occurrences from the console and call proper handlers.

These files are located in the :file:`vendor` directory, which can be found in the main project directory.

Complete the following steps to add a property in Pyspinel:

1. Clone the `Pyspinel`_ respository and install the tool.
#. Navigate to the :file:`vendor` directory that contains the :file:`codec.py`, :file:`const.py`, and :file:`vendor.py` files.
   This directory is located in the main project directory.
#. Open the :file:`const.py` file for editing.
#. Add properties to the :file:`const.py` file.
   For example, add the following properties:

   * ``PROP_VENDOR_NAME``, which is used to get the vendor name from the device.
   * ``PROP_VENDOR_AUTO_ACK_ENABLED``, which is used to get or set the auto ACK mode state.

   The class should contain code similar to the following:

   .. code-block:: python

      class VENDOR_SPINEL(object):
        """
        Class to extend SPINEL constant variables for example:
            PROP_VENDOR__BEGIN = 0x3C00

            PROP_VENDOR_HOOK = PROP_VENDOR__BEGIN + 0
            PROP_VENDOR__END = 0x4000
        """
        PROP_VENDOR__BEGIN = 0x3C00

        PROP_VENDOR_NAME = PROP_VENDOR__BEGIN
        PROP_VENDOR_AUTO_ACK_ENABLED = PROP_VENDOR__BEGIN + 1
        PROP_VENDOR_HW_CAPABILITIES = PROP_VENDOR__BEGIN + 2

        PROP_VENDOR__END = 0x4000
        pass

   You can also add your own properties, but you must assign them IDs from the proper vendor range.
#. Open the :file:`codec.py` file for editing.
#. In the :file:`codec.py` file, add definitions of handlers.
   For example, for the properties added in the :file:`const.py` file:

   .. code-block:: python

      class VendorSpinelPropertyHandler(SpinelCodec):
        """
        Class to extend Spinel property Handler with new methods.
        Methods define parsers for Vendor Hooks for example:
        `def VENDOR_HOOK_PROPERTY(self, _wpan_api, payload): return self.parse_C(payload)`
        """
        def NAME(self, _, payload):
            return self.parse_U(payload)

        def AUTO_ACK(self, _, payload):
            return self.parse_C(payload)
        pass


      WPAN_PROP_HANDLER = VendorSpinelPropertyHandler()

      # Parameter to extend SPINEL_PREP_DISPATCH with Vendor properties for example:
      #   `VENDOR_SPINEL_PROP_DISPATCH = {VENDOR_SPINEL.PROP_VENDOR_HOOK: WPAN_PROP_HANDLER.VENDOR_HOOK_PROPERTY}`
      VENDOR_SPINEL_PROP_DISPATCH = {
          VENDOR_SPINEL.PROP_VENDOR_NAME:
            WPAN_PROP_HANDLER.NAME,
          VENDOR_SPINEL.PROP_VENDOR_AUTO_ACK_ENABLED:
            WPAN_PROP_HANDLER.AUTO_ACK}

   .. note::
        Handlers call different parsing methods depending on the type of data passed with the property.
        In this case, ``NAME`` is of ``string`` type and ``AUTO_ACK`` is of ``uint8`` type, so methods ``parse_U`` and ``parse_C`` should be used.
        For details, see the ``SpinelCodec`` class in :file:`spinel/codec.py`.

#. Open the :file:`vendor.py` file for editing.
#. Extend the list of command names with the new properties and make sure they are included in the ``do_vendor`` method:

   .. code-block:: python

      class VendorSpinelCliCmd():
        """
        Extended Vendor Spinel Cli with vendor hooks commands.
        INPUT:
            spinel-cli > vendor help
        OUTPUT:
            Available vendor commands:
            ==============================================
            help
        """
        vendor_command_names = ['help', 'name', 'auto_ack']

        def do_vendor(self, line):
            params = line.split(" ")
            if params[0] == 'help':
                self.print_topics("\nAvailable vendor commands:",
                                  VendorSpinelCliCmd.vendor_command_names, 15, 30)
            elif params[0] == 'name':
                self.handle_property(None, VENDOR_SPINEL.PROP_VENDOR_NAME)
            elif params[0] == 'auto_ack':
                if len(params) > 1:
                    self.handle_property(params[1], VENDOR_SPINEL.PROP_VENDOR_AUTO_ACK_ENABLED)
                else:
                    self.handle_property(None, VENDOR_SPINEL.PROP_VENDOR_AUTO_ACK_ENABLED)


NCP/RCP device configuration
****************************

In |NCS|, the OpenThread NCP base component is responsible for processing Spinel frames and performing appropriate operations.
If it finds a frame with an unknown property ID, but one that fits the vendor ID range, it calls vendor handler methods.
You must define these methods beforehand.

Handler methods can check the property ID and perform different actions depending on its value.
They can also ignore the value, for example, if the property was defined by another vendor and you want to filter it out.

For a detailed description of how to enable the vendor hook feature in a sample, see the :ref:`ot_coprocessor_sample` sample documentation.

.. _ug_thread_vendor_hooks_testing:

Testing vendor hooks after configuration
****************************************

To test the vendor hook feature, you need a development kit that is programmed with either the :ref:`ot_coprocessor_sample` sample or another compatible sample.

Complete the following steps:

1. Connect the development kit's SEGGER J-Link USB port to the USB port on your PC with an USB cable.
#. Get the development kit's serial port name (for example, :file:`/dev/ttyACM0`).
#. Open a shell and run Pyspinel by using the following command, with *baud_rate* set to ``1000000`` and *serial_port_name* set to the port name from the previous step:

   .. parsed-literal::
      :class: highlight

      python3 spinel-cli.py -u *serial_port_name* -b *baud_rate*

#. In the Pyspinel shell, run the following command to check the list of available vendor properties:

   .. code-block:: console

      spinel-cli > vendor help

   The output looks similar to the following:

   .. code-block:: console

      Available vendor commands:
      ===========================
      help  name  auto_ack

#. In the Pyspinel shell, run the following command to get the device vendor name:

   .. code-block:: console

      spinel-cli > vendor name

   The output looks similar to the following:

   .. code-block:: console

      Nordic Semiconductor
      Done

#. In the Pyspinel shell, run the ``auto_ack`` command to get the current state of the device auto ACK mode:

   .. code-block:: console

      spinel-cli > vendor auto_ack

   The output looks similar to the following:

   .. code-block:: console

      1
      Done

#. In the Pyspinel shell, run the ``auto_ack`` command with a value to change the current state of the device auto ACK mode:

   .. code-block:: console

      spinel-cli > vendor auto_ack 0

   The output looks similar to the following:

   .. code-block:: console

      0
      Done
