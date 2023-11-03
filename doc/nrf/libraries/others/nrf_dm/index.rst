.. _nrf_dm:

Nordic Distance Measurement library
###################################

The Distance Measurement library measures the distance between devices.
It can be used with nRF52 Series and nRF53 Series devices.
It does the following:

* Provides an easy-to-use interface for measuring distances.
* Computes the distance using all the information available to the device.
* Computes the distance based on the measured differential RF physical channel frequency response (MCPD mode) or the real time flight of packets (RTT mode).
* Performs all the mathematics necessary to compute the distance inside the library.

The library is available as hard-float builds for the nRF52 Series.
On the nRF53 Series, the library is available as a soft-float build for the network core.
For the application core of the nRF53 Series there is a hard-float library that contains the compute-intensive post-processing functionality only.
All libraries are included as pre-built static libraries.

The library has the following dependencies:

* The DSP math library from CMSIS/DSP.
* The functions from the C Library file :file:`math.h`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   nrf_dm_overview
   nrf_dm_usage
   CHANGELOG
   api
