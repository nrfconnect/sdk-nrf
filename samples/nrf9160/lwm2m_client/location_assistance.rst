.. _location_assistance:

Location assistance
###################

The LwM2M Client sample supports a proprietary mechanism to fetch location assistance data from `nRF Cloud`_ by proxying it through the LwM2M server.
The location assistance is divided to two distinct approaches, GNSS-based and cell-based estimate, which is called a ground fix.
The approaches use different set of LwM2M objects. The GNSS-based approach uses only the GNSS Assistance object (ID 33625).
The ground fix approach uses mandatory Ground Fix Location object (ID 33626) and optional ECID-Signal Measurement Information object (ID 10256) and Visible Wi-Fi Access Point object (33627).

To trigger location assistance, use the following buttons on the board:

* **Button 1** - For GNSS
* **Button 2** - For ground fix

.. note::
   This feature is currently under development and as of now, only AVSystem's Coiote LwM2M server can be used for utilizing the location assistance data from nRF Cloud.

.. note::
   The old Location Assistance object (ID 50001) has been removed.

There are three different methods of obtaining the location assistance:

* Location based on cell information - The device sends information about the current cell and possibly about the neighboring cells to the  LwM2M server. The LwM2M server then sends the location request to nRF Cloud, which responds with the location data.
  Use :file:`overlay-assist-cell.conf` to enable this functionality in the sample.

* Query of A-GPS assistance data - The A-GPS assistance data is queried from nRF Cloud and provided back to the device through the LwM2M server. The A-GPS assistance data is then provided to the GNSS module for obtaining the position fix faster.
  Use :file:`overlay-assist-agps.conf` to enable this functionality in the sample.

* Query of P-GPS predictions - The P-GPS predictions are queried from nRF Cloud and provided back to the device through the LwM2M server. The predictions are stored in the device flash and injected to the GNSS module when needed.
  Use :file:`overlay-assist-pgps.conf` to enable this functionality in the sample.

Location assistance options
===========================

Check and configure the following library options that are used by the sample:

* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT` - Use Ground Fix Location object (ID 33626). Used with nRF Cloud to estimate the location of the device based on the cell neighborhood and Wi-Fi AP neighborhood.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT` - Use GNSS Assistance object (ID 33625). Used with nRF Cloud to request assistance data for the GNSS module.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS` -  nRF Cloud provides A-GPS assistance data and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS` -  nRF Cloud provides P-GPS predictions and the GNSS-module in the device uses the data for obtaining a GNSS fix, which is reported back to the LwM2M server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS` - Disable this option if you provide your own method of sending the assistance requests to the LwM2M server.
* :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER` - Disable this option if you provide your own method of populating the LwM2M objects (ID 10256) containing the cell neighborhood information.
