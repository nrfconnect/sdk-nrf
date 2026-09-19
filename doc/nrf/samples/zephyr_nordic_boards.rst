.. _zephyr_nordic_boards:

Zephyr samples on Nordic boards
###############################

The |NCS| integrates Zephyr and is compatible with many Zephyr samples.
However, Zephyr samples are not tested and verified to work with |NCS| releases.

The following table lists Zephyr samples that declare Nordic board targets in their :file:`sample.yaml` files through the ``platform_allow`` setting.
This reflects the platforms allowed for Twister and CI testing, not a guarantee that a sample works on every listed board target in all |NCS| configurations.

The table combines data from upstream Zephyr samples under :file:`zephyr/samples` and |NCS| extensions under :file:`nrf/samples/zephyr`.

To regenerate this table, run:

.. code-block:: console

   python3 scripts/doc/gen_zephyr_nordic_sample_boards.py

.. include:: /includes/zephyr_nordic_sample_boards.rst
