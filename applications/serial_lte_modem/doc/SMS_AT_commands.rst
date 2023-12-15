.. _SLM_AT_SMS:

SMS AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes SMS-related AT commands.

SMS support #XSMS
=================

The ``#XSMS`` command supports functionalities for sending and receiving SMS messages.

Set command
-----------

The set command allows you to start or stop SMS, as well as send SMS text.
Only GSM-7 encoding is supported, 8-bit and UCS2 encodings are not supported.

Syntax
~~~~~~

::

   #XSMS=<op>[,<number>,<text>]

* The ``<op>`` parameter can accept one of the following values:

  * ``0`` - Stop SMS.
  * ``1`` - Start SMS, ready to receive.
  * ``2`` - Send SMS.

* The ``<number>`` parameter is a string.
  It represents the SMS recipient's phone number, including the country code (for example ``+81xxxxxxx``).
* The ``<text>`` parameter is  a string.
  It is the SMS text to be sent.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

This is the notification syntax when an SMS message is received:

  ::

      #XSMS: <datetime>,<number>,<text>

  * The ``<datetime>`` value is a string.
    It represents the time when the SMS is received.
    It has a format of "YY-MM-DD HH:MM:SS".
  * The ``<number>`` value is a string.
    It represents the SMS sender's phone number.
  * The ``<text>`` value is a string.
    It represents the SMS text that has been received.

  When receiving concatenated SMS messages, there will be only one notification.

Example
~~~~~~~

::

  at#xsms=1

  OK
  at#xsms=2,"+8190xxxxxxxx","SLM test"

  OK

  #XSMS: "21-05-24 11:58:22","090xxxxxxxx","Tested OK"

  at#xsms=2,"+8190xxxxxxxx","0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

  OK

  #XSMS: "21-05-24 13:29:47","090xxxxxxxx","0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

    #XSMS=?

Response syntax
~~~~~~~~~~~~~~~

::

    #XSMS: <list of op value>,<number>,<text>

Examples
~~~~~~~~

::

  at#xsms=?
  #XSMS: (0,1,2),<number>,<text>
  OK
