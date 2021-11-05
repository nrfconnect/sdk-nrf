.. _zigbee_samples:

Zigbee samples
##############

The nRF Connect SDK provides the following samples showcasing the :ref:`ug_zigbee` protocol.
The samples can be built for various boards and can be configured for different usage scenarios.

In addition to their basic functionalities, the samples can be expanded with variants and extensions.
The following table lists variants and extensions available for each Zigbee sample:

+---------------------------------------+-------------+---------------+---------------------+-----+-------+----------+
| Variant or extension                  | Light bulb  | Light switch  | Network coordinator | NCP | Shell | Template |
+=======================================+=============+===============+=====================+=====+=======+==========+
| FEM support                           | X           | X             | X                   | X   | X     | -        |
+---------------------------------------+-------------+---------------+---------------------+-----+-------+----------+
| Sleepy End Device behavior            | -           | X             | -                   | -   | -     | -        |
+---------------------------------------+-------------+---------------+---------------------+-----+-------+----------+
| Multiprotocol Bluetooth LE            | -           | X             | -                   | -   | -     | -        |
+---------------------------------------+-------------+---------------+---------------------+-----+-------+----------+
| Zigbee FOTA                           | -           | X             | -                   | -   | -     | -        |
+---------------------------------------+-------------+---------------+---------------------+-----+-------+----------+

Enabling these variants and extensions depends on the sample and hardware you are using.
Some might require invoking specific configuration options or using mobile applications for testing purposes.


.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   ../../../samples/zigbee/*/README
