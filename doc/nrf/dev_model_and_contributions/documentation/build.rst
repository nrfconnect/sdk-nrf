.. _doc_build:

Building the |NCS| documentation
################################

.. contents::
   :local:
   :depth: 2

The |NCS| documentation is written using the reStructuredText markup language (:file:`.rst` file extension) with Sphinx extensions and processed using Sphinx.
API documentation is generated from Doxygen comments.

See :ref:`zephyr:documentation-overview` in the Zephyr developer guide for information about reStructuredText.

Before you start
****************

Before you can build the documentation, you must install the required tools.
The following tool versions have been tested to work:

* Doxygen 1.9.6
* Mscgen 0.20
* PlantUML
* Python dependencies as listed in :ref:`python_req_documentation` on the Requirements page

Complete the following steps to install the required tools:

1. If you have not done so already, install the |NCS| as described in :ref:`install_ncs`.
#. Install or update all required :ref:`Python dependencies <requirements_toolchain_python_deps>`.
#. Install the additional Python dependencies for building the documentation by entering the following command in a terminal window in the :file:`ncs` folder.
   `Python virtual environment <Python virtual environments_>`_ needs to be active for the command to work:

   .. code-block:: bash

      pip install -U -r nrf/doc/requirements.txt

#. Install `Doxygen`_.
#. Install `Mscgen`_ and make sure that the ``mscgen`` executable is in your :envvar:`PATH`.
#. Install PlantUML.
   On Windows, you can install `PlantUML from chocolatey`_.

.. _doc_build_steps:

Building documentation output
*****************************

All documentation build files are located in the :file:`ncs/nrf/doc` folder.
The :file:`nrf` subfolder in that directory contains all :file:`.rst` source files that are not directly related to a sample application or a library.
Documentation for samples and libraries is provided in a :file:`README.rst` or another :file:`.rst` file in the same directory as the code.

Building the documentation output requires building the output for all documentation sets that are part of the :ref:`doc_structure`.
Since there are links from the |NCS| documentation set into other documentation sets, the documentation is built in a predefined order.

Complete the following steps to build the documentation output:

1. Open a command-line window and enter the doc folder :file:`ncs/nrf/doc`.
#. Generate the Ninja build files by entering the following command:

   .. code-block:: console

      cmake -GNinja -S. -B_build

#. Enter the generated build folder:

   .. code-block:: console

      cd _build

#. Run ninja to build the complete documentation by entering the following command:

   .. code-block:: console

      ninja

The documentation output is written to the :file:`doc/_build/html` folder.
Double-click the :file:`index.html` file to display the documentation in your browser.

Alternatively, you can work with just a single documentation set, for example, ``nrf``.
The build system provides individual targets for such a purpose.
If you have not built all documentation sets before, it is recommended to run the following command:

.. parsed-literal::

   ninja *docset-name*-all

Here, *docset-name* is the name of the documentation set, for example, ``nrf``.
This target will build the :ref:`documentation sets <documentation_sets>` that are needed for *docset-name*.

On subsequent builds, it is recommended to just run the following command:

.. parsed-literal::

   ninja *docset-name*

The last couple of targets mentioned in :ref:`documentation_sets` will only invoke the build for the corresponding documentation set (referred by *docset-name*), assuming that all of its dependencies are available.

.. _caching_and_cleaning:

Caching and cleaning
********************

To speed up the documentation build, Sphinx processes only those files that have been changed since the last build.
This mechanism can sometimes cause issues such as navigation not being updated correctly.

If you experience any of such issues, clean the build folders before you run the documentation build.

To clean all the build files:

.. code-block:: console

   ninja clean

To clean the build folders for a particular documentation set:

.. parsed-literal::

   ninja *docset-name*-clean

Here, *docset-name* is the name of the documentation set, for example, ``nrf``.

.. _testing_versions:

Testing different versions locally
**********************************

Documentation sets for different versions of the |NCS| are defined in the :file:`doc/versions.json` file.
This file is used to display the :ref:`version drop-down <doc_structure_versions>`.

To test the version drop-down locally, complete the following steps:

1. In the documentation build folder (for example, :file:`_build`), rename the :file:`html` folder to :file:`latest`.
#. Open a command-line window in the documentation build folder and enter the following command to start a Python web server::

      python -m http.server

#. Access http://localhost:8000/latest/index.html with your browser to see the documentation.

To add other versions of the documentation to your local documentation output, build the versions from a tagged release and rename the :file:`html` folder to the respective version (for example, |release_number_tt|).

Dealing with warnings
*********************

When building the documentation, all warnings are regarded as errors, so they will make the documentation build fail.

However, there are some known issues with Sphinx and Breathe that generate Sphinx warnings even though the input is valid C code.
To deal with such unavoidable warnings, Zephyr provides the Sphinx extension ``zephyr.warnings_filter`` that filters out warnings based on a set of regular expressions.
You can find the extension together with usage details at :file:`ncs/zephyr/doc/_extensions/zephyr/warnings_filter.py`.

The configuration file that defines the expected warnings for the nrf documentation set is located at :file:`ncs/nrf/doc/nrf/known-warnings.txt`.
It contains regular expressions to filter out warnings related to duplicate C declarations.
These warnings are caused by different objects (for example, a struct and a function or nested elements) sharing the same name.
