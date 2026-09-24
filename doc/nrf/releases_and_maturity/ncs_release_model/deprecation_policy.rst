.. _ncs_deprecation_policy:

Deprecation policy
##################

.. contents::
   :local:
   :depth: 2

This page describes how deprecated features and APIs are managed in the |NCS|.
It explains the deprecation period, notification requirements, and how deprecation applies depending on whether you follow ``main``, a :ref:`standard release <standard_releases>`, or an :ref:`LTS release <lts_releases>`.

For how deprecated APIs are classified in the software maturity model, see :ref:`api_deprecation`.
For LTS branch handling of deprecated features, see :ref:`deprecated_features_in_lts_branches`.

User workflows
**************

The following sections describe how deprecation affects you depending on which |NCS| revision you follow.

Following ``main``
==================

In the ``sdk-nrf`` repository, features, modules, samples, APIs, and configuration options can be deprecated as the |NCS| evolves.
Deprecations are communicated in official :ref:`release_notes`.

The default deprecation period is **six months (two quarters)** from the announcement, replacing the previous policy of two release cycles.
After this period, the deprecated feature is removed from the SDK unless an exception applies.

.. include:: /includes/deprecation_common.txt

The deprecation period may be shortened or removed immediately to accommodate an |NCS| release schedule or for security reasons.
When an exception applies, the change is documented in the relevant :ref:`release_notes` and :ref:`migration_guides`.

Standard (non-LTS) releases
=============================

Deprecated features and APIs are maintained for as long as the end of life (EOL) of the SDK version.

LTS releases
============

No future LTS release shall ship with deprecated APIs or features.
For how deprecated features are handled on an LTS branch, see :ref:`deprecated_features_in_lts_branches`.
