.. _lib_download_client:

Download client
###############

The download client library can be used to download files from an HTTP or HTTPS server. It supports IPv4 and IPv6 protocols.

The file is downloaded in fragments whose size can be configured independently for TLS and non-TLS connections (:option:`CONFIG_DOWNLOAD_CLIENT_MAX_TLS_FRAGMENT_SIZE` and :option:`CONFIG_DOWNLOAD_CLIENT_MAX_FRAGMENT_SIZE`).
These fragments are returned to the application via events (:cpp:member:`DOWNLOAD_CLIENT_EVT_FRAGMENT`).

The library can detect the size of the file that is downloaded and sends an event (:cpp:member:`DOWNLOAD_CLIENT_EVT_DONE`) to the application when the download has completed.

The library can detect when the server has closed the connection.
When it happens, it returns an event to the application with an appropriate error code.
The application can then resume the download by calling the :cpp:func:`download_client_connect` and :cpp:func:`download_client_start` functions again.

The download happens in a separate thread which can be paused and resumed.

Make sure to configure the fragment size in a way that suits your application.
A large fragment size requires more RAM, while a small fragment size results in more download requests, and thus a higher protocol overhead.
If the size of the file being downloaded is larger than a hundred times the size of one fragment, the server might close the HTTP connection
after the hundredth fragment has been transferred. The library is able to detect when the server has closed the HTTP connection
and reconnect automatically. Increasing the fragment size prevents having to establish several HTTP connections and thus helps
in keeping protocol overhead to a minimum.


Protocols
*********

The library supports HTTP and HTTPS (TLS 1.2) over IPv4 and IPv6.


HTTP
====

For HTTP, the following requirements must be met:

* The application protocol to communicate with the server is HTTP 1.1.
* IETF RFC 7233 is supported by the HTTP Server.
* :option:`CONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE` is configured so that it can contain the entire HTTP response.

.. _download_client_https:

HTTPS
=====

The library uses TLS version 1.2.
When using HTTPS, the application must provision the TLS credentials and pass the security tag to the library when calling :cpp:func:`download_client_connect`.

To provision a TLS certificate to the modem, use :cpp:func:`modem_key_mgmt_write` and other :ref:`modem_key_mgmt` APIs.
The following snippet illustrates how to provision a TLS certificate, associate it to a TLS security tag, and pass that tag to the library.

.. code::

	#include <modem_key_mgmt.h>
	#include <net/download_client.h>

	/* A TLS certificate, in PEM format.
	 *
	 * CN=DigiCert Baltimore CA-2 G2, OU=www.digicert.com, O=DigiCert Inc, C=US
	 *
	 * OpenSSL can be used to convert a standard format certificate (X.509)
	 * into a PEM format certificate.
	 *
	 * Note: each line must be terminated by a newline character '\n'.
	 * You may need to do this modification manually.
	 */
	static char certificate[] = {
		"-----BEGIN CERTIFICATE-----\n"
		"MIIEYzCCA0ugAwIBAgIQAYL4CY6i5ia5GjsnhB+5rzANBgkqhkiG9w0BAQsFADBaMQswCQYDVQQG\n"
		"EwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYDVQQDExlC\n"
		"YWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTE1MTIwODEyMDUwN1oXDTI1MDUxMDEyMDAwMFow\n"
		"ZDELMAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3LmRpZ2lj\n"
		"ZXJ0LmNvbTEjMCEGA1UEAxMaRGlnaUNlcnQgQmFsdGltb3JlIENBLTIgRzIwggEiMA0GCSqGSIb3\n"
		"DQEBAQUAA4IBDwAwggEKAoIBAQC75wD+AAFz75uI8FwIdfBccHMf/7V6H40II/3HwRM/sSEGvU3M\n"
		"2y24hxkx3tprDcFd0lHVsF5y1PBm1ITykRhBtQkmsgOWBGmVU/oHTz6+hjpDK7JZtavRuvRZQHJa\n"
		"Z7bN5lX8CSukmLK/zKkf1L+Hj4Il/UWAqeydjPl0kM8c+GVQr834RavIL42ONh3e6onNslLZ5QnN\n"
		"NnEr2sbQm8b2pFtbObYfAB8ZpPvTvgzm+4/dDoDmpOdaxMAvcu6R84Nnyc3KzkqwIIH95HKvCRjn\n"
		"T0LsTSdCTQeg3dUNdfc2YMwmVJihiDfwg/etKVkgz7sl4dWe5vOuwQHrtQaJ4gqPAgMBAAGjggEZ\n"
		"MIIBFTAdBgNVHQ4EFgQUwBKyKHRoRmfpcCV0GgBFWwZ9XEQwHwYDVR0jBBgwFoAU5Z1ZMIJHWMys\n"
		"+ghUNoZ7OrUETfAwEgYDVR0TAQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwNAYIKwYBBQUH\n"
		"AQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wOgYDVR0fBDMwMTAv\n"
		"oC2gK4YpaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL09tbmlyb290MjAyNS5jcmwwPQYDVR0gBDYw\n"
		"NDAyBgRVHSAAMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJ\n"
		"KoZIhvcNAQELBQADggEBAC/iN2bDGs+RVe4pFPpQEL6ZjeIo8XQWB2k7RDA99blJ9Wg2/rcwjang\n"
		"B0lCY0ZStWnGm0nyGg9Xxva3vqt1jQ2iqzPkYoVDVKtjlAyjU6DqHeSmpqyVDmV47DOMvpQ+2HCr\n"
		"6sfheM4zlbv7LFjgikCmbUHY2Nmz+S8CxRtwa+I6hXsdGLDRS5rBbxcQKegOw+FUllSlkZUIII1p\n"
		"LJ4vP1C0LuVXH6+kc9KhJLsNkP5FEx2noSnYZgvD0WyzT7QrhExHkOyL4kGJE7YHRndC/bseF/r/\n"
		"JUuOUFfrjsxOFT+xJd1BDKCcYm1vupcHi9nzBhDFKdT3uhaQqNBU4UtJx5g=\n"
		"-----END CERTIFICATE-----"
	};

	/* The host to connect to */
	#define HOST "s3.amazonaws.com"

	/* Download client instance */
	static struct download_client dl;

	int cert_provision_and_connect(void)
	{
		int err;

		/* TLS security tag, arbitrary */
		nrf_sec_tag_t sec_tag = 42;

		/* Provision CA Certificate to the modem.
		 * The certificate is stored in persistent memory, so
		 * it is not necessary to provision it again across reboots.
		 */
		err = modem_key_mgmt_write(sec_tag, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
							       certificate, sizeof(certificate) - 1);
		if (err) {
			return err;
		}

		/* Note:
		 * It is assumed, for simplicity, that the download_client library
		 * has already been initialized via download_client_init().
		 * You need to initialize it in your own application prior to
		 * calling download_client_connect().
		 */

		/* Specify the security tag in the configuration structure */
		download_client_cfg config = {
			.sec_tag = sec_tag,
		};

		err = download_client_connect(&dl, HOST, &config);
		if (err) {
			return err;
		}

		return 0;
	}


Limitations
***********

The library requires the host server to provide a Content-Range field in the HTTP GET header.
If this header field is missing, the library logs the following error::

   <err> download_client: Server did not send "Content-Range" in response

To debug your application when getting such an error, set the log level of the library to debug (:option:`CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL_DBG`) and select :option:`CONFIG_DOWNLOAD_CLIENT_LOG_HEADERS`.

API documentation
*****************

| Header file: :file:`include/download_client.h`
| Source files: :file:`subsys/net/lib/download_client/src/`

.. doxygengroup:: dl_client
   :project: nrf
   :members:
