.. _mcuboot_index_ncs:

Introduction to MCUboot
#######################

.. contents::
   :local:
   :depth: 2

MCUboot is a secure bootloader for 32-bit microcontrollers.

Overview
********

MCUboot defines a common infrastructure for the bootloader and the system flash layout on microcontroller systems, and provides a secure bootloader that enables easy software upgrade.

MCUboot is not dependent on any specific operating system and hardware and relies on hardware porting layers from the operating system it works with.
Currently MCUboot works with the various operating systems and SDKs, such as Zephyr and the nRF Connect SDK.

MCUboot is an open governance project.
See the `membership list <https://github.com/mcu-tools/mcuboot/wiki/Members>`_ for current members, and the `project charter <https://github.com/mcu-tools/mcuboot/wiki/MCUboot-Project-Charter>`_ for more details.

Roadmap
*******

The issues being planned and worked on are tracked using GitHub issues.
To give your input, visit `MCUboot GitHub Issues <https://github.com/mcu-tools/mcuboot/issues>`_.

Source files
************

You can find additional documentation on the bootloader in the source files.
For more information, use the following links:

* `boot/bootutil <https://github.com/mcu-tools/mcuboot/tree/main/boot/bootutil>`_ - The core of the bootloader itself.
* `boot/boot_serial <https://github.com/mcu-tools/mcuboot/tree/main/boot/boot_serial>`_ - Support for serial upgrade within the bootloader itself.
* `boot/zephyr <https://github.com/mcu-tools/mcuboot/tree/main/boot/zephyr>`_ - Port of the bootloader to Zephyr.
* `imgtool <https://github.com/mcu-tools/mcuboot/tree/main/scripts/imgtool.py>`_ - A tool to securely sign firmware images for booting by MCUboot.
* `sim <https://github.com/mcu-tools/mcuboot/tree/main/sim>`_ - A bootloader simulator for testing and regression.

Joining the project
*******************

Developers are welcome!

Use the following links to join or see more about the project:

* `MCUboot official developer mailing list <https://groups.io/g/MCUBoot>`_
* `MCUboot official Slack channel <https://mcuboot.slack.com/>`_
  Get `your invite <https://join.slack.com/t/mcuboot/shared_invite/MjE2NDcwMTQ2MTYyLTE1MDA4MTIzNTAtYzgyZTU0NjFkMg>`_
* `Current members <https://github.com/mcu-tools/mcuboot/wiki/Members>`_
* `Project charter <https://github.com/mcu-tools/mcuboot/wiki/MCUboot-Project-Charter>`_
