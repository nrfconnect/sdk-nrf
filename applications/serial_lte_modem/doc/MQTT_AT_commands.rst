.. _SLM_AT_MQTT:

MQTT AT commands
****************

.. contents::
   :local:
   :depth: 2

The following commands list contains the AT commands used to operate the MQTT client.

MQTT connect #XMQTTCON
======================

The ``#XMQTTCON`` command allows you to connect to and disconnect from the MQTT broker.

Set command
-----------

The set command allows you to connect to and disconnect from the MQTT broker.

Syntax
~~~~~~

::

   AT#XMQTTCON=<op>[,<client_id>,<username>,<password>,<url>,<port>[,<sec_tag>]]

* The ``<op>`` parameter is an integer.
  It can accept one of the following values:

  * ``0`` - Disconnect from the MQTT broker.
  * ``1`` - Connect to the MQTT broker.

* The ``<client_id>`` parameter is a string.
  It indicates the MQTT Client ID.
* The ``<username>`` parameter is a string.
  It indicates the MQTT Client username.
* The ``<password>`` parameter is a string.
  It indicates the MQTT Client password in cleartext.
* The ``<url>`` parameter is a string.
  It indicates the MQTT broker hostname.
* The ``<port>`` parameter is an integer.
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
  It can assume the following values:

  * ``0`` - Connection succeded.
  * *Negative Value* - Error code.
    It indicates the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTCON=1,"MyMQTT-Client-ID","","","mqtt.server.com",1883
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

   #XMQTTCON: <cid>,<username>,<password>,<url>,<port>[,<sec_tag>]]

* The ``<cid>`` value is a string.
  It indicates the MQTT client ID.
* The ``<username>`` value is a string.
  It indicates the MQTT Client username.
* The ``<password>`` value is a string.
  It indicates the MQTT Client password in cleartext.
* The ``<url>`` value is a string.
  It indicates the MQTT broker hostname.
* The ``<port>`` value is an integer.
  It indicates the MQTT broker port.
* The ``<sec_tag>`` value is an integer.
  It indicates the credential of the security tag used for establishing a secure connection.

Examples
~~~~~~~~

::

   AT#XMQTTCON?
   #XMQTTCON: "MyMQTT-Client-ID","","","mqtt.server.com",1883
   OK

Test command
------------

The test command is not supported.

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
  It can assume the following values:

  * ``2`` - Notification that a *publish event* has been received on a topic the client is subscribed to.
  * ``7`` - Acknowledgment of the subscribe request.

* The ``<result>`` value is an integer.
  It can assume the following values:

  * ``0`` - Value indicating the acknowledgment of the connection request.
  * *Negative Value* - Error code indicating the reason for the failure.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

If the MQTT client successfully subscribes to a topic, the following unsolicited notification indicates that a message from the topic is received:

::

   #XMQTTMSG=<datatype>,<topic_length>,<message_length><CR><LF>
   <topic_received><CR><LF>
   <message>

* The ``<datatype>`` value can assume one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)

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
  It can assume the following values:

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

   AT#XMQTTPUB=<topic>,<datatype>,<msg>,<qos>,<retain>


* The ``<topic>`` parameter is a string.
  It indicates the topic on which data is published.
* The ``<datatype>`` parameter can accept one of the following values:

  * ``0`` - hexidecimal string (e.g. "DEADBEEF" for 0xDEADBEEF)
  * ``1`` - plain text (default value)

* The ``<msg>`` parameter is a string.
  It contains the payload on the topic being published.
  The max ``NET_IPV4_MTU`` is 576 bytes.
* The ``<qos>`` parameter is an integer.
  It indicates the MQTT Quality of Service types.
  It can accept the following values:

  * ``0`` - Lowest Quality of Service.
    No acknowledgment of the reception is needed for the published message.
  * ``1`` - Medium Quality of Service.
    If the acknowledgment of the reception is expected for the published message, publishing duplicate messages is permitted.
  * ``2`` - Highest Quality of Service.
    The acknowledgment of the reception is expected and the message should be published only once.

* The ``<retain>`` parameter is an integer.
  When ``1``, it indicates that the broker should store the message persistently.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  It can assume the following values:

  * ``3`` - Acknowledgment for the published message with QoS 1.
  * ``4`` - Reception confirmation for the published message with QoS 2.
    It is notified when PUBREC is received from the broker.
  * ``5`` - Release of the published message with QoS 2.
  * ``6`` - Confirmation (PUBREL) to a publish release message with QoS 2.
    It is notified when PUBREL is received from the broker.

* The ``<result>`` value is an integer.
  It can assume the following values:

  * ``0`` - Value indicating the acknowledgment of the connection request.
  * *Negative Value* - Error code indicating the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic0",1,"Test message with QoS 0",0,0
   OK
   #XMQTTMSG: 1,21,23
   nrf91/slm/mqtt/topic0
   Test message with QoS 0
   #XMQTTEVT: 2,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic1",1,"Test message with QoS 1",1,0
   OK
   #XMQTTEVT: 3,0
   #XMQTTMSG: 1,21,23
   nrf91/slm/mqtt/topic1
   Test message with QoS 1
   #XMQTTEVT: 2,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic2",1,"Test message with QoS 2",2,0
   OK
   #XMQTTEVT: 4,0
   #XMQTTEVT: 6,0
   #XMQTTMSG: 1,21,23
   nrf91/slm/mqtt/topic2Test message with QoS 2
   #XMQTTEVT: 2,0

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
