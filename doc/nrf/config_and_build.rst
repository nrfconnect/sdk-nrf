.. _ug_app_dev:
.. _config_and_build:

Configuration and building
##########################

The |NCS| build and configuration system is based on the one from Zephyr, with some additions.

The guides in this section provide universal build and configuration steps valid for :ref:`all supported boards <app_boards>`.
Depending on the board you are working with, check also its :ref:`hardware guide <device_guides>`, as some nRF Series families have specific exceptions and rules to be applied on top of the general configuration procedures.

Make sure to consider :ref:`app_bootloaders` already at this stage of the application development.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   config_and_build/config_and_build_system
   config_and_build/board_support
   config_and_build/pin_control
   config_and_build/programming
   config_and_build/modifying
   config_and_build/multi_image
   config_and_build/bootloaders_and_dfu/index
