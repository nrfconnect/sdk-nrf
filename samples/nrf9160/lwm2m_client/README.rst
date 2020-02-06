.. _lwm2m_client:

nRF9160: LwM2M Client
#####################

The LwM2M Client demonstrates how to use `LwM2M`_ to connect an nRF9160 DK to
an LwM2M server such as `Leshan Demo Server`_ via LTE.  Once connected, the
device can be queried for such information as: GPS, sensor data, and retrieve
information about the modem.


Overview
********

Lightweight Machine to Machine (`LwM2M`_) is an application layer protocol
based on CoAP/UDP, and is designed to expose various resources for reading,
writing and executing via an LwM2M server in a very lightweight environment.

The nRF9160 sample sends data such as button and switch state, accelerometer
data (the device's physical orientation), temperature and GPS position.  It
can also receive actuation commands such as buzzer activation and light control.

.. list-table::
   :align: center

   * - Button states
     - DOWN/UP
   * - Switch states
     - ON/OFF
   * - Accelerometer data
     - FLIP
   * - Temperature
     - TEMP
   * - GPS coordinates
     - GPS
   * - Buzzer
     - TRIGGER
   * - Light control
     - ON/OFF


Requirements
************

* The following development board:

    * |nRF9160DK|

* .. include:: /includes/spm.txt

* An LwM2M server URL address available on the internet.
  For this sample, the URL address mentioned on the `Leshan Demo Server`_ page is used.

Building and Running
********************

There are configuration files for various setups in the
samples/nrf9160/lwm2m_client directory:

- :file:`prj.conf`
  This is the standard default config.

The easiest way to setup this sample application is to build and run it
on the nRF9160-DK board using the default configuration :file:`prj.conf`.

You will need to tell the sample what LwM2M server to use by editing the
following line in the configuration you've chosen::

    CONFIG_APP_LWM2M_SERVER="leshan.eclipseprojects.io"

Build the sample:

.. |sample path| replace:: :file:`samples/nrf9160/lwm2m_client`

.. include:: /includes/build_and_run_nrf9160.txt

DTLS Support
************

The sample has DTLS security enabled by default.  The following information
will need to be entered into the LwM2M server before you can make a successful
connection: client endpoint, identity and pre-shared key.

The following are instructions specific to `Leshan Demo Server`_::

- Open up the Leshan Demo Server web UI
- Click on "Security" in the upper-right
- Click on "Add new client security configuration"
- Enter the following data:

    Client endpoint: nrf-{Your Device IMEI}

    Security mode: Pre-Shared Key

    Identity: nrf-{Your Device IMEI}

    Key: 000102030405060708090a0b0c0d0e0f

- Start the Zephyr sample

Connecting to the LwM2M Server
******************************

The sample will start and automatically connect to the LwM2M Server with
an endpoint named "nrf-{Your Device IMEI}".

NOTE: The IMEI of your device can be found on the bottom of the nRF-9160-DK
near a bar code with the FCC ID at the bottom.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`modem_info_readme`
    * :ref:`at_cmd_parser_readme`
    * ``drivers/gps_sim``
    * ``lib/bsd_lib``
    * ``drivers/sensor/sensor_sim``
    * :ref:`dk_buttons_and_leds_readme`
    * ``drivers/lte_link_control``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`

References
**********

.. _LwM2M:
   https://www.omaspecworks.org/what-is-oma-specworks/iot/lightweight-m2m-lwm2m/

.. _Leshan Demo Server:
   https://github.com/eclipse/leshan/blob/master/README.md
