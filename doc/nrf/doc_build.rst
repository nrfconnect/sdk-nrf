.. _doc_build:

Building the |NCS| documentation
################################

The |NCS| documentation is written using the reStructuredText markup
language (.rst file extension) with Sphinx extensions and processed
using Sphinx.
API documentation is included from Doxygen comments.

See the *Documentation overview* section in the :ref:`zephyr:zephyr_doc` developer guide for information about reStructuredText.


Before you start
****************

Before you can build the documentation, install the |NCS| as described in :ref:`gs_installing`.
Make sure that you have installed the required :ref:`Python dependencies <additional_deps_win>`.

See the *Installing the documentation processors* section in the :ref:`zephyr:zephyr_doc` developer guide for information about installing the required tools to build the documentation and their supported versions.

.. note::
   On Windows, the Sphinx executable ``sphinx-build.exe`` is placed in
   the ``Scripts`` folder of your Python installation path.
   Dependending on how you have installed Python, you may need to
   add this folder to your ``PATH`` environment variable. Follow
   the instructions in `Windows Python Path`_ to add those if needed.


Documentation structure
***********************

All documentation build files are located in the ``ncs/nrf/doc`` folder.
The ``nrf`` subfolder in that directory contains all .rst source files that are not directly related to a sample application or a library.
Documentation for samples and libraries are provided in a ``README.rst`` or ``*.rst`` file in the same directory as the code.

Building the documentation output requires building output for all documentation sets.
Currently, there are four sets: nrf, nrfxlib, zephyr, and mcuboot (covering the contents of :file:`bootloader/mcuboot`).
Since there are links from the ncs documentation set into other documentation sets, the other documentation sets must be built first.

Building documentation output
*****************************

Complete the following steps to build the documentation output:

1. Create the build folder ``ncs/nrf/doc/_build``.

   * On Windows:

     a. Navigate to ``ncs/nrf/doc``.
     #. Right-click to create a folder and name it ``_build``.
     #. Hold shift and right-click on the new folder.
        Select **Open command window here**.

   * On Linux or macOS:

     a. Open a shell window.
     #. Navigate to ``ncs/nrf/doc``.
        If the ncs folder is in your home directory, enter:

        .. code-block:: console

           cd ~/ncs/nrf/doc

     #. Create a folder named ``_build`` and enter it:

        .. code-block:: console

           mkdir -p _build && cd _build

#. Load the environment setting for Zephyr builds.

   * On Windows:

        .. code-block:: console

           ..\..\..\zephyr\zephyr-env.cmd
   * On Linux or macOS:

        .. code-block:: console

           source ../../../zephyr/zephyr-env.sh

#. Generate the Ninja build files:

        .. code-block:: console

           cmake -GNinja ..

#. Run ninja to build the Zephyr documentation:

        .. code-block:: console

           ninja zephyr

   This step can take up to 15 minutes.
#. Run ninja to build the nrfxlib documentation:

        .. code-block:: console

           ninja nrfxlib

#. Run ninja to build the mcuboot documentation:

        .. code-block:: console

           ninja mcuboot

#. Run ninja to build the |NCS| documentation:

        .. code-block:: console

           ninja nrf

The documentation output is written to ``_build\html``. Double-click the ``index.html`` file to display the documentation in your browser.

.. tip::
   If you modify or add RST files, you only need to rerun the steps that build the respective documentation: step 4 (if you modified the Zephyr documentation), step 5 (if you modified the nrfxlib documentation), step 6 (if you modified the MCUboot documentation), or step 7 (if you modified the |NCS| documentation).

   If you open up a new command prompt, you must repeat step 2.

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
   ninja -C build/ nrfxlib
   ninja -C build/ mcuboot
   ninja -C build/ nrf
   # If you modify or add .rst files in the nRF repository, run ninja again:
   ninja -C build/ nrf

If you want to build the documentation from scratch just delete the contents
of the build folder and run ``cmake`` and then ``ninja`` again.

.. _Windows Python Path: https://docs.python.org/3/using/windows.html#finding-the-python-executable
