.. _ncs_doc:

nRF Connect SDK documentation Generation
########################################

These instructions will walk you through generating the nRF Connect SDK's
documentation on your local system using the same documentation sources
as we use to create the online documentation found at TBD.

Documentation overview
**********************

nRF Connect SDK content is written using the reStructuredText markup
language (.rst file extension) with Sphinx extensions, and processed
using Sphinx to create a formatted stand-alone website. Developers can
view this content either in its raw form as .rst markup files, or you
can generate the HTML content and view it with a web browser directly on
your workstation. This same .rst content is also fed into the nRF Connect SDK's
public website documentation area (with a different theme applied).

You can read details about `reStructuredText`_, and `Sphinx`_ from
their respective websites.

The project's documentation contains the following items:

* ReStructuredText source files used to generate documentation found at the
  TBD website. Most of the reStructuredText sources
  are found in the ``/doc/nrf`` directory, but others are stored within the
  code source tree near their specific component (such as ``/samples`` and
  ``/boards``)

* Doxygen-generated material used to create all API-specific documents
  also found at TBD

* Script-generated material for kernel configuration options based on Kconfig
  files found in the source code tree

The reStructuredText files are processed by the Sphinx documentation system,
and make use of the breathe extension for including the doxygen-generated API
material.  Additional tools are required to generate the
documentation locally, as described in the following sections.

Installing the documentation processors
***************************************

Our documentation processing has been tested to run with:

* Doxygen version 1.8.13
* Sphinx version 1.7.5
* Breathe version 4.9.1
* docutils version 0.14
* sphinx_rtd_theme version 0.4.0

In order to install the documentation tools, clone a copy of the git
repositories for the nRF Connect SDK project and set up your development
environment as described in :ref:`getting_started`. This will ensure all the
required tools are installed on your system.

.. note::
   On Windows, the Sphinx executable ``sphinx-build.exe`` is placed in
   the ``Scripts`` folder of your Python installation path.
   Dependending on how you have installed Python, you may need to
   add this folder to your ``PATH`` environment variable. Follow
   the instructions in `Windows Python Path`_ to add those if needed.

Documentation presentation theme
********************************

Sphinx supports easy customization of the generated documentation
appearance through the use of themes.  Replace the theme files and rebuild the
docs and the output layout and style is changed.
The ``read-the-docs`` theme is installed as part of the
``requirements.txt`` list above, and will be used if it's available, for
local doc generation.

Running the documentation processors
************************************

The ``/doc`` directory in your cloned copy of the nRF Connect SDK project git
repos has all the .rst source files, extra tools, and CMake file for
generating a local copy of the nRF Connect SDK's technical documentation.
Assuming the local nRF Connect SDK copy is in a folder ``ncs/nrf`` in your home
folder, here are the commands to generate the html content locally:

.. code-block:: console

   # On Linux/macOS
   cd ~/ncs/nrf
   source ../zephyr/zephyr-env.sh
   mkdir -p doc/_build && cd doc/_build
   # On Windows
   cd %userprofile%\ncs\nrf
   ..\zephyr\zephyr-env.cmd
   mkdir doc\_build && cd doc/_build

   # Use cmake to configure a Ninja-based build system:
   cmake -GNinja ..
   # Now run ninja to build the Zephyr documentation:
   ninja zephyr
   # And then run ninja to build the nRF documentation:
   ninja nrf
   # If you modify or add .rst files in the nRF repository, run ninja again:
   ninja nrf

Depending on your development system, it will take up to 15 minutes to
collect and generate the HTML content. When done, you can view the HTML
output with your browser started at ``doc/_build/nrf/html/index.html``

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
   ninja -C build/ nrf
   # If you modify or add .rst files in the nRF repository, run ninja again:
   ninja -C build/ nrf

If you want to build the documentation from scratch just delete the contents
of the build folder and run ``cmake`` and then ``ninja`` again.

.. _reStructuredText: http://sphinx-doc.org/rest.html
.. _Sphinx: http://sphinx-doc.org/
