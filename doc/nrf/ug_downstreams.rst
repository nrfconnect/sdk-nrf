.. _downstreams:

Downstream Project History
--------------------------

The |NCS| contains third party open source Git repositories which may contain
additional patches not present upstream. Examples include :ref:`zephyr` and
:ref:`about_mcuboot`, which have upstream open source projects used as a basis
for downstream repositories distributed with the |NCS|.

The short logs for these downstream patches contain ``[nrf xyz]`` at the
beginning, for different ``xyz`` strings. This makes their different purposes
downstream clearer, and makes them easier to search for and see in ``git
log``. The current values of ``[nrf xyz]`` are:

- ``[nrf mergeup]``: periodic merges of the upstream tree
- ``[nrf fromlist]``: patches which have upstream pull requests, including any
  later revisions
- ``[nrf toup]``: patches which Nordic developers intend to submit upstream
  later
- ``[nrf noup]``: patches which are specific to the |NCS|
- ``[nrf temphack]``: temporary patches with some known issues
- ``[nrf fromtree]``: patches which have been cherry-picked from an upstream
  tree

It's important to note that the **downstream project history is periodically
rewritten**. This is important to prevent the number of downstream patches
included in a specific NCS release from increasing forever. A repository's
history is typically only rewritten once every major |NCS| release.

To make incorporating new history into your own forks easier, a new point in
the downstream |NCS| history is always created which has an empty ``git diff``
with the previous version. The empty diff means you can always use:

- ``git merge`` to get the rewritten history merged into your own fork
  without errors
- ``git rebase --onto`` or ``git cherry-pick`` to reapply any of your own
  patches cleanly before and after the history rewrite
- ``git cherry`` to list any additional patches you may have applied to these
  projects to rewrite history as needed

Additionally, both the old and new histories are commited sequentially into the
``revision`` fields for these projects in the :file:`nrf/west.yml` west
manifest file. This means you can always combine ``git bisect`` in the ``nrf``
repository with ``west update`` at each bisection point to diagnose
regressions, etc.
