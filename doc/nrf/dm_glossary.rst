.. _dm-glossary:

Glossary
########

.. contents::
   :local:
   :depth: 2

Repository
   A Git repository in its strict sense, the highest granularity allowed by the Git version control system.

Manifest repository
   A repository that contains a :file:`west.yml` file in its root folder and can therefore act as center of a repository star topology.

West project
   Any of the listed repositories inside the :file:`west.yml` file in a manifest repository.

Contribution
   A change to the codebase sent to a remote repository for inclusion.

Upmerge
   The act of updating a downstream repository with a new revision of its upstream counterpart.

Clone
   A local copy of a remote Git repository obtained with ``git clone``.

Fork
   A server-hosted copy of a repository (upstream) that intends to follow the changes made in the original repository as time goes by, while at the same time keeping some other changes unique to it.

Soft fork
   A fork that contains a very small set of changes when compared to its upstream.

GitHub fork
   A `GitHub fork`_ is a copy of a repository inside GitHub, that allows the user to create a Pull Request.

Upstream
   The repository from which a downstream is forked off.

Downstream
   The repository that is forked off an upstream.

nRF repository
   An |NCS| repository that does not have an externally maintained, open-source upstream.
   It is exclusive to Nordic development.

OSS repository
   An |NCS| repository that tracks an upstream Open Source Software counterpart that is externally maintained.

Commit
   A Git commit, including a unique SHA and a commit message.

Patch
   See Commit.

Commit tag
   A tag prepended to the first line of the commit message to ease filtering and identification of particular commit types.

Pull Request
   A GitHub Pull Request, a set of commits that are sent for code review using GitHub.
