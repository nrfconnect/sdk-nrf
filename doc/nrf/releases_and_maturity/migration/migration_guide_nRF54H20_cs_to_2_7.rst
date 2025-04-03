:orphan:

.. _migration_nrf54h20_to_2.7:

Migration notes for |NCS| v2.7.0 and the nRF54H20 DK
####################################################

This document describes the new features and changes implemented in the |NCS| to consider when migrating your application for the nRF54H20 DK to |NCS| v2.7.0.

To ensure the nRF54H20 DK runs its components correctly, verify its compatibility with the |NCS| v2.7.0 release by checking the version number on your DK's sticker.
The only versions of the nRF54H20 DK compatible with the |NCS| v2.7.0 are the following:

* Version PCA10175 v0.7.2 (ES3)
* Version PCA10175 v0.8.0 (ES3.3, ES4)
  ES4 is marked as v0.8.0 with no ES markings.

See the following documents, based on the version you are migrating from.

.. toctree::
   :maxdepth: 1
   :caption: Transition from nRF Connect SDK v2.4.99-cs3

   nRF54H20_migration_2.7/transition_guide_2.4.99-cs3_to_2.7_environment
   nRF54H20_migration_2.7/migration_guide_2.4.99-cs3_to_2.7_application


.. toctree::
   :maxdepth: 1
   :caption: Migrate from nRF Connect SDK v2.6.99-cs2

   nRF54H20_migration_2.7/migration_guide_2.6.99-cs2_to_2_7_environment
   nRF54H20_migration_2.7/migration_guide_2.6.99-cs2_to_2.7_application


Consult also the following pages about *Hardware model v2* and *Sysbuild*:

* :ref:`hwmv1_to_v2_migration`
* :ref:`child_parent_to_sysbuild_migration`
