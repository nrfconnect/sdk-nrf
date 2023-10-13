.. _ug_matter_ecosystems_certification:

Ecosystems certification
########################

.. contents::
   :local:
   :depth: 2

The Matter stack provided in the |NCS| works with commercial ecosystems that are compatible with the official Matter implementation.
A Matter product can be called compatible with the Matter implementation once it has passed relevant Matter certification.

Some of the ecosystem providers offer additional certification programs to verify that Matter product is able to work with that provider's applications and supports their features.
Once the Matter product passes the certification for the ecosystem, the appropriate information can be placed on the product's packaging.

.. _ug_matter_google_certification:

Works with Google Home certification
************************************

The `Works with Google Home`_ certification program ensures that a Matter product supports Matter features in the same extent as the Google Home application.
A product that passed all the certification test cases is allowed to use the Works with Google Home badge.

The Matter samples delivered in the |NCS| have not received official certificates, as certification can only be obtained for final products.
However, they have been tested against the Works with Google Home certification test cases and have successfully passed all of them.
The following is a full list of Matter samples that were verified to pass the Works with Google certification test cases:

* :ref:`matter_lock_sample`
* :ref:`matter_light_bulb_sample`
* :ref:`matter_thermostat_sample`
* :ref:`matter_window_covering_sample`
* :ref:`matter_weather_station_app`

.. _ug_matter_amazon_certification:

Amazon Frustration-Free Setup certification
*******************************************

Frustration-Free Setup (FFS) is Amazon's proprietary feature that uses Amazon cloud services to provide the onboarding information necessary to commission the Matter device to a Matter fabric.
The commissioning process is automatically triggered by the Amazon commissioner and it does not require any user interaction.
The process starts when the commissioner detects the accessory's Bluetooth LE advertising messages in the proprietary format.

For more information about the FFS and how to enable it in Matter samples, see the :ref:`Amazon Frustration-Free Setup configuration<ug_matter_configuring_ffs>` section of the :ref:`ug_matter_device_advanced_kconfigs` page.

FFS requires passing the additional certification test cases for Matter called `Matter Simple Setup`_.
There are two types of certification:

* Default - For standard products or for the creation of a reference design.
* Simplified - For products using a certified reference design.

The Matter platform provided in the |NCS| was tested against the FFS certification test cases and passed them in several variants.
This means that if you create a Matter product based on the |NCS|, you can approach the simplified certification path.
You can do this by filling the Reference APID field when you register your product on Amazon's developer page.
Use one of the following values:

+----------------------------------------------------------+-----------------------+
| Matter platform variant                                  | Reference Design APID |
+==========================================================+=======================+
| :ref:`nRF52840 DK <ug_nrf52>` (Matter over Thread)       | ZNwt                  |
+----------------------------------------------------------+-----------------------+
| :ref:`nRF5340 DK <ug_nrf5340>` (Matter over Thread)      | xzNd                  |
+----------------------------------------------------------+-----------------------+
| :ref:`nRF7002 DK <nrf7002dk_nrf5340>` (Matter over Wi-Fi)| jyjh                  |
+----------------------------------------------------------+-----------------------+
