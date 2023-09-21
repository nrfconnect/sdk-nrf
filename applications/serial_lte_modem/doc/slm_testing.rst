.. _slm_testing:

Testing scenarios
#################

.. contents::
   :local:
   :depth: 2

The following testing scenarios give detailed instructions for testing specific use cases.
They list the required AT commands and the expected responses.

Some scenarios are generic and should work out of the box, while others require you to set up a server that you can test against.

See :ref:`slm_building` for instructions on how to build and run the Serial LTE modem application.
:ref:`slm_testing_section` describes how to turn on the modem and conduct the tests.

Generic AT commands
*******************

Complete the following steps to test the functionality provided by the :ref:`SLM_AT_gen`:

1. Retrieve the version of the serial LTE modem application.

   .. parsed-literal::
      :class: highlight

      **AT#XSLMVER**
      #XSLMVER: "2.3.0","2.3.0"
      OK

#. Read the current baud rate.

   .. parsed-literal::
      :class: highlight

      **AT#XSLMUART?**
      #XSLMUART: 115200,0
      OK

   You can change the used baud rate with the corresponding set command, but note that nRF Connect Serial Terminal requires 115200 bps for communication.

#. Retrieve a list of all supported proprietary AT commands.

   .. parsed-literal::
      :class: highlight

      **AT#XCLAC**
      AT#XSLMVER
      AT#XSLEEP
      AT#XCLAC
      AT#XSOCKET
      AT#XSOCKETOPT
      AT#XBIND
      *[...]*
      OK

#. Check the supported values for the sleep command, then put the kit in sleep mode.

   .. parsed-literal::
      :class: highlight

      **AT#XSLEEP=?**
      #XSLEEP: (1,2)
      OK

TCP/IP AT commands
******************

The following sections show how to test the functionality provided by the :ref:`SLM_AT_TCP_UDP`.

TCP client
==========

1. Establish and test a TCP connection:

   a. Check the available values for the XSOCKET command.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=?**
         #XSOCKET: (0,1,2),(1,2,3),(0,1)
         OK

   #. Open a TCP socket, read the information (handle, protocol, and role) about the open socket, and set the receive timeout of the open socket to 30 seconds.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,1,0**
         #XSOCKET: 1,1,6
         OK

         **AT#XSOCKET?**
         #XSOCKET: 1,6,0
         OK

         **AT#XSOCKETOPT=1,20,30**
         OK

   #. Connect to a TCP server on a specified port.
      Replace *example.com* with the hostname or IPv4 address of a TCP server and *1234* with the corresponding port.
      Then read the connection status.
      ``1`` indicates that the connection is established.

      .. parsed-literal::
        :class: highlight

         **AT#XCONNECT="**\ *example.com*\ **",**\ *1234*
         #XCONNECT: 1
         OK

         **AT#XCONNECT?**
         #XCONNECT: 1
         OK

   #. Send plain text data to the TCP server and retrieve the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XSEND="Test TCP"**
         #XSEND: 8
         OK

         **AT#XRECV=0**
         #XRECV: 17
         PONG: 'Test TCP'
         OK

   #. Close the socket and confirm its state.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

         **AT#XSOCKET?**
         #XSOCKET: 0
         OK

#. If you do not have a TCP server to test with, you can use TCP commands to request and receive a response from an HTTP server, for example, *www.google.com*:

   a. Open a TCP socket and connect to the HTTP server on port 80.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,1,0**
         #XSOCKET: 1,1,6
         OK

         **AT#XCONNECT="google.com",80**
         #XCONNECT: 1
         OK

   #. Send an HTTP request to the server in data mode.

      .. parsed-literal::
         :class: highlight

         **AT#XSEND**
         OK

   #. Send the text below as a whole (for example, as a copy and paste from a text editor).

      .. parsed-literal::
         :class: highlight

           HEAD / HTTP/1.1<CR><LF>
           Host: www.google.com:443<CR><LF>
           Connection: close<CR><LF>
           <CR><LF>

   #. Exit data mode.

      .. parsed-literal::
         :class: highlight

         +++
         #XDATAMODE: 0

   #. Receive the response from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XRECV=0**
         #XRECV: 576
         HTTP/1.1 200 OK
         Content-Type: text/html; charset=ISO-8859-1
         *[...]*
         OK

         **AT#XRECV=0**
         #XRECV:147
         *[...]*
         Connection: close
         OK

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

#. Test a TCP client with TCP proxy service:

   a. Check the available values for the XTCPCLI command.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPCLI=?**
         #XTCPCLI: (0,1,2),<url>,<port>,<sec_tag>
         OK

   #. Create a TCP/TLS client and connect to a server.
      Replace *example.com* with the hostname or IPv4 address of a TCP server and *1234* with the corresponding port.
      Then read the information about the connection.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPCLI=1,"**\ *example.com*\ **",**\ *1234*
         #XTCPCLI: 2,"connected"
         OK

         **AT#XTCPCLI?**
         #XTCPCLI: 1,0
         OK

   #. Send plain text data to the TCP server and retrieve ten bytes of the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSEND="Test TCP"**
         #XTCPSEND: 8
         OK

         **AT#XTCPRECV=10**
         PONG: b'Te
         #XTCPRECV: 10
         OK

   #. Disconnect and confirm the status of the connection.
      ``-1`` indicates that no connection is open.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPCLI=0**
         OK

         **AT#XTCPCLI?**
         #XTCPCLI: -1
         OK

UDP client
==========

1. Test a UDP client with connectionless UDP:

   a. Open a UDP socket and read the information (handle, protocol, and role) about the open socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,2,0**
         #XSOCKET: 1,2,17
         OK
         **AT#XSOCKET?**
         #XSOCKET: 1,17,0
         OK

   #. Send plain text data to a UDP server on a specified port.
      Replace *example.com* with the hostname or IPv4 address of a UDP server and *1234* with the corresponding port.
      Then retrieve the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XSENDTO="**\ *example.com*\ **",**\ *1234*\ **,"Test UDP"**
         #XSENDTO: 8
         OK
         **AT#XRECVFROM=0**
         #XRECVFROM: 14
         PONG: Test UDP
         OK

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

#. Test a UDP client with connection-based UDP:

   a. Open a UDP socket and connect to a UDP server on a specified port.
      Replace *example.com* with the hostname or IPv4 address of a UDP server and *1234* with the corresponding port.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,2,0**
         #XSOCKET: 1,2,17
         OK

         **AT#XCONNECT="**\ *example.com*\ **",**\ *1234*
         #XCONNECT: 1
         OK

   #. Send plain text data to the UDP server and retrieve the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XSEND="Test UDP"**
         #XSEND: 8
         OK

         **AT#XRECV=0**
         #XRECV: 14
         PONG: Test UDP
         OK

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

#. Test a connection-based UDP client with UDP proxy service:

   a. Check the available values for the XUDPCLI command.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPCLI=?**
         #XUDPCLI: (0,1,2),<url>,<port>,<sec_tag>
         OK

   #. Create a UDP client and connect to a server.
      Replace *example.com* with the hostname or IPv4 address of a UDP server and *1234* with the corresponding port.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPCLI=1,"**\ *example.com*\ **",**\ *1234*
         #XUDPCLI: 2,"connected"
         OK

   #. Send plain text data to the UDP server and check the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPSEND="Test UDP"**
         #XUDPSEND: 8
         OK
         #XUDPDATA: 14
         PONG: Test UDP

   #. Disconnect from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPCLI=0**
         OK

TLS client
==========

Before completing this test, you must update the CA certificate, the client certificate, and the private key to be used for the TLS connection in the modem.
The credentials must use the security tag 16842755.

To store the credentials in the modem, use the Cellular Monitor app.
See `Managing credentials`_ in the Cellular Monitor User Guide for instructions.

You must register the corresponding credentials on the server side.

1. Establish and test a TLS connection:

   a. List the credentials that are stored in the modem with security tag 16842755.

      .. parsed-literal::
         :class: highlight

         **AT%CMNG=1,16842755**
         %CMNG: 16842755,0,"0000000000000000000000000000000000000000000000000000000000000000"
         %CMNG: 16842755,1,"0101010101010101010101010101010101010101010101010101010101010101"
         %CMNG: 16842755,2,"0202020202020202020202020202020202020202020202020202020202020202"
         OK

   #. Open a TCP/TLS socket that uses the security tag 16842755 and connect to a TLS server on a specified port.
      Replace *example.com* with the hostname or IPv4 address of a TLS server and *1234* with the corresponding port.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,1,0,16842755**
         #XSOCKET: 1,1,258
         OK

         **AT#XCONNECT="**\ *example.com*\ **",**\ *1234*
         #XCONNECT: 1
         OK

   #. Send plain text data to the TLS server and retrieve the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XSEND="Test TLS client"**
         #XSEND: 15
         OK

         **AT#XRECV=0**
         #XRECV: 24
         PONG: b'Test TLS client'
         OK

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

#. Test a TLS client with TCP proxy service:

   a. Create a TCP/TLS client and connect to a server.
      Replace *example.com* with the hostname or IPv4 address of a TLS server and *1234* with the corresponding port.
      Then read the information about the connection.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPCLI=1,"**\ *example.com*\ **",**\ *1234*
         #XTCPCLI: 2,"connected"
         OK

         **AT#XTCPCLI?**
         #XTCPCLI: 1,0
         OK

   #. Send plain text data to the TLS server and retrieve the returned data.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSEND="Test TLS client"**
         #XTCPSEND: 15
         OK
         #XTCPDATA: 24

         **AT#XTCPRECV**
         PONG: b'Test TLS client'
         #XTCPRECV: 24
         OK

   #. Disconnect from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPCLI=0**
         #XTCPCLI: "disconnected"
         OK

.. not tested

DTLS client
===========

The DTLS client requires connection-based UDP to trigger the DTLS establishment.

Before completing this test, you must update the Pre-shared Key (PSK) and the PSK identity to be used for the TLS connection in the modem.
The credentials must use the security tag 16842756.

To store the credentials in the modem, enter the following AT commands:

.. parsed-literal::
   :class: highlight

   **AT%CMNG=0,16842756,3,"6e7266393174657374"**
   **AT%CMNG=0,16842756,4,"nrf91test"**

You must register the same PSK and PSK identity on the server side.

1. Establish and test a DTLS connection:

   a. List the credentials that are stored in the modem with security tag 16842755.

	  .. parsed-literal::
		 :class: highlight

		 **AT%CMNG=1,16842756**
		 %CMNG: 16842756,3,"0303030303030303030303030303030303030303030303030303030303030303"
		 %CMNG: 16842756,4,"0404040404040404040404040404040404040404040404040404040404040404"
		 OK

   #. Open a TCP/DTLS socket that uses the security tag 16842756 and connect to a DTLS server on a specified port.
      Replace *example.com* with the hostname or IPv4 address of a DTLS server and *1234* with the corresponding port.

	  .. parsed-literal::
		 :class: highlight

		 **AT#XSOCKET=1,2,0,16842756**
		 #XSOCKET: 1,2,273
		 OK

		 **AT#XCONNECT="**\ *example.com*\ **",**\ *1234*
		 #XCONNECT: 1
		 OK

   #. Send plain text data to the DTLS server and retrieve the returned data.

	  .. parsed-literal::
		 :class: highlight

		 **AT#XSEND="Test DTLS client"**
		 #XSEND: 16
		 OK

		 **AT#XRECV=0**
		 #XRECV: 25
		 PONG: b'Test DTLS client'
		 OK

   #. Close the socket.

	  .. parsed-literal::
		 :class: highlight

		 **AT#XSOCKET=0**
		 #XSOCKET: 0,"closed"
		 OK

#. Test a DTLS client with UDP proxy service:

   a. Create a UDP/DTLS client and connect to a server.
      Replace *example.com* with the hostname or IPv4 address of a DTLS server and *1234* with the corresponding port.
      Then read the information about the connection.

	  .. parsed-literal::
		 :class: highlight

		 **AT#XUDPCLI=1,"**\ *example.com*\ **",**\ *1234*\ **,16842756**
		 #XUDPCLI: 2,"connected"
		 OK

   #. Disconnect from the server.

	  .. parsed-literal::
		 :class: highlight

		 **AT#XUDPCLI=0**
		 OK

TCP server
==========

.. |global_private_address| replace:: the nRF91 Series DK must have a global private address.
   The radio network must be configured to route incoming IP packets to the nRF91 Series DK.

.. |global_private_address_check| replace::    To check if the setup is correct, use the ``AT+CGDCONT?`` command to check if the local IP address allocated by the network is a reserved private address of class A, B, or C (see `Private addresses`_).
   If it is not, ping your nRF91 Series DK from the destination server.


To act as a TCP server, |global_private_address|

|global_private_address_check|

1. Create a Python script :file:`client_tcp.py` that acts as a TCP client.
   See the following sample code (make sure to use the correct IP address and port):

   .. code-block:: python

      import socket
      import time

      host_addr = '000.000.000.00'
      host_port = 1234
      s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.connect((host_addr, host_port))
      time.sleep(1)
      print("Sending: 'Hello, TCP#1!")
      s.send(b"Hello, TCP#1!")
      time.sleep(1)
      print("Sending: 'Hello, TCP#2!")
      s.send(b"Hello, TCP#2!")
      data = s.recv(1024)
      print(data)

      time.sleep(1)
      print("Sending: 'Hello, TCP#3!")
      s.send(b"Hello, TCP#3!")
      time.sleep(1)
      print("Sending: 'Hello, TCP#4!")
      s.send(b"Hello, TCP#4!")
      time.sleep(1)
      print("Sending: 'Hello, TCP#5!")
      s.send(b"Hello, TCP#5!")
      time.sleep(1)
      data = s.recv(1024)
      print(data)

      print("Closing connection")
      s.close()

#. Establish and test a TCP connection:

   a. Open a TCP socket, bind it to the TCP port that you want to use, and start listening.
      Replace *1234* with the correct port number.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,1,1**
         #XSOCKET: 2,1,6
         OK

         **AT#XBIND=**\ *1234*
         OK

         **AT#XLISTEN**
         OK

   #. Run the :file:`client_tcp.py` script to start sending data to the server.

   #. Accept the connection from the client and start receiving and acknowledging the data.

      .. parsed-literal::
         :class: highlight

         **AT#XACCEPT**
         #XACCEPT: connected with *IP address*
         #XACCEPT: 3
         OK

         **AT#XRECV=0**
         #XRECV: 26
         Hello, TCP#1!Hello, TCP#2!
         OK

         **AT#XSEND="TCP1/2 received"**
         #XSEND: 15
         OK

         **AT#XRECV=0**
         #XRECV: 39
         Hello, TCP#3!Hello, TCP#4!Hello, TCP#5!
         OK

         **AT#XSEND="TCP3/4/5 received"**
         #XSEND: 17
         OK

   #. Observe the output of the Python script::

         $ python client_tcp.py

         Sending: 'Hello, TCP#1!
         Sending: 'Hello, TCP#2!
         TCP1/2 received
         Sending: 'Hello, TCP#3!
         Sending: 'Hello, TCP#4!
         Sending: 'Hello, TCP#5!
         TCP3/4/5 received
         Closing connection

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: "closed"
         OK


#. Test the TCP server with TCP proxy service:

   a. Check the available values for the XTCPSVR command and read the information about the current state.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSVR=?**
         #XTCPSVR: (0,1,2),<port>,<sec_tag>
         OK

         **AT#XTCPSVR?**
         #XTCPSVR: -1,-1
         OK

   #. Create a TCP server and read the information about the current state.
      Replace *1234* with the correct port number.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSVR=1,**\ *1234*
         #XTCPSVR: 2,"started"
         OK

         **AT#XTCPSVR?**
         #XTCPSVR: 1,-1,0
         OK

   #. Run the :file:`client_tcp.py` script to start sending data to the server.

   #. Observe that the server accepts the connection from the client.
      Read the information about the current state again.

      .. parsed-literal::
         :class: highlight

         #XTCPSVR: *IP address* connected
         #XTCPDATA: 13
         #XTCPDATA: 13

         **AT#XTCPSVR?**
         #XTCPSVR: 1,2,0
         OK

   #. Start receiving and acknowledging the data.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPRECV**
         Hello, TCP#1!Hello, TCP#2!
         #XTCPRECV: 26
         OK

         **AT#XTCPSEND="TCP1/2 received"**
         #XTCPSEND: 15
         OK
         #XTCPDATA: 13
         #XTCPDATA: 13
         #XTCPDATA: 13

         **AT#XTCPSVR?**
         #XTCPSVR: 1,2,0
         OK

         **AT#XTCPRECV**
         Hello, TCP#3!Hello, TCP#4!Hello, TCP#5!
         #XTCPRECV: 39
         OK

         **AT#XTCPSEND=1,"TCP3/4/5 received"**
         #XTCPSEND: 17
         OK

   #. Observe the output of the Python script::

         $ python client_tcp.py

         Sending: 'Hello, TCP#1!
         Sending: 'Hello, TCP#2!
         TCP1/2 received
         Sending: 'Hello, TCP#3!
         Sending: 'Hello, TCP#4!
         Sending: 'Hello, TCP#5!
         TCP3/4/5 received
         Closing connection

   #. Read the information about the current state.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSVR?**
         #XTCPSVR: 1,2,0
         OK

   #. Stop the server.

      .. parsed-literal::
         :class: highlight

         **AT#XTCPSVR=0**
         #XTCPSVR:-1,"stopped"
         OK

         **AT#XTCPSVR?**
         #XTCPSVR: -1,-1
         OK

UDP server
==========

To act as a UDP server, |global_private_address|

|global_private_address_check|

1. Create a Python script :file:`client_udp.py` that acts as a UDP client.
   See the following sample code (make sure to use the correct IP addresses and port):

   .. code-block:: python

      import socket
      import time

      host_addr = '000.000.000.00'
      host_port = 1234
      host = (host_addr, host_port)
      local_addr = '9.999.999.99'
      local_port = 1234
      local = (local_addr, local_port)
      s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
      s.bind(local)
      print("Sending: 'Hello, UDP#1!")
      s.sendto("Hello, UDP#1!", host)
      time.sleep(1)
      print("Sending: 'Hello, UDP#2!")
      s.sendto("Hello, UDP#2!", host)
      data, address = s.recvfrom(1024)
      print(data)
      print(address)

      print("Sending: 'Hello, UDP#3!")
      s.sendto("Hello, UDP#3!", host)
      time.sleep(1)
      print("Sending: 'Hello, UDP#4!")
      s.sendto("Hello, UDP#4!", host)
      time.sleep(1)
      print("Sending: 'Hello, UDP#5!")
      s.sendto("Hello, UDP#5!", host)
      data, address = s.recvfrom(1024)
      print(data)
      print(address)

      print("Closing connection")
      s.close()

#. Establish and test a UDP connection:

   a. Open a UDP socket and bind it to the UDP port that you want to use.
      Replace *1234* with the correct port number.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=1,2,1**
         #XSOCKET: 2,2,17
         OK

         **AT#XBIND=**\ *1234*
         OK

   #. Run the :file:`client_udp.py` script to start sending data to the server.

   #. Start receiving and acknowledging the data.
      Replace *example.com* with the hostname or IPv4 address of the UDP client and *1234* with the corresponding port.

      .. parsed-literal::
         :class: highlight

         **AT#XRECVFROM=0**
         #XRECVFROM: 13
         Hello, UDP#1!
         OK

         **AT#XRECVFROM=0**
         #XRECVFROM: 13
         Hello, UDP#2!
         OK

         **AT#XSENDTO="**\ *example.com*\ **",**\ *1234*\ **,"UDP1/2 received"**
         #XSENDTO: 15
         OK

         **AT#XRECVFROM=0**
         #XRECVFROM: 13
         Hello, UDP#3!
         OK

         **AT#XRECVFROM=0**
         #XRECVFROM: 13
         Hello, UDP#4!
         OK

         **AT#XRECVFROM=0**
         #XRECVFROM: 13
         Hello, UDP#5!
         OK

         **AT#XSENDTO="**\ *example.com*\ **",**\ *1234*\ **,"UDP3/4/5 received"**
         #XSENDTO: 17
         OK

      Note that you will get an error message if a UDP packet is lost.
      For example, this error indicates that a packet is lost in the downlink to the nRF91 Series DK:

      .. parsed-literal::
         :class: highlight

         **AT#XRECVFROM=0**
         #XSOCKET: -60
         ERROR

   #. Observe the output of the Python script::

         $ python client_udp.py

         Sending: 'Hello, UDP#1!
         Sending: 'Hello, UDP#2!
         UDP1/2 received
         ('000.000.000.00', 1234)
         Sending: 'Hello, UDP#3!
         Sending: 'Hello, UDP#4!
         Sending: 'Hello, UDP#5!
         UDP3/4/5 received
         ('000.000.000.00', 1234)
         Closing connection

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XSOCKET=0**
         #XSOCKET: 0,"closed"
         OK

#. Test the UDP server with UDP proxy service:

   a. Check the available values for the XUDPSVR command and create a UDP server.
      Replace *1234* with the correct port number.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPSVR=?**
         #XUDPSVR: (0,1,2),<port>,<sec_tag>
         OK

         **AT#XUDPSVR=1,**\ *1234*
         #XUDPSVR: 2,"started"
         OK

   #. Run the :file:`client_udp.py` script to start sending data to the server.

   #. Observe that the server starts receiving data and acknowledge the data.

      .. parsed-literal::
         :class: highlight

         #XUDPDATA: 13
         Hello, UDP#1!
         #XUDPDATA: 13
         Hello, UDP#2!

         **AT#XUDPSEND="UDP1/2 received"**
         #XUDPSEND: 15
         OK

         #XUDPDATA: 13
         Hello, UDP#3!
         #XUDPDATA: 13
         Hello, UDP#4!
         #XUDPDATA: 13
         Hello, UDP#5!

         **AT#XUDPSEND="UDP3/4/5 received"**
         #XUDPSEND: 17
         OK

   #. Observe the output of the Python script::

         $ python client_udp.py

         Sending: 'Hello, UDP#1!
         Sending: 'Hello, UDP#2!
         UDP1/2 received
         ('000.000.000.00', 1234)
         Sending: 'Hello, UDP#3!
         Sending: 'Hello, UDP#4!
         Sending: 'Hello, UDP#5!
         UDP3/4/5 received
         ('000.000.000.00', 1234)
         Closing connection

   #. Close the socket.

      .. parsed-literal::
         :class: highlight

         **AT#XUDPSVR=0**
         #XUDPSVR: "stopped"
         OK

TLS server
==========

The TLS server role is currently not supported.

.. parsed-literal::
   :class: highlight

   **AT#XSOCKET=1,1,1,16842753**
   #XSOCKET: "(D)TLS Server not supported"
   ERROR

   **AT#XTCPSVR=1,3443,16842753**
   #XTCPSVR: "TLS Server not supported"
   ERROR

DTLS server
===========

The DTLS server role is currently not supported (modem limitation).

.. parsed-literal::
   :class: highlight

   **AT#XSOCKET=1,2,1,16842755**
   #XSOCKET: "(D)TLS Server not supported"
   ERROR

DNS lookup
==========

1. Look up the IP address for a hostname.

   .. parsed-literal::
      :class: highlight

      **AT#XGETADDRINFO="www.google.com"**
      #XGETADDRINFO: "172.217.174.100"
      OK

      **AT#XGETADDRINFO="ipv6.google.com"**
      #XGETADDRINFO: "2404:6800:4006:80e::200e"
      OK

      **AT#XGETADDRINFO="172.217.174.100"**
      #XGETADDRINFO: "172.217.174.100"
      OK

      **AT#XGETADDRINFO="2404:6800:4006:80e::200e"**
      #XGETADDRINFO: "2404:6800:4006:80e::200e"
      OK

Socket options
==============

After opening a client-role socket, you can configure various options.

1. Check the available values for the XSOCKETOPT command.

   .. parsed-literal::
      :class: highlight

      **AT#XSOCKETOPT=?**
      #XSOCKETOPT: (0,1),<name>,<value>
      OK

#. Open a client socket.

   .. parsed-literal::
      :class: highlight

      **AT#XSOCKET=1,1,0**
      #XSOCKET: 2,1,6
      OK

#. Test to set and get socket options.
   Note that not all options are supported.

   .. parsed-literal::
      :class: highlight

      **AT#XSOCKETOPT=1,20,30**
      OK

ICMP AT commands
****************

Complete the following steps to test the functionality provided by the :ref:`SLM_AT_ICMP`:

1. Ping a remote host, for example, *www.google.com*.

   .. parsed-literal::
      :class: highlight

      **AT#XPING="www.google.com",45,5000,5,1000**
      OK
      #XPING: 0.637 seconds
      #XPING: 0.585 seconds
      #XPING: 0.598 seconds
      #XPING: 0.598 seconds
      #XPING: 0.599 seconds
      #XPING: average 0.603 seconds

      **AT#XPING="ipv6.google.com",45,5000,5,1000**
      OK
      #XPING: 0.140 seconds
      #XPING: 0.109 seconds
      #XPING: 0.113 seconds
      #XPING: 0.118 seconds
      #XPING: 0.112 seconds
      #XPING: average 0.118 seconds

#. Ping a remote IP address, for example, 172.217.174.100.

   .. parsed-literal::
      :class: highlight

      **AT#XPING="172.217.174.100",45,5000,5,1000**
      OK
      #XPING: 0.873 seconds
      #XPING: 0.576 seconds
      #XPING: 0.599 seconds
      #XPING: 0.623 seconds
      #XPING: 0.577 seconds
      #XPING: average 0.650 seconds

FTP AT commands
***************

Note that these commands are available only if :ref:`CONFIG_SLM_FTPC <CONFIG_SLM_FTPC>` is defined.
Before you test the FTP AT commands, check the setting of the :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME` option.
By default, the :ref:`lib_ftp_client` library keeps the connection to the FTP server alive for 60 seconds, but you can change the duration or turn KEEPALIVE off by setting :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME` to 0.

The FTP client behavior depends on the FTP server that is used for testing.
Complete the following steps to test the functionality provided by the :ref:`SLM_AT_FTP` with two example servers:

1. Test an FTP connection to *speedtest.tele2.net*.

   This server supports only anonymous login.
   Files must be uploaded to a given folder and will be deleted immediately.
   It is not possible to create, rename, or delete folders or rename files.

   a. Connect to the FTP server, check the status, and change the transfer mode.
      Then disconnect.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="open",,,"speedtest.tele2.net"**
         220 (vsFTPd 3.0.3)
         200 Always in UTF8 mode.
         331 Please specify the password.
         230 Login successful.
         OK

         **AT#XFTP="status"**
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

         **AT#XFTP="ascii"**
         200 Switching to ASCII mode.
         OK

         **AT#XFTP="binary"**
         200 Switching to Binary mode.
         OK

         **AT#XFTP="close"**
         221 Goodbye.
         OK

   #. Connect to the FTP server and retrieve information about the existing files and folders.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="open",,,"speedtest.tele2.net"**
         220 (vsFTPd 3.0.3)
         200 Always in UTF8 mode.
         331 Please specify the password.
         230 Login successful.
         OK

         **AT#XFTP="pwd"**
         257 "/" is the current directory
         OK

         **AT#XFTP="ls"**
         227 Entering Passive Mode (90,130,70,73,103,35).
         1000GB.zip
         100GB.zip
         100KB.zip
         *[...]*
         5MB.zip
         upload
         150 Here comes the directory listing.
         226 Directory send OK.
         OK

         **AT#XFTP="ls","-l"**
         227 Entering Passive Mode (90,130,70,73,94,158).
         150 Here comes the directory listing.
         -rw-r--r--    1 0        0        1073741824000 Feb 19  2016 1000GB.zip
         -rw-r--r--    1 0        0        107374182400 Feb 19  2016 100GB.zip
         -rw-r--r--    1 0        0          102400 Feb 19  2016 100KB.zip
         -rw-r--r--    1 0        0        104857600 Feb 19  2016 100MB.zip
         *[...]*
         -rw-r--r--    1 0        0         5242880 Feb 19  2016 5MB.zip
         drwxr-xr-x    2 105      108        561152 Apr 30 02:30 upload
         226 Directory send OK.
         OK

         **AT#XFTP="ls","-l","upload"**
         227 Entering Passive Mode (90,130,70,73,86,44).
         150 Here comes the directory listing.
         -rw-------    1 105      108      57272385 Apr 30 02:29 10MB.zip
         -rw-------    1 105      108        119972 Apr 30 02:30 14qj36kc9esslej6porartkjks.txt
         *[...]*
         -rw-------    1 105      108         32352 Apr 30 02:30 upload_file.txt
         226 Directory send OK.
         OK

         **AT#XFTP="cd","upload"**
         250 Directory successfully changed.
         OK

         **AT#XFTP="pwd"**
         257 "/upload" is the current directory
         OK

         **AT#XFTP="ls","-l"**
         227 Entering Passive Mode (90,130,70,73,113,191).
         150 Here comes the directory listing.
         -rw-------    1 105      108      57272385 Apr 30 02:29 10MB.zip
         -rw-------    1 105      108        294236 Apr 30 02:31 1MB.zip
         *[...]*
         -rw-------    1 105      108        838960 Apr 30 02:31 upload_file.txt
         226 Directory send OK.
         OK

         **AT#XFTP="cd", ".."**
         250 Directory successfully changed.
         OK

         **AT#XFTP="pwd"**
         257 "/" is the current directory
         OK

         **AT#XFTP="ls","-l"**
         227 Entering Passive Mode (90,130,70,73,90,43).
         150 Here comes the directory listing.
         -rw-r--r--    1 0        0        1073741824000 Feb 19  2016 1000GB.zip
         -rw-r--r--    1 0        0        107374182400 Feb 19  2016 100GB.zip
         -rw-r--r--    1 0        0          102400 Feb 19  2016 100KB.zip
         *[...]*
         -rw-r--r--    1 0        0         5242880 Feb 19  2016 5MB.zip
         drwxr-xr-x    2 105      108        561152 Apr 30 02:31 upload
         226 Directory send OK.
         OK

         **AT#XFTP="ls","-l 1KB.zip"**
         227 Entering Passive Mode (90,130,70,73,106,84).
         150 Here comes the directory listing.
         -rw-r--r--    1 0        0            1024 Feb 19  2016 1KB.zip
         226 Directory send OK.
         OK

   #. Switch to binary transfer mode and download a file from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="binary"**
         200 Switching to Binary mode.
         OK

         **AT#XFTP="get","1KB.zip"**
         227 Entering Passive Mode (90,130,70,73,84,29).

         00000000000000000000000000\ *[...]*\ 000000000000
         226 Transfer complete.
         OK

   #. Navigate to the :file:`upload` folder, switch to binary transfer mode, and create a binary file with the content "DEADBEEF".

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="cd","upload"**
         250 Directory successfully changed.
         OK

         **AT#XFTP="binary"**
         200 Switching to Binary mode.
         OK

         **AT#XFTP="put","upload.bin",0,"DEADBEEF"**
         227 Entering Passive Mode (90,130,70,73,114,150).
         150 Ok to send data.
         226 Transfer complete.
         OK

   #. Switch to ASCII transfer mode and create a text file with the content "TEXTDATA".

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="ascii"**
         200 Switching to ASCII mode.
         OK

         **AT#XFTP="put","upload.txt",1,"TEXTDATA"**
         227 Entering Passive Mode (90,130,70,73,99,84).
         150 Ok to send data.
         226 Transfer complete.
         OK

   #. Disconnect from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="close"**
         221 Goodbye.
         OK

#. Test an FTP connection to "ftp.dlptest.com".

   This server does not support anonymous login.
   Go to `DLPTest.com`_ to get the latest login information.
   After login on, you can create and remove folders and files, rename files, and upload files.

   a. Connect to the FTP server and check the status.
      Replace *user* and *password* with the login information from `DLPTest.com`_.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="open","**\ *user*\ **","**\ *password*\ **","ftp.dlptest.com"**
         220-#########################################################
         220-Please upload your web files to the public_html directory.
         220-Note that letters are case sensitive.
         220-#########################################################
         220 This is a private system - No anonymous login
         200 OK, UTF-8 enabled
         331 User *user* OK. Password required
         230-Your bandwidth usage is restricted
         230 OK. Current restricted directory is /
         OK

         **AT#XFTP="status"**
         215 UNIX Type: L8
         211 https:\ //www.pureftpd.org/
         OK

   #. Retrieve information about the existing files and folders.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="pwd"**
         257 "/" is your current location
         OK

         **AT#XFTP="ls"**
         227 Entering Passive Mode (35,209,241,59,135,181)
         150 Accepted data connection
         226-Options: -a
         226 42 matches total
         OK
         .
         ..
         1_2596384601376578508_17-9ULspeedtest.upt
         1_603281663034123496_17-9ULspeedtest.upt
         *[...]*
         aa\_.rar
         write to File.txt

   #. Create a folder and enter it.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="mkdir", "newfolder"**
         257 "newfolder" : The directory was successfully created
         OK

         **AT#XFTP="ls","-l","newfolder"**
         227 Entering Passive Mode (35,209,241,59,135,134)
         150 Accepted data connection
         226-Options: -a -l
         226 2 matches total
         OK
         drwxr-xr-x    2 dlptest9   dlptest9         4096 Apr 29 19:53 .
         drwxr-xr-x    3 dlptest9   dlptest9        57344 Apr 29 19:53 ..
         +CEREG: 1,"1285","02EF8210",7

         **AT#XFTP="cd","newfolder"**
         250 OK. Current directory is /newfolder
         OK

   #. Switch to binary transfer mode and create a binary file with the content "DEADBEEF".

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="binary"**
         200 TYPE is now 8-bit binary
         OK

         **AT#XFTP="put","upload.bin",0,"DEADBEEF"**
         227 Entering Passive Mode (35,209,241,59,135,182)
         150 Accepted data connection
         226-File successfully transferred
         226 0.013 seconds (measured here), 310.20 bytes per second
         OK

         **AT#XFTP="ls","-l","upload.bin"**
         227 Entering Passive Mode (35,209,241,59,135,146)
         150 Accepted data connection
         226-Options: -a -l
         226 1 matches total
         OK
         -rw-r--r--    1 dlptest9   dlptest9            4 Apr 29 19:54 upload.bin

   #. Rename the file.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="rename","upload.bin","uploaded.bin"**
         350 RNFR accepted - file exists, ready for destination
         250 File successfully renamed or moved
         OK

         **AT#XFTP="ls","-l","uploaded.bin"**
         227 Entering Passive Mode (35,209,241,59,135,111)
         150 Accepted data connection
         -rw-r--r--    1 dlptest9   dlptest9            4 Apr 29 19:54 uploaded.bin
         226-Options: -a -l
         226 1 matches total
         OK

   #. Switch to ASCII transfer mode and create a text file with the content "line #1\\r\\n".

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="ascii"**
         200 TYPE is now ASCII
         OK

         **AT#XFTP="put","upload.txt",1,"line #1\\r\\n"**
         227 Entering Passive Mode (35,209,241,59,135,136)
         150 Accepted data connection
         226-File successfully transferred
         226 0.013 seconds (measured here), 0.82 Kbytes per second
         OK

         **AT#XFTP="ls","-l upload.txt"**
         227 Entering Passive Mode (35,209,241,59,135,166)
         150 Accepted data connection
         226-Options: -a -l
         226 1 matches total
         OK
         -rw-r--r--    1 dlptest9   dlptest9           11 Apr 29 19:56 upload.txt

   #. Rename the file.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="rename","upload.txt","uploaded.txt"**
         350 RNFR accepted - file exists, ready for destination
         250 File successfully renamed or moved
         OK

         **AT#XFTP="ls","-l uploaded.txt"**
         227 Entering Passive Mode (35,209,241,59,135,213)
         200 Zzz...  // (KEEPALIVE response)
         150 Accepted data connection
         226-Options: -a -l
         226 1 matches total
         OK
         -rw-r--r--    1 dlptest9   dlptest9           11 Apr 29 19:56 uploaded.txt
         +CEREG: 1,"1285","02EF8200",7

   #. Delete the files and the folder that you created.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="delete","uploaded.bin"**
         250 Deleted uploaded.bin
         OK

         **AT#XFTP="delete","uploaded.txt"**
         250 Deleted uploaded.txt
         OK

         **AT#XFTP="cd", ".."**
         250 OK. Current directory is /
         OK

         **AT#XFTP="rmdir", "newfolder"**
         250 The directory was successfully removed
         OK

   #. Disconnect from the server.

      .. parsed-literal::
         :class: highlight

         **AT#XFTP="close"**
         221-Goodbye. You uploaded 1 and downloaded 0 kbytes.
         221 Logout.
         OK

.. _slm_testing_twi:

TWI AT commands
***************

Complete the following steps to test the functionality provided by the i2c sensors on the Thingy:91 using the two-wire interface (TWI):

1. Test the TWI list command using ``AT#XTWILS``.
   As Thingy:91 connects to the sensors via i2c2, it shows that TWI2 is available:

   ::

      AT#XTWILS
      #XTWILS: 2
      OK

2. Test the TWI write command using ``AT#XTWIW=2,"76","D0"``.
   It performs a write operation to the device address ``0x76`` (BME680), and it writes ``D0`` to the device:

   ::

      AT#XTWIW=2,"76","D0"
      OK

3. Test the TWI read command using ``AT#XTWIR=2,"76",1``.
   It performs a read operation to the device address ``0x76`` (BME680), and it reads 1 byte from the device:

   ::

      AT#XTWIR=2,"76",1

      #XTWIR: 61
      OK

   The value returned (``61``) indicates ``0x61`` as the ``CHIP ID``.

4. Test the TWI write-and-read command using ``AT#XTWIWR=2,"76","D0",1``.
   It performs a write-then-read operation to the device address ``0x76`` (BME680) to get the ``CHIP ID`` of the device:

   ::

      AT#XTWIWR=2,"76","D0",1

      #XTWIWR: 61
      OK

   The value returned (``61``) indicates ``0x61`` as the ``CHIP ID``.
