.. _ug_app_dev:
.. _configuration_and_build:

Configuration and building
##########################

After you have :ref:`created an application <create_application>`, you need to configure and build it in order to be able to run it.

The |NCS| configuration and build system is based on the one from :ref:`Zephyr <zephyr:getting_started>`, with some additions.

The following figure lists the main tools and configuration methods in the |NCS|.
All of them have a role in the creation of an application, from configuring the libraries or applications to building them.

.. figure:: images/ncs-toolchain.svg
   :alt: nRF Connect SDK tools and configuration

   |NCS| tools and configuration methods

* Devicetree describes the hardware.
* Kconfig generates definitions that configure the software.
* Partition Manager describes the memory layout.
* CMake generates build files based on the provided :file:`CMakeLists.txt` files, which use information from devicetree files, Kconfig, and Partition Manager.
* Ninja (comparable to Make) uses the build files to build the program.
* The compiler (for example, GCC) creates the executables.

For a more detailed overview, see :ref:`app_build_system`.

Depending on the board you are working with, check also its :ref:`hardware guide <device_guides>`, as some nRF Series families have specific exceptions and rules to be applied on top of the general configuration procedures.

Make sure to consider :ref:`app_bootloaders` and :ref:`app_dfu` already at this stage of the application development.

.. note::
   If you want to go through dedicated training related to some of the topics covered here, enroll in the `nRF Connect SDK Intermediate course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   config_and_build/config_and_build_system
   config_and_build/board_support/index
   config_and_build/configuring_app/index
   config_and_build/companion_components
   config_and_build/programming
   config_and_build/multi_image
   config_and_build/bootloaders/index
   config_and_build/dfu/index
