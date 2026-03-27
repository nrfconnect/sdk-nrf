.. _snippet-s0-partition:

s0 partition snippet (s0-partition)
###################################

.. code-block:: console

   west build -S s0-partition [...]

Overview
********

This snippet changes the chosen flash partition to be the ``s0_partition`` node, this can be
used to build for the primary slot for b0.

Requirements
************

Partition mapping correct setup in devicetree.
