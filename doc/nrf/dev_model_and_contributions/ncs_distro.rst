.. _dm_ncs_distro:

Redistributing the |NCS|
########################

.. contents::
   :local:
   :depth: 2

This section is intended for users that would like to re-distribute the |NCS| with additions or changes to it.
It can be useful if you are a software or hardware vendor that would like to augment the |NCS| with additional functionality, and still benefit from the infrastructure it provides.

The contents of this section is not comprehensive.
You can rather consider it as suggestions for extending the |NCS|.
You can use other mechanisms as long as they comply with the text in the :file:`LICENSE` file located in the root of every Git repository in the |NCS|.
If you need any help, reach out to Nordic Semiconductor.

Getting added to the main manifest
**********************************

The simplest way to become part of the |NCS| is to be included in its official `west manifest file`_.
This way, your own :ref:`module repository <zephyr:modules>` will automatically be part of the |NCS| official distribution and will be included in all releases.

This solution is reserved for selected vendors and requires a commercial agreement with Nordic Semiconductor.
Contact your local representative if you wish to be considered for inclusion.

Some examples of this type of integration are:

* :ref:`Memfault <mod_memfault>`.
  This functionality is present through the addition of a `west project in the nRF Connect SDK manifest <Memfault entry in the manifest_>`_.
  The `repository provided by Memfault <Memfault firmware SDK_>`_ is public.
  By default, |NCS| users will obtain a copy of it (as well as the integration code necessary for |NCS| based applications) when :ref:`getting the code <cloning_the_repositories>`.

* ANT protocol support.
  The code and documentation of this protocol is confidential, and access to the private module repository requires the GitHub user to be added to it.
  This is why the west project is disabled by default, using the :ref:`project groups <zephyr:west-manifest-groups>` feature of west.
  This means that the ANT entry in the manifest is disabled by default through its presence in the `manifest group filter`_.
  You will not get a copy of this repository when :ref:`getting the code <cloning_the_repositories>`, and instead, need to enable the repository first using west itself to fetch it locally::

    west config manifest.group-filter +ant
    west update

Providing a Zephyr module
*************************

This approach is essentially identical to the previous one.
However, the :ref:`module repository <zephyr:modules>` you create will not be part of the |NCS| official distribution by default.
Instead, you will need to host your own documentation explaining how to add the module you make available to user's :ref:`own manifest <dm_workflow_4>`.

An example of this approach is the `Golioth Firmware SDK`_, which is set up as a `Zephyr module <Golioth module.yml_>`_ that a user can add to their own manifest.

Providing a west manifest
*************************

Instead of just providing a module repository like in the previous approach, you can also include a west manifest that imports the |NCS| in it, see :ref:`dm_workflow_4`.
With this approach, you can import your manifest directly.
It will then in turn import the |NCS| `west manifest file`_.

A good starting point for such an approach is the Nordic-provided `ncs-example-application`_ repository.
It contains all the necessary integration with |NCS| and example functionality (a driver, library, subsystem, test, etc.) to serve as a reference for your own distribution's manifest repository.
Just like in the previous approach, Golioth provides an example of such a west manifest, `west-ncs.yml <Golioth west-ncs.yml_>`_ along with a `guide document <Golioth Firmware SDK NCS doc_>`_.

Forking the |NCS|
*****************

You can always :ref:`fork all or parts of the nRF Connect SDK <dm-wf-fork>` and then make all the changes required by your features.
