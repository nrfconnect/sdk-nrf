.. _getting_started:
.. _installation:

Installation
############

To set up the |NCS|, follow the :ref:`installation instructions <install_ncs>` for your preferred development environment: |VSC| or command line.

When you install the |NCS|, you set up the following SDK structure of inter-connected :ref:`ncs_git_intro` repositories that are part of the `nrfconnect GitHub organization`_:

.. figure:: images/ncs_repo_structure_simplified.svg
   :alt: The simplified |NCS| repository structure

   The simplified |NCS| repository structure

These repositories include software developed by Nordic Semiconductor and open source projects.
Their versions form the SDK's code base and are managed using Zephyr's :ref:`west <ncs_west_intro>` tool.

All repositories with the prefix ``sdk`` contain the |NCS| firmware and code.
You can read more about repository types and what they include in the :ref:`dm_code_base` section of the documentation.

Each repository has a revision, which is determined by the current revision of the main repository `sdk-nrf`_.
This repository is the manifest repository, because it contains the SDK's :ref:`west manifest file <zephyr:west-manifests>` (`see the file in the repository <west manifest file_>`_) that lists all the SDK's repositories and their revisions (which is also listed on the :ref:`repos_and_revs` page).

.. note::
   If you want to go through a dedicated training to familiarize yourself with the basic functionalities of the |NCS|, enroll in the `nRF Connect SDK Fundamentals course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   installation/recommended_versions
   installation/install_ncs
   installation/updating
