.. _SLM_AT_MQTT:

MQTT client AT commands
***********************

.. contents::
   :local:
   :depth: 2

This page describes the AT commands used to operate the MQTT client.

.. note::

   The MQTT client holds no internal state information for publish and subscript.
   It will not retransmit packets or guarantee packet delivery according to QoS classes.
   This deviates from MQTT v3.1.1.

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
  It indicates the MQTT client username.
* The ``<password>`` parameter is a string.
  It indicates the MQTT client password in cleartext.
* The ``<url>`` parameter is a string.
  It indicates the MQTT broker hostname.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It indicates the MQTT broker port.
* The ``<sec_tag>`` parameter is an integer.
  It indicates the credential of the security tag used for establishing a secure connection.

Response syntax
~~~~~~~~~~~~~~~

::

   #XMQTTEVT=<evt_type>,<result>

* The ``<evt_type>`` value is an integer indicating the type of the event.
  It can return the following values for the ``#XMQTTCON`` command:

  * ``0`` - Acknowledgment of connection request (CONNACK).
  * ``1`` - Disconnection notification (DISCONNECT).
    The MQTT client is disconnected from the MQTT broker once this event is notified.

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.


Unsolicited notification
------------------------

::
   #XMQTTEVT=<evt_type>,<result>

* The ``<evt_type>`` value is an integer indicating the type of the event.
  After the connection is established, it can return ``9`` to indicate a ping response from the MQTT broker (PINGRESP).
  This is received when pinging (PINGREQ) the broker after the keep alive is reached.

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTCFG="MyMQTT-Client-ID",300,1
   OK

   AT#XMQTTCON=1,"","","mqtt.server.com",1883
   OK
   #XMQTTEVT: 0,0

   Keep alive expires and broker responds to our ping:
   #XMQTTEVT: 9,0

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
  It indicates the topic to subscribe to.
* The ``<qos>`` parameter is an integer.
  It indicates the MQTT Quality of Service type to use.
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
  It can return the following values for the ``#XMQTTSUB`` command::

  * ``7`` - Acknowledgment of the subscribe request (SUBACK).

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.

Unsolicited notifications
~~~~~~~~~~~~~~~~~~~~~~~~~

When the MQTT client has successfully subscribed to a topic and a message is published with the topic, the following unsolicited notifications are received:

::

   #XMQTTMSG: <topic_length>,<message_length>
   <topic_received>
   <message>

* The ``<topic_length>`` value is an integer.
  It indicates the length of the ``<topic_received>`` field.
* The ``<message_length>`` parameter is an integer.
  It indicates the length of the ``<message>`` field.
* The ``<topic_received>`` value is a string.
  It indicates the topic that received the message.
* The ``<message>`` value can be a string or a HEX.
  It contains the message received from the topic.

::

   #XMQTTEVT: <evt_type>,<result>

* The ``<evt_type>`` value is an integer.
  It can return the following values for the ``#XMQTTSUB`` command:

  * ``2`` - Message received on a topic the client is subscribed to (PUBLISH).
  * ``5`` - Release of a published message with QoS 2 (PUBREL).

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.


Examples
~~~~~~~~

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic0",0
   OK
   #XMQTTEVT: 7,0

   Message with QoS0 is received:
   #XMQTTMSG: 21,7
   nrf91/slm/mqtt/topic0
   message
   #XMQTTEVT: 2,0

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic1",1
   OK
   #XMQTTEVT: 7,0

   Message with QoS1 is received:
   #XMQTTMSG: 21,7
   nrf91/slm/mqtt/topic1
   message

   #XMQTTEVT: 2,0

::

   AT#XMQTTSUB="nrf91/slm/mqtt/topic2",2
   OK
   #XMQTTEVT: 7,0

   Message with QoS2 is received:
   #XMQTTMSG: 21,7
   nrf91/slm/mqtt/topic2
   message

   #XMQTTEVT: 2,0

   #XMQTTEVT: 5,0

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
  It can return ``8`` for the ``#XMQTTUNSUB`` command to indicate an acknowledgment of the unsubscription request (UNSUBACK).

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.

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
  It indicates the MQTT Quality of Service type to use.
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
  It can return the following values for the ``#XMQTTPUB`` command:

  * ``3`` - Acknowledgment for the published message with QoS 1 (PUBACK).
  * ``4`` - Reception confirmation for the published message with QoS 2 (PUBREC).
  * ``6`` - Confirmation to a publish release message with QoS 2 (PUBCOMP).

* The ``<result>`` value is an integer. It can return the following values:

  * ``0`` - Success.
  * *Negative value* - Error code indicating the reason for the failure.

Examples
~~~~~~~~

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic0","Test message with QoS 0",0,0
   OK

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic0"
   OK
   {"msg":"Test Json publish"}+++
   #XDATAMODE: 0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic1","Test message with QoS 1",1,0
   OK
   #XMQTTEVT: 3,0

::

   AT#XMQTTPUB="nrf91/slm/mqtt/topic2","",2,0
   OK
   Test message with QoS 2+++
   #XDATAMODE: 0
   #XMQTTEVT: 4,0
   #XMQTTEVT: 6,0

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.
