.. _dm_managing_code:

Managing the code base
######################

.. contents::
   :local:
   :depth: 2


To obtain and manage the |NCS| code base, you must use a combination of `Git`_ and :ref:`west<zephyr:west>`.

As described in :ref:`dm_code_base`, the |NCS| contains the following repositories:

* The manifest repository, `sdk-nrf`_.
  This repository is managed by the user using Git exclusively, since west will not modify or update it in any way.
  The exception to this is the ``west init`` command, which can clone the manifest repository automatically at an arbitrary revision.

* The west projects.
  Those are listed in the manifest repository's :file:`west.yml` manifest file.
  They are entirely managed by west, which will clone them or check out a specific revision of them every time you run ``west update``.


.. _dm-wf-get-ncs:

Obtaining a copy of the |NCS|
*****************************

To obtain a fresh copy of the |NCS| at revision ``{revision}`` and place it in a folder named :file:`ncs`, use the following commands::

  west init -m https://github.com/nrfconnect/sdk-nrf --mr {revision} ncs
  cd ncs
  west update

Replace ``{revision}`` with any revision you wish to obtain.
This can be ``main`` if you want the latest state, or any released version (for example, |release_tt|).
If you omit the ``--mr`` parameter, west defaults to ``main``.

This is the procedure used for :ref:`getting the nRF Connect SDK code <cloning_the_repositories>` when :ref:`manual_installation`.

.. _dm-wf-update-ncs:

Updating a copy of the |NCS|
****************************

If you already have a copy of the |NCS| and wish to update it or switch to a new revision, then you only need to do the following::

  cd ncs/nrf
  git fetch {remote}
  # Check out the latest main branch
  git checkout {remote}/main
  # or check out a release
  git checkout {revision}
  west update

Where ``{remote}`` is the Git remote that points to the official Nordic repository.
This is called ``origin`` by default for the `sdk-nrf`_ repository and ``ncs`` for most others, but :ref:`may have another name <dm-wf-fork>`.
You can use ``git remote -v`` to list all your remotes.

This is the procedure used for :ref:`updating_repos` after :ref:`gs_installing`.
However, using ``git checkout`` is one of multiple ways of achieving this.
Git offers several commands and mechanisms to set the current working copy of a repository to a particular revision.
Depending on how you manage the branches of your local clone of the `sdk-nrf`_ repository, you can also replace the use of ``git checkout`` with, among many others::

  # If you have no changes of your own
  git reset --hard {remote}/main
  git reset --hard {revision}
  # If you have changes of your own
  git rebase {remote}/main
  git rebase {revision}

Describing the exact differences between the commands above is outside the scope of this section.
Refer to the publicly available `Git`_ documentation.

.. _dm-wf-fork:

Forking a repository of the |NCS|
*********************************

In some cases, you might want to keep a :term:`Soft fork` of one or more repositories that are part of the |NCS|.
The procedure to achieve that is the same regardless of whether you fork the manifest repository and/or one or more project repositories.

There are two similar but slightly different meanings to the term "fork", as described in the :ref:`glossary`:

* A fork in general terms is a server-hosted copy of an upstream repository with a few downstream changes on top of it.
  It can be hosted on GitHub or elsewhere.
* A `GitHub fork`_ is GitHub's mechanism to copy an existing repository and then send Pull Requests from it to the upstream repository.

A GitHub fork can be used to send Pull Requests and to act as a regular long-lived fork in general terms.
You can also create standard forks with GitHub by just creating an empty repository first and then initializing it with the contents of the upstream repository you wish to fork.

.. note::
   About Git remotes: The default name for a remote is ``origin`` but you can pick any arbitrary name for a remote.
   By convention, the following remote names are typically used:

   * ``origin`` usually points to the user's personal copy of the repository.
   * ``ncs`` is used to point to the |NCS| repository.
   * ``upstream`` typically points to the upstream repository, when applicable.

   The ``west init`` command creates a remote named ``origin`` that points to the original location of the cloned manifest repository.
   The ``west update`` command, on the other hand, uses the ``remote:`` property in the :file:`west.yml` file to name the remote pointing to the original location.

If you want to create a `GitHub fork`_ follow the steps below:

#. Create a `GitHub fork`_ using the :guilabel:`Fork` button in the GitHub user interface.
#. Add the newly created remote repository as a Git remote::

     cd ncs/{folder_path}
     # Rename the default remote from 'origin' to 'ncs', if required
     git remote rename origin ncs
     git remote add origin https://github.com/{username}/{repo}.git

   For example, to create a fork of the `sdk-nrf`_ repository for GitHub user ``foo``::

     cd ncs/nrf
     # The manifest repository defaults to a remote named 'origin'
     git remote rename origin ncs
     git remote add origin https://github.com/foo/sdk-nrf.git

   If you were to fork an OSS repository instead, which itself is already a fork of the original upstream project::

     cd ncs/zephyr
     # No need to rename the remote, since it will already be named 'ncs'
     git remote add origin https://github.com/foo/sdk-zephyr.git
     git remote add upstream https://github.com/zephyrproject-rtos/zephyr.git

  That way you would actually have three remotes, each pointing to the relevant copy of the Zephyr codebase:

  * ``origin`` pointing to your own fork of ``sdk-zephyr``.
  * ``ncs`` pointing to the |NCS| `sdk-zephyr`_.
  * ``upstream`` pointing to the upstream `official Zephyr repository`_.

To create a regular fork, follow the exact same steps as above, but the actual repository must be created by you beforehand, instead of clicking :guilabel:`Fork` in GitHub.
Also, since a GitHub fork automatically initializes the forked repository with the exact same contents as the original one, you must push the contents yourself::

  cd ncs/{folder_path}
  # Rename the default remote from 'origin' to 'ncs'
  git remote rename origin ncs
  git remote add origin https://github.com/{username}/{repo}.git
  git push origin main
