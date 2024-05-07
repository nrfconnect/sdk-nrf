.. _app_boards:

Board support
#############

The |NCS| provides board definitions for all Nordic Semiconductor devices and follows Zephyr's :ref:`zephyr:hw_support_hierarchy`.

For the complete list, see the tables on the :ref:`app_boards_names` page, listed by :ref:`app_boards_names_zephyr`, :ref:`app_boards_names_nrf`, and :ref:`shield_names_nrf`.
You can select the board targets for these boards when :ref:`building`.

Some boards can support Cortex-M Security Extensions (CMSE), with their board targets separated for different :ref:`app_boards_spe_nspe`.

In addition, you can :ref:`define custom boards <defining_custom_board>`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   board_names
   processing_environments
   defining_custom_board
