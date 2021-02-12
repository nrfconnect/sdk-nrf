.. _doc_build:

Building the |NCS| documentation
################################

.. contents::
   :local:
   :depth: 2

The |NCS| documentation is written using the reStructuredText markup language (.rst file extension) with Sphinx extensions and processed using Sphinx.
API documentation is included from Doxygen comments.

See :ref:`zephyr:documentation-overview` in the Zephyr developer guide for information about reStructuredText.


Before you start
****************

Before you can build the documentation, install the |NCS| as described in :ref:`gs_installing`.
Make sure that you have installed the required :ref:`Python dependencies <additional_deps>`.

See :ref:`zephyr:documentation-processors` in the Zephyr developer guide for information about installing the required tools to build the documentation and their supported versions.
In addition to these tools, you must install `mscgen`_ and make sure the ``mscgen`` executable is in your ``PATH``.

.. note::
   On Windows, the Sphinx executable ``sphinx-build.exe`` is placed in the :file:`Scripts` folder of your Python installation path.
   Depending on how you have installed Python, you might need to add this folder to your ``PATH`` environment variable.
   Follow the instructions in `Windows Python Path`_ if needed.


Documentation structure
***********************

All documentation build files are located in the :file:`ncs/nrf/doc` folder.
The :file:`nrf` subfolder in that directory contains all :file:`.rst` source files that are not directly related to a sample application or a library.
Documentation for samples and libraries are provided in a :file:`README.rst` or :file:`.rst` file in the same directory as the code.

Building the documentation output requires building output for all documentation sets.
Currently, there are four sets: nrf, nrfxlib, zephyr, and mcuboot (covering the contents of :file:`bootloader/mcuboot`).
Since there are links from the ncs documentation set into other documentation sets, the other documentation sets must be built first.

Building documentation output
*****************************

Complete the following steps to build the documentation output:

1. Open a shell and enter the doc folder :file:`ncs/nrf/doc`.

   * On Windows:

     a. Navigate to :file:`ncs/nrf`.
     #. Hold shift and right-click on the :file:`doc` folder.
        Select :guilabel:`Open command window here`.

   * On Linux or macOS:

     a. Open a shell window.
     #. Navigate to :file:`ncs/nrf/doc`.
        If the ncs folder is in your home directory, enter:

        .. code-block:: console

           cd ~/ncs/nrf/doc

#. Generate the Ninja build files:

   .. code-block:: console

      cmake -GNinja -B_build .

#. Enter the generated build folder:

   .. code-block:: console

      cd _build

#. Run ninja to build the documentation:

   .. code-block:: console

      ninja build-all

   This command will build all documentation sets.
   Note that this process can take quite some time.

   Alternatively, if you want to build each documentation set separately, complete the following steps:

   a. Run ninja to build the Kconfig documentation:

      .. code-block:: console

         ninja kconfig-html

   #. Run ninja to build the Zephyr documentation:

      .. code-block:: console

         ninja zephyr

      This step can take up to 15 minutes.

   #. Run ninja to build the mcuboot documentation:

      .. code-block:: console

         ninja mcuboot

   #. Run ninja to build the nrfxlib inventory file (used by nrf):

      .. code-block:: console

         ninja nrfxlib-inventory

   #. Run ninja to build the |NCS| documentation:

      .. code-block:: console

         ninja nrf

   #. Run ninja to build the nrfxlib documentation:

      .. code-block:: console

         ninja nrfxlib

The documentation output is written to the :file:`_build/html` folder.
Double-click the :file:`index.html` file to display the documentation in your browser.

.. tip::

   If you modify or add RST files, you do not need to rerun the full documentation build.
   For simple changes, it is sufficient to run the substep that builds the respective documentation (for example, only ``ninja nrf`` for changes to the |NCS| documentation).
   If this results in unexpected build errors, follow :ref:`caching_and_cleaning` and rerun ``ninja build-all``.

.. _caching_and_cleaning:

Caching and cleaning
********************

To speed up the documentation build, Sphinx processes only those files that have been changed since the last build.
In addition, RST files are copied to a different location during the build process.
This mechanism can cause outdated or deleted files to be used in the build, or the navigation to not be updated as expected.

If you experience any such problems, clean the build folders before you run the documentation build.
Note that this will cause the documentation to be built from scratch, which takes a considerable time.

To clean the build folders for the Zephyr documentation:

.. code-block:: console

   ninja clean-zephyr

To clean the build folders for the nrfxlib documentation:

.. code-block:: console

   ninja clean-nrfxlib

To clean the build folders for the MCUboot documentation:

.. code-block:: console

   ninja clean-mcuboot

To clean the build folders for the |NCS| documentation:

.. code-block:: console

   ninja clean-nrf

Out-of-tree builds
******************

Out-of-tree builds are also supported, so you can actually build from outside
the source tree:

.. code-block:: console

   # On Linux/macOS
   cd ~
   source ncs/zephyr/zephyr-env.sh
   cd ~
   mkdir build
   # On Windows
   cd %userprofile%
   ncs\zephyr\zephyr-env.cmd
   mkdir build

   # Use cmake to configure a Ninja-based build system:
   cmake -GNinja -Bbuild/ -Hncs/nrf/doc
   # Now run ninja on the generated build system:
   ninja -C build/ zephyr
   ninja -C build/ mcuboot
   ninja -C build/ nrfxlib-inventory
   ninja -C build/ nrf
   ninja -C build/ nrfxlib
   # If you modify or add .rst files in the nRF repository, run ninja again:
   ninja -C build/ nrf

If you want to build the documentation from scratch, delete the contents of the build folder and run ``cmake`` and then ``ninja`` again.

Different versions
******************

Documentation sets for different versions of the |NCS| are defined in the :file:`doc/versions.json` file.
This file is used to display the version drop-down in the top-left corner of the documentation.

The version drop-down is displayed only if the documentation files are organized in the required folder structure and the documentation is hosted on a web server.
To test the version drop-down locally, complete the following steps:

1. In the documentation build folder (for example, :file:`_build`), rename the :file:`html` folder to :file:`latest`.
#. Open a command window in the documentation build folder and enter the following command to start a Python web server::

      python -m http.server

#. Access http://localhost:8000/latest/index.html with your browser to see the documentation.

To add other versions of the documentation to your local documentation output, build the versions from a tagged release and rename the :file:`html` folder to the respective version (for example, |release_number_tt|).
