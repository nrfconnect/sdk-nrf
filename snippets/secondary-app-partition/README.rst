.. _snippet-secondary-app-partition:

Secondary app partition snippet (secondary-app-partition)
#########################################################

.. code-block:: console

   west build -S secondary-app-partition [...]

Overview
********

This snippet changes the chosen flash partition to be the ``secondary_app_partition`` node.
You can use this to build for the alternate slot in MCUboot direct-xip or firmware loader modes.

Requirements
************

Partition mapping correct setup in devicetree, for example:

.. literalinclude:: ../../dts/vendor/nordic/nrf52840_partition.dtsi
   :language: dts
   :dedent:
   :lines: 15-
