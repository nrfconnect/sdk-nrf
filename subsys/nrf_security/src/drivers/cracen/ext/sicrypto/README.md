# SiCrypto

A high level crypto library for Silex Insight hardware offload.


## Introduction

Silex Insight provides powerful hardware offload for symmetric cryptography
and for asymmetric (public-key) cryptography. The sxsymcrypt and SilexPK
libraries provide convenient and asynchronous software interfaces to those
offload modules.

The hardware offload works at a very low level. The sxsymcrypt and
SilexPK libraries give a simple access to that hardware offload. But
often, that offload does not perform all the steps required by the
cryptographic protocols and standards. Sometimes, a single operation
involves multiple types of hardware offloads, for example signing a
message involves hashing and mathematical computations with the private
key.

This library, called sicrypto, complements and builds upon SilexPK and
sxsymcrypt to provide higher level interfaces to perform popular crypto
operations.

The API has a native asynchronous non-blocking interface. But it can
also be used very easily in a blocking synchronous way.
