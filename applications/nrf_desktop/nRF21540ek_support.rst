.. _nrf_desktop_nrf21540ek:

nRF Desktop: Adding nRF21540 EK shield support
##############################################

.. contents::
   :local:
   :depth: 2

You can use the nRF Desktop application with the :ref:`ug_radio_fem_nrf21540ek` shield, an RF front-end module (FEM) for the 2.4 GHz range extension.
You can use the shield with any nRF Desktop HID application configured for a development kit that is fitted with Arduino-compatible connector (see the :guilabel:`DK` tab in :ref:`nrf_desktop_requirements`).
This means that the shield support is not available for nRF Desktop's dedicated boards, such as ``nrf52840gmouse``, ``nrf52kbd``, or ``nrf52840dongle``.

Low Latency Packet mode
***********************

You cannot use the RF front-end module (FEM) together with Low Latency Packet Mode (LLPM) due to timing requirements.
You must disable the LLPM support in the nRF Desktop application (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`) for builds with FEM.

Building with EK shield support
*******************************

To build the application with the shield support, pass the ``SHIELD`` parameter to the build command.
Make sure to also disable the LLPM support.
For example, you can build the application for ``nrf52840dk/nrf52840`` with ``nrf21540ek`` shield using the following command:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 -- -DSHIELD=nrf21540ek -DCONFIG_CAF_BLE_USE_LLPM=n

For the multi-core build, you need to pass the ``SHIELD`` parameter to images built on both application and network core.
The network core controls the FEM, but the application core needs to forward the needed pins to the network core.
Use ``ipc_radio_`` as the *image_name* parameter, because in the nRF Desktop application, network core runs using :ref:`ipc_radio`.
The command for ``nrf5340dk/nrf5340/cpuapp`` with ``nrf21540ek`` shield would look as follows:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf21540ek_fwd -Dipc_radio_SHIELD=nrf21540ek -DCONFIG_CAF_BLE_USE_LLPM=n

For detailed information about building an application using the nRF21540 EK, see the :ref:`ug_radio_fem_nrf21540ek_programming` section in the Working with RF Front-end modules documentation.
