.. _releases_and_maturity:

Releases and maturity
#####################

The |NCS| receives regular releases, which introduce new components or fix issues in existing features.
Every release consists of a combination of all included repositories at different revisions.

The versioning scheme adopted is similar to `Semantic versioning`_, but with important semantic differences:

* Every release of the |NCS| is identified with a version string in the ``MAJOR.MINOR.PATCH`` format.
* Between two releases, ``99`` is added in place of ``PATCH`` to indicate ongoing development.
* When a new functionality is introduced in the development state, ``devN`` postfix can be added at the end of the version number.

To learn more about the versioning and the release criteria of the |NCS|, read :ref:`dm-revisions`.

Each new release comes with its own :ref:`release_notes` and can also change the :ref:`software_maturity` of the existing components.

If an issue is found in a release after it has taken place, those issues are listed on the :ref:`known_issues` page.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   releases_and_maturity/dev_model
   releases_and_maturity/release_notes
   releases_and_maturity/repository_revisions
   releases_and_maturity/software_maturity
   releases_and_maturity/known_issues

Migration guides
****************

The |NCS| provides migration guides for all major and minor releases, in order to assist user's transition from the previous release.

.. toctree::
   :maxdepth: 1
   :glob:
   :reversed:

   releases_and_maturity/migration/*
