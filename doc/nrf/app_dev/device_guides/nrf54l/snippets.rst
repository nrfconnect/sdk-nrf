.. _nRF54l_snippets:

Snippets for nRF54L05 and nRF54L10
##################################

.. contents::
   :local:
   :depth: 2

You can emulate the nRF54L05 and nRF54L10 targets on an nRF54L15 device using the following snippets:

.. list-table::
   :header-rows: 1

   * - Functionality
     - Snippet name
     - Compatible board targets
   * - :ref:`emulated-nrf54l05`
     - ``emulated-nrf54l05``
     - ``nrf54l15dk/nrf54l15/cpuapp``
   * - :ref:`emulated-nrf54l10`
     - ``emulated-nrf54l10``
     - ``nrf54l15dk/nrf54l15/cpuapp``

.. important::
   You cannot use these snippets with the FLPR core because all memory, including RAM and RRAM, is allocated to the application core.

Currently, using the snippets is supported only on the :ref:`zephyr:nrf54l15dk_nrf54l15` board.

.. note::
   Trusted Firmware-M (TF-M) is not yet supported for nRF54L05 and nRF54L10 emulated targets.

.. _emulated-nrf54l05:

nRF54L05 snippet
****************

The ``emulated-nrf54l05`` snippet emulates the nRF54L05 target on an nRF54L15 device.

You have the following options to add the ``emulated-nrf54l05`` snippet to the :term:`build configuration`:

.. tabs::

   .. group-tab:: west

      When building with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -- -D<image_name>_SNIPPET="emulated-nrf54l05"

   .. group-tab:: CMake

      When building with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="emulated-nrf54l05" [...]

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="emulated-nrf54l05" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.

.. _emulated-nrf54l10:

nRF54L10 snippet
****************

The ``emulated-nrf54l10`` snippet emulates the nRF54L10 target on an nRF54L15 device.

You have the following options to add the ``emulated-nrf54l10`` snippet to the :term:`build configuration`:

.. tabs::

   .. group-tab:: west

      When building with west, use the following command pattern, where *board_target* corresponds to your board target and `<image_name>` to your application image name:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -- -D<image_name>_SNIPPET="emulated-nrf54l10"

   .. group-tab:: CMake

      When building with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="emulated-nrf54l10" [...]

      To build with the |nRFVSC|, specify ``-D<image_name>_SNIPPET="emulated-nrf54l10" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.
