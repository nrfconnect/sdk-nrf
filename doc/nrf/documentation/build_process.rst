.. _doc_build_process:

Documentation build process
###########################

.. contents::
   :local:
   :depth: 2

Whenever the main branch of the `sdk-nrf`_ repository is updated or a pull request is made, a new version of the documentation is built and published. The following section describes the steps involved with building and publishing the documentation.

Building with Sphinx
********************
The |NCS| documentation uses the Sphinx framework to build the docsets for HTML.
Sphinx primarily uses two files to work:

* An entry point that serves as the top of the ToC tree and the root of the documentation (often named :file:`index.rst`).
* A Python file :file:`conf.py` in which all further configuration of Sphinx is placed.

The command :code:`sphinx-build` then needs to be run with the appropriate arguments.
As with the rest of the |NCS|, this is handled by CMake.

CMake
-----
CMake not only creates a generator used to build the documentation, it also checks for the required binaries and dependencies and ensures that the docsets are built in the correct order.
:file:`nrf/doc/CMakeLists.txt` contains the function :code:`add_docset` which is used to create targets for all the docsets.
This includes the :code:`{docset}`/ :code:`{docset}-all`, :code:`{docset}-inventory`/ :code:`{docset}-inventory-all`, :code:`{docset}-linkcheck` and :code:`{docset}-clean` targets.
The inventory targets create reference targets without building the documentation, providing a solution for any circular dependencies.
Some docsets require additional source files to be generated before the docset can be built, like :ref:`Zephyr's device tree<devicetree-intro>`.
In these cases a separate target is created and added as a dependency for the docset target.

External contents
-----------------
When :code:`sphinx-build` is executed, a :file:`_build/\\{docset\\}/src` directory is used as the source directory.
The :file:`external_content` extension executes before the build is initiated and copies every relevant documentation file into this folder.
This is a "smart" copy, meaning that only new or modified files are copied to reduce build time.
As this can change the file hierarchy somewhat, local link paths are modified during this process.
Which files are copied is listed under :code:`external_content_contents` in the :file:`conf.py` file.

Intersphinx
-----------
The |NCS| documentation consists of multiple docsets.
The main docset is the nRF documentation, while the other docsets are pulled from their upstream versions.
The only file that is needed to maintain for this, is a customized :file:`conf.py` file located under :file:`doc/\\{docset-name\\}/`.
To be able to make references across docsets, the Sphinx extension Intersphinx is used, and local paths are configured under :code:`intersphinx_mapping` in :file:`doc/nrf/conf.py`.
The upstream docsets are pulled using west.

Building and publishing the documentation
*****************************************
The |NCS| documentation is currently hosted on ``developer.nordicsemi.com`` under the folder :file:`nRF_Connect_SDK` and is updated with every pull request that is merged into the main branch of the Git repository.
Every pull request made is also hosted under the :file:`nRF_Connect_SDK_dev` folder if the build workflow is successful.
This is done through two chained GitHub actions, :file:`docbuild.yml` and :file:`docpublish.yml`.

Docbuild
--------
Whenever a pull request is created the `docbuild`_ workflow is triggered.
This will checkout all the relevant repositories with west, install the necessary dependencies and build the documentation with CMake.
After the documentation is built a cache file is created using :file:`doc/_scripts/cache_create.py` which can be used locally to speed up builds.

Docpublish
----------
A successful completion of the docbuild workflow will trigger the `docpublish`_ workflow.
It retrieves the artifacts from the previous workflow, uploads the cache files with :file:`doc/_scripts/cache_upload.py` and then uploads the Sphinx-generated html files of the documentation to ``developer.nordicsemi.com`` where they are served.
A link to the preview page is then added for pull requests.

Caching
-------
A cache is created in the docbuild workflow and uploaded in the docpublish workflow.
When a build is initiated and caching is enabled, the builder will first check if the current version of the documentation is available in the cache and use it if it is.
Caching is not enabled in the GitHub workflows, as they always process new versions to build.
