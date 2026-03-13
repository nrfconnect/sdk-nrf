.. _snippet-s1-partition:

s1 partition snippet (s1-partition)
###################################

.. code-block:: console

   west build -S s1-partition [...]

Overview
********

This snippet changes the chosen flash partition to be the ``s1_partition`` node, this can be
used to build for the alternate slot for b0.

Requirements
************

Partition mapping correct setup in devicetree, for example:

.. literalinclude:: ../../dts/vendor/nordic/nrf52840_partition.dtsi
   :language: dts
   :dedent:
   :lines: 15-
