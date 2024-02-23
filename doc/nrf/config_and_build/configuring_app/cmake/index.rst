.. _configuring_cmake:

Adding files and configuring CMake
##################################

.. contents::
   :local:
   :depth: 2

As described in more detail in :ref:`app_build_system`, the Zephyr and |NCS| build systems are based on `CMake <CMake documentation_>`_.
For this reason, every application in Zephyr and the |NCS| must have a :file:`CMakeLists.txt` file.
This file is the entry point of the build system as it specifies the application source files for the compiler to include in the :ref:`configuration phase <app_build_system>`.

Maintaining CMakeLists.txt
**************************

The recommended method to maintain and update the :file:`CMakeLists.txt` file is to use the |nRFVSC|.
The extension provides support for the `source control with west`_ and `CMake build system`_, including `build configuration management <How to work with build configurations_>`_ and `source and config files overview <Details View_>`_.

.. _modifying_files_compiler:

Adding source files to CMakeLists.txt
*************************************

You can add source files to the ``app`` CMake target with the :c:func:`target_sources` function provided by CMake.

Pay attention to the following configuration options:

* If your application is complex, you can split it into subdirectories.
  These subdirectories can provide their own :file:`CMakeLists.txt` files.
  (The main :file:`CMakeLists.txt` file needs to include those.)
* The build system searches for header files in include directories.
  Add additional include directories for your application with the :c:func:`target_include_directories` function provided by CMake.
  For example, if you want to include an :file:`inc` directory, the code would look like the following:

  .. code-block:: c

     target_include_directories(app PRIVATE inc)

See :ref:`zephyr:zephyr-app-cmakelists` in the Zephyr documentation for more information about how to edit :file:`CMakeLists.txt`.

.. note::
    You can also read the `CMake Tutorial`_ in the CMake documentation for a better understanding of how :file:`CMakeLists.txt` are used in a CMake project.
    This tutorial however differs from Zephyr and |NCS| project configurations, so use it only as reference.

.. _cmake_options:
.. _building_overlay_files:

Providing CMake options
***********************

You can provide additional options for building your application to the CMake process, which can be useful, for example, to switch between different build scenarios.
These options are specified when CMake is run, thus not during the actual build, but when configuring the build.

The |NCS| uses the same CMake build variables as Zephyr and they are compatible with both CMake and west.
For the complete list of build variables in Zephyr and more information about them, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.
The following table lists the most common ones used in the |NCS|:

.. list-table:: Build system variables in the |NCS|
   :header-rows: 1

   * - Variable
     - Purpose
     - CMake argument to use
   * - Name of the Kconfig option
     - Set the given Kconfig option to a specific value :ref:`for a single build <configuration_temporary_change_single_build>`.
     - ``-D<name_of_Kconfig_option>=<value>``
   * - :makevar:`EXTRA_CONF_FILE`
     - Provide additional :ref:`Kconfig fragment files <configuration_permanent_change>`.
     - ``-DEXTRA_CONF_FILE=<file_name>.conf``
   * - :makevar:`EXTRA_DTC_OVERLAY_FILE`
     - Provide additional, custom :ref:`devicetree overlay files <configuring_devicetree>`.
     - ``-DEXTRA_DTC_OVERLAY_FILE=<file_name>.overlay``
   * - :makevar:`SHIELD`
     - Select one of the supported :ref:`shields <shield_names_nrf>` for building the firmware.
     - ``-DSHIELD=<shield_build_target>``
   * - :makevar:`CONF_FILE`
     - Select one of the available :ref:`build types <modifying_build_types>`, if the application or sample supports any.
     - ``-DCONF_FILE=prj_<build_type_name>.conf``
   * - ``-S`` (west) or :makevar:`SNIPPET` (CMake)
     - | Select one of the :ref:`zephyr:snippets` to add to the application firmware during the build.
       | The west argument ``-S`` is more commonly used.
     - ``-S <name_of_snippet>`` or ``-DSNIPPET=<name_of_snippet``

You can use these parameters in both the |nRFVSC| and the command line.

The build variables are applied one after another, based on the order you provide them.
This is how you can specify them:

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      You can specify the additional configuration variables when `setting up a build configuration <How to build an application_>`_:

      * :makevar:`CONF_FILE` - Select the build type in the :guilabel:`Configuration` menu.
      * :makevar:`EXTRA_CONF_FILE` - Add the Kconfig fragment file in the :guilabel:`Kconfig fragments` menu.
      * :makevar:`EXTRA_DTC_OVERLAY_FILE` - Add the devicetree overlays in the :guilabel:`Devicetree overlays` menu.
      * Other variables - Provide CMake arguments in the :guilabel:`Extra CMake arguments` field, preceded by ``--``.

      For example, to build the :ref:`location_sample` sample for the nRF9161 DK with the nRF7002 EK Wi-Fi support, select ``nrf9161dk_nrf9161_ns`` in the :guilabel:`Board` menu, :file:`overlay-nrf7002ek-wifi-scan-only.conf` in the :guilabel:`Kconfig fragments` menu, and provide ``-- -DSHIELD=nrf7002ek`` in the :guilabel:`Extra CMake arguments` field.

   .. group-tab:: Command line

      Pass the additional options to the ``west build`` command when :ref:`building`.
      The CMake arguments must be added after a ``--`` at the end of the command.

      For example, to build the :ref:`location_sample` sample for the nRF9161 DK with the nRF7002 EK Wi-Fi support, the command would look like follows:

      .. code-block::

         west build -p -b nrf9161dk_nrf9161_ns -- -DSHIELD=nrf7002ek -DEXTRA_CONF_FILE=overlay-nrf7002ek-wifi-scan-only.conf

See :ref:`configuration_permanent_change` and Zephyr's :ref:`zephyr:west-building-cmake-args` for more information.

.. _cmake_options_images:

Providing CMake options for specific images
===========================================

You can prefix the build variable names with the image name if you want the variable to be applied only to a specific image: ``-D<prefix>_<build_variable>=<file_name>``.
For example, ``-DEXTRA_CONF_FILE=external_crypto.conf`` will be applied to the default image for which you are building (most often, the application image), while ``-Dmcuboot_EXTRA_CONF_FILE=external_crypto.conf`` will be applied to the MCUboot image.

This feature is not available for setting Kconfig options.

.. _cmake_options_examples:

Examples of commands with CMake options
=======================================

See the following subsections for examples of how to provide the build system variables listed in the table above.

Examples for Kconfig options
----------------------------

Enabling a single Kconfig option
  The following example shows how you can enable a single Kconfig option (:kconfig:option:`CONFIG_BRIDGED_DEVICE_BT`) when building for a build target (``nrf7002dk_nrf5340_cpuapp``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf7002dk_nrf5340_cpuapp``
        * :guilabel:`Extra CMake arguments`: ``-- -DCONFIG_BRIDGED_DEVICE_BT=y``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_BRIDGED_DEVICE_BT=y

Setting a Kconfig option to a value
  The following example shows how you can set a Kconfig option to a specific value (:kconfig:option:`CONFIG_OPENTHREAD_DEFAULT_TX_POWER`) when building for a build target (``nrf52840dk_nrf52840``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf52840dk_nrf52840``
        * :guilabel:`Extra CMake arguments`: ``-- -DCONFIG_OPENTHREAD_DEFAULT_TX_POWER=2``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf52840dk_nrf52840 -- -DCONFIG_OPENTHREAD_DEFAULT_TX_POWER=2

Examples for EXTRA_CONF_FILE
----------------------------

Providing one Kconfig fragment file
  The following example shows how you can provide an additional Kconfig fragment file (:file:`overlay-tls-nrf91.conf`) when building for a build target (``thingy91_nrf9160_ns``).
  In addition, the example also enables a Kconfig option (:kconfig:option:`CONFIG_MQTT_HELPER_LOG_LEVEL_DBG`).

  .. tabs::

    .. group-tab:: nRF Connect for VS Code

       Use the following parameters when `setting up a build configuration <How to build an application_>`_:

       * :guilabel:`Board`: ``thingy91_nrf9160_ns``
       * :guilabel:`Extra CMake arguments`: ``-- -DEXTRA_CONF_FILE=overlay-tls-nrf91.conf -DCONFIG_MQTT_HELPER_LOG_LEVEL_DBG=y``

    .. group-tab:: Command line

       .. code-block::

          west build -b thingy91_nrf9160_ns -- -DEXTRA_CONF_FILE=overlay-tls-nrf91.conf -DCONFIG_MQTT_HELPER_LOG_LEVEL_DBG=y

Providing two or more Kconfig fragment files
  The following example shows how you can provide two additional Kconfig fragment files (:file:`overlay-802154.conf` and :file:`overlay-bt.conf`) when building for a build target (``nrf5340dk_nrf5340_cpuapp``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf5340dk_nrf5340_cpuapp``
        * :guilabel:`Extra CMake arguments`: ``-- -DEXTRA_CONF_FILE="overlay-802154.conf;overlay-bt.conf"``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf5340dk_nrf5340_cpuapp -- -DEXTRA_CONF_FILE="overlay-802154.conf;overlay-bt.conf"

Providing Kconfig fragment files for different cores
  The following example shows how you can provide two Kconfig fragment files, one for the application core (:file:`nrf5340dk_app_sr_net.conf`) and one for the network core (:file:`nrf5340dk_mcuboot_sr_net.conf`) when building for a build target (``nrf5340dk_nrf5340_cpuapp``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf5340dk_nrf5340_cpuapp``
        * :guilabel:`Extra CMake arguments`: ``-- -DEXTRA_CONF_FILE=nrf5340dk_app_sr_net.conf -Dmcuboot_EXTRA_CONF_FILE=nrf5340dk_mcuboot_sr_net.conf``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf5340dk_nrf5340_cpuapp -- -DEXTRA_CONF_FILE=nrf5340dk_app_sr_net.conf -Dmcuboot_EXTRA_CONF_FILE=nrf5340dk_mcuboot_sr_net.conf

Examples for EXTRA_DTC_OVERLAY_FILE
-----------------------------------

Providing one devicetree overlay file
  The following example shows how you can provide an additional devicetree overlay file (:file:`usb.overlay`) when building for a build target (``nrf52840dk_nrf52840``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf52840dk_nrf52840``
        * :guilabel:`Extra CMake arguments`: ``-- -DEXTRA_DTC_OVERLAY_FILE=no-dfe.overlay``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf52840dk_nrf52840 -- -DEXTRA_DTC_OVERLAY_FILE=no-dfe.overlay

Providing devicetree overlay and Kconfig fragment files
  The following example shows how you can provide an additional devicetree overlay file (:file:`usb.overlay`) and an additional Kconfig fragment file (:file:`prj_cdc.conf`) when building for a build target (``nrf52840dk_nrf52840``):

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf52840dk_nrf52840``
        * :guilabel:`Extra CMake arguments`: ``-- -DEXTRA_DTC_OVERLAY_FILE=usb.overlay -DEXTRA_CONF_FILE=prj_cdc.conf``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf52840dk_nrf52840 -- -DEXTRA_DTC_OVERLAY_FILE=usb.overlay -DEXTRA_CONF_FILE=prj_cdc.conf

Examples for SHIELD
-------------------

The following example shows how you can add shield support (``nrf7002eb``) when building for a build target (``thingy53_nrf5340_cpuapp``):

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Use the following parameters when `setting up a build configuration <How to build an application_>`_:

      * :guilabel:`Board`: ``thingy53_nrf5340_cpuapp``
      * :guilabel:`Extra CMake arguments`: ``-- -DSHIELD=nrf7002eb``

   .. group-tab:: Command line

      .. code-block::

         west build -b thingy53_nrf5340_cpuapp -- -DSHIELD=nrf7002eb

Examples for CONF_FILE
----------------------

Select the build type
  The following example shows how you can select a build type when building for a build target.
  It uses the ``prj_usb.conf`` file of the :ref:`Zigbee NCP <zigbee_ncp_sample>` sample to select the build type, and ``nrf52840dk_nrf52840`` as the build target.

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf52840dk_nrf52840``
        * :guilabel:`Extra CMake arguments`: ``-- -DCONF_FILE=prj_usb.conf``

     .. group-tab:: Command line

        .. code-block::

           west build samples/zigbee/ncp -b nrf52840dk_nrf52840 -- -DCONF_FILE=prj_usb.conf

Select the build type and provide additional parameters
  The following example for selecting a build target uses the ``prj_thread_wifi_switched.conf`` file of the :ref:`Matter door lock <matter_lock_sample>` sample to select the build type, and ``nrf5340dk_nrf5340_cpuapp`` as the build target.
  It also adds a shield support for the application core (``nrf7002ek``) and the network core (``nrf7002ek_coex``), and sets custom Kconfig option settings.

  .. tabs::

     .. group-tab:: nRF Connect for VS Code

        Use the following parameters when `setting up a build configuration <How to build an application_>`_:

        * :guilabel:`Board`: ``nrf5340dk_nrf5340_cpuapp``
        * :guilabel:`Extra CMake arguments`: ``-- -DSHIELD=nrf7002ek -Dmultiprotocol_rpmsg_SHIELD=nrf7002ek_coex -DCONF_FILE=prj_thread_wifi_switched.conf -DCONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE=y -Dmcuboot_CONFIG_UPDATEABLE_IMAGE_NUMBER=3``

     .. group-tab:: Command line

        .. code-block::

           west build -b nrf5340dk_nrf5340_cpuapp -p -- -DSHIELD=nrf7002ek -Dmultiprotocol_rpmsg_SHIELD=nrf7002ek_coex -DCONF_FILE=prj_thread_wifi_switched.conf -DCONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE=y -Dmcuboot_CONFIG_UPDATEABLE_IMAGE_NUMBER=3

Examples for SNIPPET
--------------------

The following example shows how you can apply a snippet (``nrf91-modem-trace-uart``) when building for a build target (``nrf9160dk_nrf9160``):

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      Use the following parameters when `setting up a build configuration <How to build an application_>`_:

      * :guilabel:`Board`: ``nrf9160dk_nrf9160``
      * :guilabel:`Extra CMake arguments`: ``-S nrf91-modem-trace-uart``

   .. group-tab:: Command line

      .. code-block::

         west build --board nrf9160dk_nrf9160 -S nrf91-modem-trace-uart
