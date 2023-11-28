.. _contributions_ncs:

Contribution guidelines in the |NCS|
####################################

.. contents::
   :local:
   :depth: 2

Any individual or company can choose to contribute to the |NCS| codebase.
Contributions are welcome but entirely optional, as the :ref:`licenses <dm_licenses>` used by the |NCS| allow for free modification of the source code without a requirement to contribute the changes back.

The |NCS| contribution guidelines are modelled on :ref:`those adopted in Zephyr <zephyr:contribute_guidelines>`.
The following table lists the major contribution guidelines and summarizes the similarities and differences between Zephyr and the |NCS|.

.. note::
    If you want to contribute to the Zephyr codebase, create a PR upstream in the `official Zephyr repository`_ instead of a PR that targets the `sdk-zephyr`_ :term:`downstream fork` repository.

.. list-table:: Contribution guidelines in Zephyr and the |NCS|
   :widths: auto
   :header-rows: 1

   * - :ref:`zephyr:contribute_guidelines` in Zephyr
     - Description
     - Differences with the |NCS|
   * - Check :ref:`Licensing <zephyr:licensing_requirements>`
     - Apache 2.0 in Zephyr.
     - `LicenseRef-Nordic-5-Clause <LICENSE file_>`_ in the |NCS|.
   * - Respect :ref:`Copyright notices <zephyr:copyrights>`
     - Recognize the ownership of the original work according to the Linux Foundation's `Community Best Practice for Copyright Notice`_.
     - Zephyr guidelines valid in the |NCS|.
   * - Include :ref:`Developer Certification of Origin <zephyr:DCO>`
     - The DCO is required for every contribution made by every developer.
     - Zephyr guidelines valid in the |NCS|.
   * - Gain familiarity with :ref:`development environment and tools <zephyr:developing_with_zephyr>`
     - | The environment should be set up as recommended in the official documentation and you should use tools that are supported and compatible with the project, such as Git and CMake.
       | You also need to create an account on `GitHub`_.
     - The |NCS| also requires familiarity with the :ref:`requirements_toolchain` and :ref:`app_build_additions` to the configuration and build system.
   * - Gain familiarity with source tree structure
     - In Zephyr, this means respecting the :ref:`zephyr:source_tree_v2` and the purpose of main files and directories in order to avoid redundancy.
     - While the guidelines are valid for the |NCS|, the SDK uses a different :ref:`repository organization <dm_code_base>`.
   * - Check issues before contributing
     - When contributing, check the existing pull requests and issues in the respective repository in order to avoid redundancy.
     - In the |NCS|, verify also the existing :ref:`known_issues`.
   * - Run local scripts before contributing
     - :ref:`Run a series of local scripts <zephyr:Continuous Integration>` to verify the code changes that are to be contributed.
     - Zephyr guidelines valid in the |NCS|.
   * - Respect the coding style
     - Follow a series of :ref:`coding style recommendations <zephyr:coding_style>` and :ref:`zephyr:coding_guidelines`.
     - Zephyr guidelines valid in the |NCS|.
   * - Document changes to code and API
     - Follow :ref:`zephyr:doc_guidelines` and instructions for :ref:`testing changes to documentation locally <zephyr:zephyr_doc>`.
     - In the |NCS|, documentation guidelines are more detailed than in Zephyr, as described in :ref:`documentation`.
   * - Make small, controlled changes
     - Follow a :ref:`zephyr:Contribution workflow` based on small, incremental changes. See also :ref:`zephyr:contributor-expectations`.
     - Zephyr guidelines valid in the |NCS|.
   * - Write clear commit messages
     - See Zephyr's :ref:`zephyr:commit-guidelines` for more information.
     - Zephyr guidelines valid in the |NCS|.
