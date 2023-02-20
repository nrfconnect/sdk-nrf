.. _dm_code_base:

nRF Connect SDK code base
#########################

.. contents::
   :local:
   :depth: 2

The source code, libraries, and tools that compose the |NCS| are entirely hosted in a set of `Git`_ repositories.
Basic familiarity with Git is required to understand the architecture of the repository set and to work with the |NCS|.

All |NCS| repositories are publicly hosted on the `nrfconnect GitHub organization`_, and accessible to both individual users and companies.

.. _dm_repo_types:

Repository types
****************

There are two main types of Git repositories in the |NCS| repository set:

* nRF repositories

  * Created, developed, and maintained by Nordic.
  * Usually licensed for use on Nordic products only.

* OSS repositories

  * Created and maintained by Nordic.
  * Soft forks of open-source projects.
  * Typically contain a small set of changes that are specific to |NCS|.
  * Updated ("upmerged") regularly with the latest changes from the open source project.

nRF repositories are stand-alone and have no upstreams, since they are unique to the |NCS|.
Some examples of repositories of this type are:

* `sdk-nrf`_: The main repository for Nordic-developed software.
* `sdk-nrfxlib`_: A repository containing linkable libraries developed by Nordic.

OSS repositories, on the other hand, are typically soft forks of an upstream open source project, which Nordic maintains in order to keep a small set of changes that do not belong, or have not been merged, to the upstream official open-source repository.
For example:

* `sdk-zephyr`_ is a soft fork (and therefore a downstream) of the upstream official `Zephyr repository`_.
* `sdk-mcuboot`_ is a soft fork (and therefore a downstream) of the upstream official `MCUboot repository`_.

Repository structure
********************

The |NCS| uses :ref:`west <zephyr:west>` to manage the combination of repositories and versions.

A :ref:`manifest repository <zephyr:west-manifests>`, `sdk-nrf`_, contains a file in its root folder, :file:`west.yml`, which lists all other repositories (west projects) included in the |NCS|.
The |NCS| repository structure has a star topology, with the `sdk-nrf`_ repository being the center of the star and all other repositories being west projects that are managed by :file:`west.yml`.
This is equivalent to topology T2 in the :ref:`west documentation <zephyr:west-basics>`.

.. figure:: images/ncs-west-repos.svg
   :alt: A graphical depiction of the |NCS| repository structure

   The |NCS| repository structure

The figure above depicts the |NCS| repository structure.
A central concept with this repository structure is that each revision (in Git terms) of the `sdk-nrf`_ repository completely determines the revisions of all other
repositories (that is, the west projects).
This means that the linear Git history of this manifest repository also determines the history of the repository set in its entirety, thanks to the :file:`west.yml` `west manifest file`_ being part of the manifest repository.
West reads the contents of the manifest file to find out which revisions of the project repositories are to be checked out every time ``west update`` is run.
In this way, you can decide to work with a specific |NCS| release either by initializing a new west installation at a particular tag or by checking out the corresponding tag for a release in an existing installation and then updating your project repositories to the corresponding state with ``west update``.
Alternatively, you can work with the latest state of development by using the main branch of the `sdk-nrf`_ repository, updating it with Git regularly and using ``west update`` to update the project repositories every time the manifest repository changes.
More information about manifests can be found in the :ref:`west manifest section <zephyr:west-manifests>` of the Zephyr documentation.

.. _dm-revisions:

Revisions
*********

There are two fundamental revisions that are relevant to most |NCS| users:

* The ``main`` branch of the `sdk-nrf`_ repository
* Any Git tag (that is, release, release candidate or development tag) of the `sdk-nrf`_ repository

As discussed above, the revision of the manifest repository, `sdk-nrf`_, uniquely determines the revisions of all other repositories, so a discussion about |NCS| revisions can be essentially limited to the manifest repository revision.

The ``main`` branch of the `sdk-nrf`_ repository always contains the latest development state of the |NCS|.
Since all development is done openly, you can use it if you are not particularly concerned about stability and want to track the latest changes that are being merged continuously into the different repositories.

The Git tags correspond to official releases tested and signed by the Nordic engineers.
The format for nRF repositories is as follows::

  vX.Y.Z(-rcN)

Where X, Y, and Z are the major, minor, and patch version respectively and, optionally, a release candidate postfix ``-rcN`` is attached if the tag identifies a candidate instead of the actual release.

The Git tags are composed as follows::

  vX.Y.Z(-rcN|-devN)

X, Y, and Z are the major, minor, and patch version, respectively.

A special value of ``99`` for the patch version number is reserved for any revision in between releases.
Tags without a suffix correspond to official releases tested and signed by Nordic Semiconductor engineers.
A release candidate suffix ``-rcN`` is attached if the tag identifies a candidate instead of the actual release.

.. dev_tag_definition_start

In between releases, there might be development tags.
These are identified by a ``-devN`` suffix included after the |NCS| version, for example ``v1.9.99-dev1`` for the main branch or ``v1.9.2-dev1`` for the release branch.
Such versions indicate a snapshot of the development state that is created to highlight the introduction of a new piece of functionality.
Unlike an |NCS| release, a ``-devN`` version may or may not have received the same amount of testing.

.. dev_tag_definition_end

In the case of OSS repositories, the git tag format reuses the upstream project's version along with some |NCS| specific postfixes::

  vX.Y.Z-ncsN(-rcM)

In this format, X, Y and Z are the major, minor and patch versions of the upstream project, and ``-ncsN`` is used to identify the number of |NCS| releases based on that particular version.

.. _dm-oss-downstreams:

OSS repositories downstream project history
*******************************************

As described in :ref:`dm_repo_types`, the |NCS| contains OSS repositories, which are based on third-party, open-source Git repositories and may contain additional patches not present upstream.
Examples include `sdk-zephyr`_ and `sdk-mcuboot`_, which have upstream open-source projects used as a basis for downstream repositories distributed with the |NCS|.
This section describes how the history of these OSS repositories is maintained, and how they are synchronized with their upstreams.

The short logs for these downstream patches contain ``[nrf xyz]`` at the beginning, for different ``xyz`` strings.
This makes their different purposes downstream clearer, and makes them easier to search for and see in ``git log``.
The current values of ``[nrf xyz]`` are:

* ``[nrf mergeup]``: periodic merges of the upstream tree
* ``[nrf fromlist]``: patches which have upstream pull requests, including any later revisions
* ``[nrf noup]``: patches which are specific to the |NCS|
* ``[nrf fromtree]``: patches which have been cherry-picked from an upstream tree

.. note::
    The downstream project history is periodically rewritten.
    This is important to prevent the number of downstream patches included in a specific |NCS| release from increasing forever.
    A repository's history is typically only rewritten once for every |NCS| release.

To make incorporating new history into your own forks easier, a new point in the downstream |NCS| history is always created which has an empty ``git diff`` with the previous version.
The empty diff means you can always use:

* ``git merge`` to get the rewritten history merged into your own fork without errors
* ``git rebase --onto`` or ``git cherry-pick`` to reapply any of your own patches cleanly before and after the history rewrite
* ``git cherry`` to list any additional patches you may have applied to these projects to rewrite history as needed

Additionally, both the old and new histories are committed sequentially into the ``revision`` fields for these projects in the :file:`nrf/west.yml` west
manifest file.
This means you can always combine ``git bisect`` in the ``nrf`` repository with ``west update`` at each bisection point to diagnose regressions and the rest.
