.. _building:
.. _gs_modifying:
.. _configure_application:

Configuring and building an application
#######################################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

After you have :ref:`created an application <create_application>`, you need to build it in order to be able to program it.
Just as for creating the application, you can build the application using either the |nRFVSC| or the command line.

.. note::
    |application_sample_long_path_windows|

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      For instructions about building with the |nRFVSC|, see `How to build an application`_ in the extension documentation.

      .. note::
          By default, the extension runs both stages of the CMake build (:ref:`configuration phase and building phase <app_build_system>`).
          If you want to only set up the build configuration without building it, make sure the :guilabel:`Build after generating configuration` is not selected.

      If you want to build with custom options or scripts, read about `Binding custom tasks to actions`_ in the extension documentation.

      .. note::
          |ncs_oot_sample_note|

      |output_files_note|

   .. group-tab:: Command line

      Complete the following steps to build on the command line:

      1. Open a terminal window.
      #. Go to the specific application directory.

         For example, if you want to build the :ref:`at_client_sample` sample, run the following command to navigate to its directory:

         .. code-block:: console

            cd nrf/samples/cellular/at_client

      #. Build the application by using the following west command with the *build_target* specified:

         .. parsed-literal::
            :class: highlight

            west build -b *build_target*

         See :ref:`programming_board_names` for more information on the supported boards and build targets.
         The board targets supported for a given application are always listed in its requirements section.

         After running the ``west build`` command, the build files can be found in :file:`build/zephyr`.
         |output_files_note|
         For more information on the contents of the build directory, see :ref:`zephyr:build-directory-contents` in the Zephyr documentation.

       For more information on building using the command line, see :ref:`Building <zephyr:west-building>` in the Zephyr documentation.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   cmake/index
   hardware/index
   kconfig/index
   advanced_building
   output_build_files

.. |output_files_note| replace:: For more information about files generated as output of the build process, see :ref:`app_build_output_files`.
