.. _SLM_AT_MQTT:

MQTT client AT commands
***********************

.. contents::
   :local:
   :depth: 2

The following commands list contains the AT commands used to operate the MQTT client.

MQTT event #XMQTTEVT
====================

The ``#XMQTTEVT`` is an unsolicited notification that indicates the event of the MQTT client.

Unsolicited notification
------------------------

It indicates the event of the MQTT client.

Syntax
~~~~~~

::

   #XMQTTEVT=<evt_type>,<result>

* The ``<evt_type>`` value is an integer indicating the type of the event.
  It can return the following values:

  * ``0`` - Connection request.
  * ``1`` - Disconnection.
    The MQTT client is disconnected from the MQTT broker once this event is notified.
  * ``2`` - Message received on a topic the client is subscribed to.
  * ``3`` - Acknowledgment for the published message with QoS 1.
  * ``4`` - Confirmation of the reception for the published message with QoS 2.
  * ``5`` - Release of the published message with QoS 2.
  * ``6`` - Confirmation to a publish release message with QoS 2.
  * ``7`` - Reception of the subscribe request.
  * ``8`` - Reception of the unsubscription request.
  * ``9`` - Ping response from the MQTT broker.

* The ``<result>`` value is an integer indicating the result of the event.
  It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Failure.
    It is the error code indicating the reason for the failure.

MQTT configure #XMQTTCFG
========================

The ``#XMQTTCFG`` command allows you to configure the MQTT client before connecting to a broker.

Set command
-----------

The set command allows you to configure the MQTT client.

Syntax
~~~~~~

::

   AT#XMQTTCFG=<client_id>[,<keep_alive>[,<clean_session>]]

* The ``<client_id>`` parameter is a string.
  It indicates the MQTT Client ID.
  If this command is not issued, SLM uses the default value of ``slm_default_client_id``.
* The ``<keep_alive>`` parameter is an integer.
  It indicates the maximum Keep Alive time in seconds for MQTT.
  The default Kepp Alive time is 60 seconds.
* The ``<clean_session>`` parameter is an integer.
  It can have one of the following values:

    * ``0`` - Connect to a MQTT broker using a persistent session.
    * ``1`` - Connect to a MQTT broker using a clean session.

  The default is using a persistent session.

Examples
~~~~~~~~

::

   AT#XMQTTCFG="MyMQTT-Client-ID",300,1
   OK

Read command
------------

The read command shows MQTT client configuration information.

Syntax
~~~~~~

::

   AT#XMQTTCFG?

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTCFG: <client_id>,<keep_alive>,<clean_session>

* The ``<client_id>`` parameter is a string.
  It indicates the MQTT Client ID.
* The ``<keep_alive>`` parameter is an integer.
  It indicates the maximum Keep Alive time in seconds for MQTT.
* The ``<clean_session>`` parameter is an integer.
  It can have one of the following values:

    * ``0`` - Connect to a MQTT broker using a persistent session.
    * ``1`` - Connect to a MQTT broker using a clean session.

Examples
~~~~~~~~

::

   AT#XMQTTCFG?
   #XMQTTCFG: "MyMQTT-Client-ID",60,0
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XMQTTCFG=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTCFG: <client_id>,<keep_alive>,<clean_session>

Examples
~~~~~~~~

::

   AT#XMQTTCFG=?
   #XMQTTCFG: <client_id>,,<keep_alive>,<clean_session>
   OK


MQTT connect #XMQTTCON
======================

The ``#XMQTTCON`` command allows you to connect to and disconnect from the MQTT broker.

Set command
-----------

The set command allows you to connect to and disconnect from the MQTT broker.

Syntax
~~~~~~

::

   AT#XMQTTCON=<op>[,<username>,<password>,<url>,<port>[,<sec_tag>]]

* The ``<op>`` parameter is an integer.
  It can accept one of the following values:

  * ``0`` - Disconnect from the MQTT broker.
  * ``1`` - Connect to the MQTT broker using IP protocol family version 4.
  * ``2`` - Connect to the MQTT broker using IP protocol family version 6.

* The ``<username>`` parameter is a string.
  It indicates the MQTT Client username.
* The ``<password>`` parameter is a string.
  It indicates the MQTT Client password in cleartext.
* The ``<url>`` parameter is a string.
  It indicates the MQTT broker hostname.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It indicates the MQTT broker port.
* The ``<sec_tag>`` parameter is an integer.
  It indicates the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  When ``0``, it indicates the acknowledgment of the connection request.
* The ``<result>`` value is an integer.
  It can return the following values:

  * ``0`` - Connection succeeded.
  * *Negative Value* - Error code.
    It indicates the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTCFG="MyMQTT-Client-ID",300,1
   OK

   AT#XMQTTCON=1,"","","mqtt.server.com",1883
   OK
   #XMQTTEVT: 0,0

::

   AT#XMQTTCON=0
   OK
   #XMQTTEVT: 1,0

Read command
------------

The read command shows MQTT client information.

Syntax
~~~~~~

::

   AT#XMQTTCON?

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTCON: <status>[,<client_id>,<url>,<port>[,<sec_tag>]]

* The ``<status>`` value is an integer.
  It can have one of the following values:

    * ``0`` - MQTT is not connected.
    * ``1`` - MQTT is connected.

* The ``<url>`` value is a string.
  It indicates the MQTT broker hostname.
  Present only when ``<status>`` is ``1``.
* The ``<port>`` value is an unsigned 16-bit integer (0 - 65535).
  It indicates the MQTT broker port.
  Present only when ``<status>`` is ``1``.
* The ``<sec_tag>`` value is an integer.
  It indicates the credential of the security tag used for establishing a secure connection.
  Present only when ``<status>`` is ``1``.

Examples
~~~~~~~~

::

   AT#XMQTTCON?
   #XMQTTCON: 1,"","","mqtt.server.com",1883
   OK

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XMQTTCON=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTCON: (list of op),<username>,<password>,<url>,<port>,<sec_tag>

Examples
~~~~~~~~

::

   AT#XMQTTCON=?
   #XMQTTCON: (0,1,2),<username>,<password>,<url>,<port>,<sec_tag>
   OK

MQTT subscribe #XMQTTSUB
========================

The ``#XMQTTSUB`` command allows you to subscribe to an MQTT topic.

Set command
-----------

The set command allows you to subscribe to an MQTT topic.

Syntax
~~~~~~

::

   AT#XMQTTSUB=<topic>,<qos>

* The ``<topic>`` parameter is a string.
  It indicates the topic to be subscribed to.
* The ``<qos>`` parameter is an integer.
  It indicates the MQTT Quality of Service types.
  It can accept the following values:

  * ``0`` - Lowest Quality of Service.
    No acknowledgment of the reception is needed for the published message.
  * ``1`` - Medium Quality of Service.
    If the acknowledgment of the reception is expected for the published message, publishing duplicate messages is permitted.
  * ``2`` - Highest Quality of Service.
    The acknowledgment of the reception is expected and the message should be published only once.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  It can return the following values:

  * ``2`` - Notification that a *publish event* has been received on a topic the client is subscribed to.
  * ``7`` - Acknowledgment of the subscribe request.

* The ``<result>`` value is an integer.
  It can return the following values:

  * ``0`` - Value indicating the acknowledgment of the connection request.
  * *Negative Value* - Error code indicating the reason for the failure.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

If the MQTT client successfully subscribes to a topic, the following unsolicited notification indicates that a message from the topic is received:

::

   #XMQTTMSG: <topic_length>,<message_length><CR><LF>
   <topic_received><CR><LF>
   <message>

* The ``<topic_length>`` value is an integer.
  It indicates the length of the ``<topic_received>`` field.
* The ``<message_length>`` parameter is an integer.
  It indicates the length of the ``<message>`` field.
* The ``<topic_received>`` value is a string.
  It indicates the topic that receives the message.
* The ``<message>`` value can be a string or a HEX.
  It contains the message received from a topic.


Examples
~~~~~~~~

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic0",0
   OK
   #XMQTTEVT: 7,0

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic1",1
   OK
   #XMQTTEVT: 7,0

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic2",2
   OK
   #XMQTTEVT: 7,0

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

MQTT unsubscribe #XMQTTUNSUB
============================

The ``#XMQTTUNSUB`` command allows you to unsubscribe from an MQTT topic.

Set command
-----------

The set command allows you to unsubscribe from an MQTT topic.

Syntax
~~~~~~

::

   AT#XMQTTUNSUB=<topic>


* The ``<topic>`` parameter is a string.
  It indicates the topic to unsubscribe from.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  When ``8``, it acknowledges the reception of the unsubscription request.

* The ``<result>`` value is an integer.
  It can return the following values:

  * ``0`` - Value indicating the successful unsubscription.
  * *Negative Value* - Error code indicating the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTUNSUB="nrf91/slm/mqtt/topic0"
   OK
   #XMQTTEVT: 8,0

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

MQTT publish #XMQTTPUB
======================

The ``#XMQTTPUB`` command allows you to publish messages on MQTT topics.

Set command
-----------

The set command allows you to publish messages on MQTT topics.

Syntax
~~~~~~

::

   AT#XMQTTPUB=<topic>[,<msg>[,<qos>[,<retain>]]]


* The ``<topic>`` parameter is a string.
  It indicates the topic on which data is published.
* The ``<msg>`` parameter is a string.
  It contains the payload on the topic being published.

  The maximum size of the payload is 1024 bytes when not empty.
  If the payload is empty (for example, ``""``), SLM enters ``slm_data_mode``.
* The ``<qos>`` parameter is an integer.
  It indicates the MQTT Quality of Service types.
  It can accept the following values:

  * ``0`` - Lowest Quality of Service (default value).
    No acknowledgment of the reception is needed for the published message.
  * ``1`` - Medium Quality of Service.
    If the acknowledgment of the reception is expected for the published message, publishing duplicate messages is permitted.
  * ``2`` - Highest Quality of Service.
    The acknowledgment of the reception is expected and the message should be published only once.

* The ``<retain>`` parameter is an integer.
  Its default value is ``0``.
  When ``1``, it indicates that the broker should store the message persistently.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  It can return the following values:

  * ``3`` - Acknowledgment for the published message with QoS 1.
  * ``4`` - Reception confirmation for the published message with QoS 2.

    It is notified when PUBREC is received from the broker.
  * ``5`` - Release of the published message with QoS 2.
  * ``6`` - Confirmation (PUBREL) to a publish release message with QoS 2.

    It is notified when PUBREL is received from the broker.

* The ``<result>`` value is an integer.
  It can return the following values:

  * ``0`` - Value indicating the acknowledgment of the connection request.
  * *Negative Value* - Error code indicating the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic0","Test message with QoS 0",0,0
   OK
   #XMQTTMSG: 21,23
   nrf91/slm/mqtt/topic0
   Test message with QoS 0
   #XMQTTEVT: 2,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic0"
   OK
   {"msg":"Test Json publish"}+++
   #XDATAMODE: 0
   #XMQTTMSG: 21,27
   nrf91/slm/mqtt/topic0
   {"msg":"Test Json publish"}
   #XMQTTEVT: 2,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic1","Test message with QoS 1",1,0
   OK
   #XMQTTEVT: 3,0
   #XMQTTMSG: 21,23
   nrf91/slm/mqtt/topic1
   Test message with QoS 1
   #XMQTTEVT: 2,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic2","",2,0
   OK
   Test message with QoS 2+++
   #XDATAMODE: 0
   #XMQTTEVT: 4,0
   #XMQTTEVT: 6,0
   #XMQTTMSG: 21,23
   nrf91/slm/mqtt/topic2
   Test message with QoS 2
   #XMQTTEVT: 2,0

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
