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

Begin by cloning a copy of the git repository for the nRF Connect SDK and
setting up your development environment as described in :ref:`getting_started`.

Other than ``doxygen``, the documentation tools should be installed
using ``pip3`` (as documented in the development environment set up
instructions).

The documentation generation tools are included in the set of tools
expected for the Zephyr build environment and so are included in
``zephyr/scripts/requirements.txt``

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
   mkdir doc\_build & cd doc/_build

   # Use cmake to configure a Ninja-based build system:
   cmake -GNinja ..
   # Now run ninja on the generated build system:
   ninja nrf

Depending on your development system, it will take about 15 minutes to
collect and generate the HTML content.  When done, you can view the HTML
output with your browser started at ``doc/_build/nrf/html/index.html``

If you want to build the documentation from scratch you can use this command:

.. code-block:: console

   ninja nrf-pristine

.. _reStructuredText: http://sphinx-doc.org/rest.html
.. _Sphinx: http://sphinx-doc.org/
