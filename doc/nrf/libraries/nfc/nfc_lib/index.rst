.. _nfc:

Near Field Communication (NFC)
##############################

Near Field Communication (NFC) is a technology for wireless transfer of small amounts of data between two devices.
The provided NFC libraries are RTOS-agnostic and built for Nordic Semiconductor nRF52 and nRF53 Series SoCs.

See the :ref:`nrf:ug_nfc` user guide for information about how to use the libraries in the |NCS|.

The following libraries are available:

Type 2 Tag
  Supports NFC-A Type 2 Tag in read-only mode.
  See :ref:`type_2_tag` for more information.

  The following variants are available:

  * libnfct2t_nrf52 (for nRF52832, nRF52833, and nRF52840)
  * libnfct2t_nrf53 (for nRF5340)

Type 4 Tag
  Supports NFC-A Type 4 Tag.
  See :ref:`type_4_tag` for more information.

  The following variants are available:

  * libnfct4t_nrf52 (for nRF52832, nRF52833, and nRF52840)
  * libnfct4t_nrf53 (for nRF5340)

Each library is distributed in both soft-float and hard-float builds.

.. important::
   The libraries are for evaluation purposes only.


.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   integration_notes
   type_2_tag
   type_4_tag
   CHANGELOG
