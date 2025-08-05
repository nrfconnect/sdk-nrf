.. _ug_bt_fast_pair:

Google Fast Pair integration
############################

Google Fast Pair is a standard for pairing *Bluetooth®* and Bluetooth Low Energy (LE) devices with as little user interaction required as possible.
Google also provides additional features built upon the Fast Pair standard.
For detailed information about supported functionalities, see the official `Fast Pair`_ documentation.

.. note::
   The software maturity level for the Fast Pair support in the |NCS| is indicated at the use case level in the :ref:`Google Fast Pair use case support <software_maturity_fast_pair_use_case>` table and at the feature level in the :ref:`Google Fast Pair feature support <software_maturity_fast_pair_feature>` table.

The subpages include the :ref:`main integration guide <ug_bt_fast_pair_integration>` and an additional page that discusses the :ref:`Advertising Manager helper module <ug_bt_fast_pair_adv_manager>`.
While this helper module can provide additional functionality in your application, it is not strictly required for the Fast Pair integration.

.. toctree::
   :maxdepth: 1
   :glob:

   core
   adv_manager
