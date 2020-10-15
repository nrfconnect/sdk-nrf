.. _lwm2m_msc:

Message sequence charts
#######################

The below message sequence chart shows the initialization of the LwM2M carrier library:


.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    Application=>"LwM2M carrier Library"       [label="lwm2m_carrier_init()"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_BSDLIB_INIT"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<"LwM2M carrier Library"       [label="Success"];
    Application=>"LwM2M carrier Library"       [label="lwm2m_carrier_run()"];
    |||;
    "LwM2M carrier Library" :> "LwM2M carrier Library" [label="(no return)"];
    ...;



The following message sequence chart shows the bootstrap process:

.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    |||;
    ---                                       [label="Device is not bootstrapped yet"];
    |||;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTED"];
    "LwM2M carrier Library" rbox "LwM2M carrier Library" [label="Write keys to modem"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_BOOTSTRAPPED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_LTE_READY"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_REGISTERED"];


The following message sequence chart shows FOTA updates:

.. msc::
    hscale = "1.1";
    Application,"LwM2M carrier Library";
    |||;
    ---                                       [label="Carrier initiates modem update"];
    |||;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_FOTA_START"];
    ...;
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_DISCONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTING"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_CONNECTED"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_LTE_READY"];
    Application<<="LwM2M carrier Library"      [label="LWM2M_CARRIER_EVENT_REGISTERED"];
    ...;
