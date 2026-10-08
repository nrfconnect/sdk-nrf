.. _ncs_release_model:

Release model
#############

The |NCS| release model describes how releases are structured, documented, and maintained.

Every |NCS| release consists of a combination of :ref:`ncs_git_intro` repositories at different versions and revisions, managed together by :ref:`ncs_west_intro`.
The revision of each of those repositories is determined by the current revision of the main (or :ref:`manifest <zephyr:west-manifests>`) repository, `sdk-nrf`_.

For each version of the |NCS|, Nordic Semiconductor provides a dedicated toolchain.
The |NCS| :term:`toolchain` includes the Zephyr SDK and then adds tools and modules required to build |NCS| samples and applications on top of it.
These include the :ref:`required SDK tools <requirements_toolchain_tools>`, the :ref:`Python dependencies <requirements_toolchain_python_deps>`, and the `GN tool`_ for creating :ref:`ug_matter` applications.
You can check the versions of the required tools and Python dependencies on the :ref:`Requirements reference page <requirements_toolchain>`.

.. note::
   Unless you are familiar with the :ref:`development process <dev-model>`, you should always work with a specific, stable release of the |NCS|.

For more information, see the following pages:

* About the repository and development model, see the :ref:`dm_code_base` page.
* About version strings, see :ref:`dm-revisions`.
* About Git tags, see :ref:`dm_revisions_git_tags`.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ncs_release_model/standard_releases
   ncs_release_model/lts_releases
