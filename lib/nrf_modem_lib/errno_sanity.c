/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <nrf_errno.h>
#include <toolchain.h>

BUILD_ASSERT(EPERM           == NRF_EPERM,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOENT          == NRF_ENOENT,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ESRCH           == NRF_ESRCH,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EINTR           == NRF_EINTR,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EIO             == NRF_EIO,             "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENXIO           == NRF_ENXIO,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(E2BIG           == NRF_E2BIG,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOEXEC         == NRF_ENOEXEC,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EBADF           == NRF_EBADF,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ECHILD          == NRF_ECHILD,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EAGAIN          == NRF_EAGAIN,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOMEM          == NRF_ENOMEM,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EACCES          == NRF_EACCES,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EFAULT          == NRF_EFAULT,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EBUSY           == NRF_EBUSY,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EEXIST          == NRF_EEXIST,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EXDEV           == NRF_EXDEV,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENODEV          == NRF_ENODEV,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTDIR         == NRF_ENOTDIR,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EISDIR          == NRF_EISDIR,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EINVAL          == NRF_EINVAL,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENFILE          == NRF_ENFILE,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EMFILE          == NRF_EMFILE,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTTY          == NRF_ENOTTY,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ETXTBSY         == NRF_ETXTBSY,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EFBIG           == NRF_EFBIG,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOSPC          == NRF_ENOSPC,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ESPIPE          == NRF_ESPIPE,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EROFS           == NRF_EROFS,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EMLINK          == NRF_EMLINK,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EPIPE           == NRF_EPIPE,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EDOM            == NRF_EDOM,            "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ERANGE          == NRF_ERANGE,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOMSG          == NRF_ENOMSG,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EDEADLK         == NRF_EDEADLK,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOLCK          == NRF_ENOLCK,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOSTR          == NRF_ENOSTR,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENODATA         == NRF_ENODATA,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ETIME           == NRF_ETIME,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOSR           == NRF_ENOSR,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EPROTO          == NRF_EPROTO,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EBADMSG         == NRF_EBADMSG,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOSYS          == NRF_ENOSYS,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTEMPTY       == NRF_ENOTEMPTY,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENAMETOOLONG    == NRF_ENAMETOOLONG,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ELOOP           == NRF_ELOOP,           "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EOPNOTSUPP      == NRF_EOPNOTSUPP,      "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ECONNRESET      == NRF_ECONNRESET,      "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOBUFS         == NRF_ENOBUFS,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EAFNOSUPPORT    == NRF_EAFNOSUPPORT,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EPROTOTYPE      == NRF_EPROTOTYPE,      "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTSOCK        == NRF_ENOTSOCK,        "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOPROTOOPT     == NRF_ENOPROTOOPT,     "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ECONNREFUSED    == NRF_ECONNREFUSED,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EADDRINUSE      == NRF_EADDRINUSE,      "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ECONNABORTED    == NRF_ECONNABORTED,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENETUNREACH     == NRF_ENETUNREACH,     "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENETDOWN        == NRF_ENETDOWN,        "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ETIMEDOUT       == NRF_ETIMEDOUT,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EHOSTUNREACH    == NRF_EHOSTUNREACH,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EINPROGRESS     == NRF_EINPROGRESS,     "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EALREADY        == NRF_EALREADY,        "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EDESTADDRREQ    == NRF_EDESTADDRREQ,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EMSGSIZE        == NRF_EMSGSIZE,        "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EPROTONOSUPPORT == NRF_EPROTONOSUPPORT, "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EADDRNOTAVAIL   == NRF_EADDRNOTAVAIL,   "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENETRESET       == NRF_ENETRESET,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EISCONN         == NRF_EISCONN,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTCONN        == NRF_ENOTCONN,        "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ENOTSUP         == NRF_ENOTSUP,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EILSEQ          == NRF_EILSEQ,          "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EOVERFLOW       == NRF_EOVERFLOW,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ECANCELED       == NRF_ECANCELED,       "Errno not aligned with nrf_errno.");

BUILD_ASSERT(EAGAIN          == NRF_EWOULDBLOCK,     "Errno not aligned with nrf_errno.");

/* Not in IEEE Std 1003.1-2017 */
BUILD_ASSERT(ENOTBLK         == NRF_ENOTBLK,         "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EPFNOSUPPORT    == NRF_EPFNOSUPPORT,    "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ESHUTDOWN       == NRF_ESHUTDOWN,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(EHOSTDOWN       == NRF_EHOSTDOWN,       "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ESOCKTNOSUPPORT == NRF_ESOCKTNOSUPPORT, "Errno not aligned with nrf_errno.");
BUILD_ASSERT(ETOOMANYREFS    == NRF_ETOOMANYREFS,    "Errno not aligned with nrf_errno.");
