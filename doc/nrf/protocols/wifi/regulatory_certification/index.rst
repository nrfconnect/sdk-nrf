.. _ug_wifi_regulatory_certification:

Regulatory Certification Testing for nRF70 Series Devices
#########################################################

The regulatory certification process ensures that products meet specific safety, quality, and performance standards established by regulatory bodies.
Although the operation of radio equipment in the 2.4 GHz and 5 GHz bands is license-free, the end product (not the IC itself) must be type-approved by the appropriate regulatory authority.

Different regions have regulatory agencies that oversee the requirements for their region, which could be a country or a group of countries.
Each of these agencies plays a crucial role in its respective region, and compliance with their requirements is essential for manufacturers to market and sell their products there.

Some of the major regulatory agencies are:

* European Telecommunications Standards Institute (ETSI, Europe)
* Federal Communications Commission (FCC, United States of America)
* Ministry of Internal Affairs and Communications (MIC, Japan)
* State Radio Regulation of China (SRRC, China)
* Korea Communication Commission (KCC, South Korea)

This application note outlines the test setup, software tools, and configuration options for the nRF70 Series products.
It enables users to configure and conduct regulatory and compliance tests as defined by the standardization bodies.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   test_setup
   regulatory_test_cases
   antenna_gain_compensation
   band_edge_compensation
   wifi_radio_test_sample/index
   radio_test_short_range_sample/index
   using_wifi_shell_sample
   using_wifi_station_sample
   adaptivity_test_procedure
