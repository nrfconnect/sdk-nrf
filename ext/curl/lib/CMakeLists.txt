#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_compile_definitions(HAVE_CONFIG_H CURL_NO_OLDIES)

# List of files is copied from Makefile.inc including line breaks and file order.
# This allows easy comparison to source repository version in case updates are made.
zephyr_library_sources(
	altsvc.c amigaos.c asyn-ares.c asyn-thread.c base64.c
	conncache.c connect.c content_encoding.c cookie.c curl_addrinfo.c
	curl_ctype.c curl_des.c curl_endian.c curl_fnmatch.c curl_get_line.c
	curl_gethostname.c curl_gssapi.c curl_memrchr.c curl_multibyte.c
	curl_ntlm_core.c curl_ntlm_wb.c curl_path.c curl_range.c curl_rtmp.c
	curl_sasl.c curl_sspi.c curl_threads.c dict.c dotdot.c easy.c escape.c
	file.c fileinfo.c formdata.c ftp.c url.c ftplistparser.c getenv.c getinfo.c
	gopher.c hash.c hmac.c hostasyn.c hostcheck.c hostip.c hostip4.c hostip6.c
	hostsyn.c http.c http2.c http_chunks.c http_digest.c http_negotiate.c
	http_ntlm.c http_proxy.c idn_win32.c if2ip.c imap.c inet_ntop.c inet_pton.c
	krb5.c ldap.c llist.c md4.c md5.c memdebug.c mime.c mprintf.c mqtt.c
	multi.c netrc.c non-ascii.c nonblock.c openldap.c parsedate.c pingpong.c
	pop3.c progress.c psl.c doh.c rand.c rename.c rtsp.c select.c
	sendf.c setopt.c sha256.c share.c slist.c smb.c smtp.c socketpair.c socks.c
	socks_gssapi.c socks_sspi.c speedcheck.c splay.c strcase.c strdup.c
	strerror.c strtok.c strtoofft.c system_win32.c telnet.c tftp.c timeval.c
	transfer.c urlapi.c version.c warnless.c wildcard.c x509asn1.c dynbuf.c
	version_win32.c easyoptions.c easygetopt.c
)
zephyr_library_sources(
	vtls/bearssl.c vtls/gskit.c vtls/gtls.c vtls/keylog.c
	vtls/mbedtls.c vtls/mbedtls_threadlock.c vtls/mesalink.c vtls/nss.c
	vtls/openssl.c vtls/schannel.c vtls/schannel_verify.c vtls/sectransp.c
	vtls/vtls.c vtls/wolfssl.c)
