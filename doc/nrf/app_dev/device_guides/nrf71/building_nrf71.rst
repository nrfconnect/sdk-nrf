.. _building_nrf71:

Building and programming with nRF71 Series
##########################################

.. contents::
   :local:
   :depth: 2

.. TODO: Describe how to build and program applications for the nRF71 Series.

Board targets
*************

.. TODO: Document the board targets and when to use each:
   * ``nrf7120dk/nrf7120/cpuapp`` - secure application core.
   * ``nrf7120dk/nrf7120/cpuapp/ns`` - non-secure, security by separation with TF-M
     (cross-link to :ref:`app_boards_spe_nspe` and :ref:`ug_tfm`).
   * ``nrf7120dk/nrf7120/cpuflpr`` - FLPR coprocessor target.

Building for Wi-Fi
******************

.. TODO: Describe the Wi-Fi build configuration, including the ``wifi-ipv4`` snippet, with a west build example:

   .. code-block:: console

      west build -b nrf7120dk/nrf7120/cpuapp -S wifi-ipv4

.. TODO: Cross-link to :ref:`building` and :ref:`programming` for the general workflow.
