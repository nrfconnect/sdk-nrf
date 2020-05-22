.. _nrfx:

nrfx
####

Description
***********

nrfx is a standalone set of drivers for peripherals present in Nordic
Semiconductor's SoCs. It originated as an extract from the nRF5 SDK.
The intention was to provide drivers that can be used in various
environments without the necessity to integrate other parts of the SDK
into them. For the user's convenience, the drivers come with the MDK
package. This package contains definitions of register structures and
bitfields for all supported SoCs, as well as startup and initialization
files for them.

Only relevant files from nrfx are imported. For instance, template files,
the ones with startup code or strictly related to the installation of IRQs
in the vector table are omitted in the import.

Supported SoCs
**************

* :ref:`nRF51 Series Drivers <nrf51_series_drivers>`
* :ref:`nRF52810/nRF52811 Drivers <nrf52810_drivers>`
* :ref:`nRF52832 Drivers <nrf52832_drivers>`
* :ref:`nRF52833 Drivers <nrf52833_drivers>`
* :ref:`nRF52840 Drivers <nrf52840_drivers>`
* :ref:`nRF5340 Drivers <nrf5340_drivers>`
* :ref:`nRF9160 Drivers <nrf9160_drivers>`

Directories
***********

.. code-block::

     .
     ├── doc             # Project documentation files
     ├── drivers         # nrfx driver files
     │   ├── include     # nrfx driver headers
     │   └── src         # nrfx driver sources
     ├── hal             # Hardware Access Layer files
     ├── helpers         # nrfx driver helper files
     ├── mdk             # nRF MDK files
     ├── soc             # SoC specific files
     └── templates       # Templates of nrfx integration files

Dependencies
************

CMSIS header files

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   main_page.rst
   drv_supp_matrix.rst
   nrf51_series.rst
   nrf52810.rst
   nrf52832.rst
   nrf52833.rst
   nrf52840.rst
   nrf5340.rst
   nrf9160.rst
   nrfx_api.rst
