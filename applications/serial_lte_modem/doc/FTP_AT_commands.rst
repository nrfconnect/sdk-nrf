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
* ``AT#XFTP="put",<file>,<datatype>[,<data>]``
* ``AT#XFTP="uput",<datatype>[,<data>]``
* ``AT#XFTP="mput",<file>,<datatype>[,<data>]``

The values of the parameters depend on the command string used.

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
