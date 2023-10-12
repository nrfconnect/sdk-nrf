.. _SLM_AT_FTP:

FTP AT commands
***************

.. contents::
   :local:
   :depth: 2

The following commands list contains FTP related AT commands.

FTP client #XFTP
================

The ``#XFTP`` command allows you to send FTP commands.

Set command
-----------

The set command allows you to send FTP commands.

Syntax
~~~~~~

::

   #AT#XFTP=<cmd>[,<param1>[<param2]..]]

The ``<cmd>`` command is a string, and can be used as follows:

* ``AT#XFTP="open",<username>,<password>,<hostname>[,<port>[,<sec_tag>]]``
* ``AT#XFTP="status"``
* ``AT#XFTP="ascii"``
* ``AT#XFTP="binary"``
* ``AT#XFTP="close"``
* ``AT#XFTP="verbose","on|off"``
* ``AT#XFTP="pwd"``
* ``AT#XFTP="cd",<folder>``
* ``AT#XFTP="ls"[,<options>[,<folder or file>]]``
* ``AT#XFTP="mkdir",<folder>``
* ``AT#XFTP="rmdir",<folder>``
* ``AT#XFTP="rename",<filename_old>,<filename_new>``
* ``AT#XFTP="delete",<file>``
* ``AT#XFTP="get",<file>``
* ``AT#XFTP="put",<file>[,<data>]``
* ``AT#XFTP="uput"[,<data>]``
* ``AT#XFTP="mput",<file>[,<data>]``

The values of the parameters depend on the command string used.
When using the ``put``, ``uput`` and ``mput`` commands, if the ``<data>`` attribute is not specified, SLM enters ``slm_data_mode``.

.. note::
   The maximum size of the data that can be received when using the commands ``ls`` and ``get`` is limited by the size of the internal FTP buffer, which is 2048 bytes.
   The size can be adjusted by modifying the declaration of ``ftp_data_buf`` in :file:`src/ftp_c/slm_at_ftp.c`.

   If more data than can be handled is received after issuing an ``ls`` or ``get`` command, the data will be truncated to the maximum supported size.

Response syntax
~~~~~~~~~~~~~~~

The response syntax depends on the commands used.

Examples
~~~~~~~~

::

   AT#XFTP="open",,,"speedtest.tele2.net"
   220 (vsFTPd 3.0.3)
   200 Always in UTF8 mode.
   331 Please specify the password.
   230 Login successful.
   OK

::

   AT#XFTP="status"
   215 UNIX Type: L8
   211-FTP server status:
        Connected to ::ffff:202.238.218.44
        Logged in as ftp
        TYPE: ASCII
        No session bandwidth limit
        Session timeout in seconds is 300
        Control connection is plain text
        Data connections will be plain text
        At session startup, client count was 38
        vsFTPd 3.0.3 - secure, fast, stable
   211 End of status
   OK

::

   AT#XFTP="ascii"
   200 Switching to ASCII mode.
   OK

::

   AT#XFTP="binary"
   200 Switching to Binary mode.
   OK

::

   AT#XFTP="close"
   221 Goodbye.
   OK

Read command
------------

The read command is not supported.

Test command
------------

The test command is not supported.

TFTP client #XTFTP
==================

The ``#XTFTP`` command allows you to send TFTP commands.

Set command
-----------

The set command allows you to send TFTP commands based on RFC1350.

Syntax
~~~~~~

::

   AT#XTFTP=<op>,<url>,<port>,<file_path>[,<mode>,<data>]

* The ``<op>`` parameter can accept one of the following values:

  * ``1`` - TFTP read request using IP protocol family version 4.
  * ``2`` - TFTP write request using IP protocol family version 4.
  * ``3`` - TFTP read request using IP protocol family version 6.
  * ``4`` - TFTP write request using IP protocol family version 6.

* The ``<url>`` parameter is a string.
  It indicates the hostname or the IP address of the TFTP server.
  Its maximum size is 128 bytes.
  When the parameter is an IP address, it supports both IPv4 and IPv6.
* The ``<port>`` parameter is an unsigned 16-bit integer (0 - 65535).
  It represents the TFTP service port on the remote server, which is usually port 69.
* The ``<file_path>`` parameter is a string.
  It indicates the file path on the TFTP server to read from or write to.
  Its maximum size is 128 bytes.
* The ``<mode>`` parameter is a string.
  It indicates the three modes defined in TFTP protocol.
  Valid values are ``netascii``, ``octet`` and ``mail``.
  The default value ``octet`` is applied if this parameter is omitted.
* The ``<data>`` parameter is a string.
  When ``<op>`` has a value of ``2`` or ``4``, this is the data to be put to the remote server.

   .. note::
      The maximum data size supported in WRITE request is 1024 bytes.
      The TFTP connection is terminated by writing less than 512 bytes, as defined in the protocol specification (RFC 1350).

Response syntax
~~~~~~~~~~~~~~~

::

   #XTFTP: <size>, "success"
   <data>

   #XTFTP: <error>, "<error_msg>"

* The ``<size>`` value is an integer.
  When positive, it indicates the size of data in bytes read from TFTP server.
* The ``<data>`` value is the arbitrary data read from TFTP server.
* The ``<error>`` value is an integer.
  It is a negative integer based on the type of error.
* The ``<error_msg>`` value is a string.
  It is the description that corresponds to the ``<error>`` value.

Examples
~~~~~~~~

::

  AT#XTFTP=2,"tftp-server.com",69,"test_put_01.txt","octet","test send TFTP PUT"
  #XTFTP: 18,"success"
  OK

  AT#XTFTP=1,"tftp-server.com",69,"test_put_01.txt"
  test send TFTP PUT
  #XTFTP: 18,"success"
  OK

  AT#XTFTP=2,"tftp-server.com",69,"test_put_02.txt""octet","012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
  #XTFTP: 540,"success"
  OK

  AT#XTFTP=1,"tftp-server.com",69,"test_put_02.txt"
  012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
  #XTFTP: 540,"success"
  OK

  AT#XTFTP=1,"tftp-server.com",69,"test_put_not_exist.txt"
  #XTFTP: -4, "remote error"
  ERROR

Read command
------------

The read command is not supported.

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   AT#XTFTP=?

Examples
~~~~~~~~

::

   AT#XTFTP=?
   #XTFTP: (1,2,3,4),<url>,<port>,<file_path>,<mode>
   OK
