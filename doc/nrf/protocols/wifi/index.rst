.. _ug_wifi:

Wi-Fi
#####

Wi-Fi® is the branding used by the `Wi-Fi Alliance® <Wi-Fi Alliance_>`_ to describe the family of wireless local area network standards specified by the `IEEE 802.11 Working Group`_.
The two industry bodies work closely together, with the Wi-Fi Alliance focusing on the requirements and certification of the technology, and the IEEE focusing on the underlying technical specifications.
The Wi-Fi Alliance is responsible for the Wi-Fi CERTIFIED™ accreditation program, which aims to achieve a high level of interoperability, reliability, and security compliance.

Wi-Fi is an evolving standard, with new IEEE 802.11 standards being incorporated every 4-6 years, along with the corresponding Wi-Fi Alliance certification program.
The branding *Wi-Fi 6* is defined by the Wi-Fi Alliance, and aligns with the IEEE 802.11ax specification, but includes all previous versions of the IEEE 802.11 specifications dating back to the introduction of Wi-Fi in 1997.

The evolution from the base standard started with the introduction of IEEE 802.11b and IEEE 802.11a in 1999, IEEE 802.11g in 2003, IEEE 802.11n in 2008 (and subsequently branded Wi-Fi 4), IEEE 802.11ac in 2014 (subsequently branded Wi-Fi 5), IEEE 802.11ax in 2019, branded Wi-Fi 6, and IEEE 802.11be in 2024, branded Wi-Fi 7.
These standards operate in the unlicensed spectrum of the 2.4 GHz and 5 GHz bands.
However, 11b and 11g are only applicable in the 2.4 GHz band, while 11a and 11ac are only applicable in the 5 GHz band.
The IEEE 802.11ax standard is also specified for operation in the 6 GHz band, with this variant being branded Wi-Fi 6E to signify extension to the 2.4 and 5 GHz bands used in Wi-Fi 6.
Wi-Fi 7, based on the IEEE 802.11be standard, operates across the 2.4, 5, and 6 GHz bands and introduces wider 320 MHz channels for higher throughput.

Wi-Fi is supported in the |NCS| through the nRF70 Series family of companion ICs.
The remainder of this chapter presents an overview of Wi-Fi support in the |NCS|.
The Wi-Fi protocol documentation is written to be agnostic to the specific device family, so that support for additional Wi-Fi device families can be added without duplicating content.
Content that is specific to a device family is generally documented in the corresponding device guide, although some family-specific reference material, such as the memory requirements and the regulatory certification testing, is currently kept in this section.

For more information, see the following:

* :ref:`wifi_samples` for the available samples.
* :ref:`ug_nrf70` for the information related to Wi-Fi support in the |NCS| and using the development kit (DK).
* `nRF70 Series`_ for the technical documentation on the nRF70 Series devices.
* `Guidelines and application notes for nRF70 Series devices`_.

If you want to go through an online training course to familiarize yourself with Wi-Fi and the development of Wi-Fi applications, enroll in the `Wi-Fi Fundamentals course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   wifi.rst
   station_mode/index
   scan_mode/index
   sap_mode/index
   wifi_direct
   advanced_modes/index
   provisioning/index
   regulatory_support
   regulatory_certification/index
