.. _caf_shell:

CAF: Shell module
#################

.. contents::
   :local:
   :depth: 2

The shell module of the :ref:`lib_caf` (CAF) allows interactions with CAF modules and events using a predefined set of Zephyr's :ref:`zephyr:shell_api` commands.
The module introduces a shell command for submitting events informing about button press or release (:c:struct:`button_event`).
Submitting :c:struct:`button_event` related to the hardware button presses can be handled by the :ref:`caf_buttons`.

Configuration
*************

To use the module, enable the following Kconfig options:

* :kconfig:option:`CONFIG_CAF_SHELL` - This option is mandatory as it enables this module.
* :kconfig:option:`CONFIG_SHELL` - This option enables Zephyr shell that is used by the module to receive commands from user.
* :kconfig:option:`CONFIG_CAF` - This option enables the Common Application Framework.

This module selects the Kconfig option :kconfig:option:`CONFIG_CAF_BUTTON_EVENTS`.

Usage
*****

All shell commands of this module are subcommands of the :command:`caf_events` command.
To send a :c:struct:`button_event`, use the following syntax:

  .. code-block:: console

     caf_events button_event [button_id] [pressed]

The :command:`button_event` subcommand has two required arguments:

* ``button_id`` - A decimal number represents the ID of a button.
* ``pressed`` - Button press state, can only be either ``y`` or ``n``.
