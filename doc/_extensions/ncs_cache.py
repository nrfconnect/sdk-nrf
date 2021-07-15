"""
NCS Documentation Cache Extension
=================================

The ``ncs_cache`` utility can be used to enable the automatic download of
documentation cache files for a given docset. When enabled, pre-built files will
be downloaded before the build is started. It should result in reduced build
times.

** IMPORTANT **: This extension should be loaded before any other extensions
changing sources.

Usage
*****

The extension will only actuate if ``NCS_CACHE_ENABLE`` environment variable is
set. Note that cache will not be download if the repository of any of the
projects a docset depends on is not clean, i.e. has changes. You may still
force cache download in this situation if ``NCS_CACHE_FORCE`` environment
variable is set.

Others
******

A cache upload script (cache_upload.py) is also provided to upload cache files
to the cache server. Refer to the script for more details on the cache strategy
and details on the content of the cache files.

Options
*******

- ``ncs_cache_docset``: Docset name.
- ``ncs_cache_build_dir``: Build directory.
- ``ncs_cache_config``: Cache configuration file.
- ``ncs_cache_manifest``: Manifest file.

Copyright (c) 2021 Nordic Semiconductor ASA
"""

import hashlib
from io import BytesIO
import os
from pathlib import Path
import pickle
import zipfile
from typing import Dict, Any, Tuple

from azure.core.pipeline import PipelineResponse
from azure.storage.blob import ContainerClient
import pygit2
from sphinx.application import Sphinx, ENV_PICKLE_FILENAME
from sphinx.environment import BuildEnvironment, CONFIG_OK
from sphinx.util import logging
from sphinx.util.console import bold, term_width_line
from west.manifest import Manifest
import yaml

try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


__version__ = "0.1.0"


AZ_CONN_STR_PUBLIC = ";".join(
    (
        "DefaultEndpointsProtocol=https",
        "EndpointSuffix=core.windows.net",
        "AccountName=ncsdocsa",
    )
)
"""Azure connection string (public acces)."""

AZ_CONTAINER = "ncs-doc-container01"
"""Azure container."""

ENV_CACHE_PICKLE_FILENAME = (
    Path(ENV_PICKLE_FILENAME).stem + "-cache" + Path(ENV_PICKLE_FILENAME).suffix
)
"""Cache environment file."""


logger = logging.getLogger(__name__)


def is_repo_dirty(repo: pygit2.Repository) -> bool:
    """Check if a repository is dirty (not clean).

    Args:
        repo: Repository.

    Returns:
        True if dirty, False otherwise.
    """

    return bool(
        [f for f, code in repo.status().items() if code != pygit2.GIT_STATUS_IGNORED]
    )


def get_docset_props(docset: str, config: Dict, manifest: Manifest) -> Tuple[str, bool]:
    """Obtain the id and dirty status of the given docset.

    Args:
        docset: Docset name.
        config: Cache configuration.
        manifest: Repository manifest.

    Returns:
        Docset ID.
    """

    for entry in config:
        if entry["docset"] != docset:
            continue

        id = ""
        dirty = False
        for name in entry["projects"]:
            p = next((p for p in manifest.projects if p.name == name), None)
            assert p, f"Project {name} not in manifest"

            repo = pygit2.Repository(Path(p.topdir) / p.path)
            id += repo.revparse_single("HEAD").id.hex
            dirty = dirty or is_repo_dirty(repo)

        return hashlib.sha256(id.encode("utf-8")).hexdigest(), dirty

    raise ValueError(f"Docset {docset} not in configuration file")


def progress(response: PipelineResponse) -> None:
    """Download progress callback."""

    mb_tot = response.context["data_stream_total"] / (1024 * 1024)
    mb_curr = response.context["download_stream_current"] / (1024 * 1024)
    pct = int((mb_curr / mb_tot) * 100)
    s = term_width_line(
        bold("Downloading cache files... ")
        + f"{mb_curr:.2f} of {mb_tot:.2f} Mb [{pct} %]"
    )
    logger.info(s, nonl=pct < 100)


def make_absolute_set(items: Dict, basedir: Path) -> None:
    """Make relative paths ('*' prefix) absolute to basedir.

    Args:
        items: Dictionary with items -> set of entries
        basedir: Base directory.
    """

    for doc, set_entries in items.items():
        patched_set = set()
        for set_entry in set_entries:
            if set_entry and set_entry.startswith("*"):
                set_entry = str(basedir / set_entry[1:])
            patched_set.add(set_entry)
        items[doc] = patched_set


def download_cache(app: Sphinx) -> None:
    """Download cache files"""

    cc = ContainerClient.from_connection_string(AZ_CONN_STR_PUBLIC, AZ_CONTAINER)
    cache_files = [b.name for b in cc.list_blobs()]

    config = yaml.load(open(app.config.ncs_cache_config).read(), Loader=Loader)
    manifest = Manifest.from_file(app.config.ncs_cache_manifest)
    id, dirty = get_docset_props(app.config.ncs_cache_docset, config, manifest)

    if dirty and not bool(os.environ.get("NCS_CACHE_FORCE", False)):
        logger.info("Cache will not be downloaded (repository not clean)")
        return

    cache_file = f"{app.config.ncs_cache_docset}-{id}.zip"
    if cache_file not in cache_files:
        logger.info("Cache not available")
        return

    buf = BytesIO()
    bc = cc.get_blob_client(cache_file)
    bc.download_blob(raw_response_hook=progress).readinto(buf)

    logger.info("Uncompressing cache files...")
    with zipfile.ZipFile(buf, "r", zipfile.ZIP_DEFLATED) as zf:
        zf.extractall(app.config.ncs_cache_build_dir)

    # load cached environment
    env_file = Path(app.doctreedir) / ENV_CACHE_PICKLE_FILENAME
    with open(env_file, "rb") as f:
        env_cache = pickle.load(f)

    app.env.all_docs = env_cache["all_docs"]
    app.env.dependencies = env_cache["dependencies"]
    app.env.domaindata = env_cache["domaindata"]
    app.env.files_to_rebuild = env_cache["files_to_rebuild"]
    app.env.included = env_cache["included"]
    app.env.longtitles = env_cache["longtitles"]
    app.env.metadata = env_cache["metadata"]
    app.env.titles = env_cache["titles"]
    app.env.toc_num_entries = env_cache["toc_num_entries"]
    app.env.tocs = env_cache["tocs"]

    if "doxyrunner_cache" in env_cache:
        app.env.doxyrunner_cache = env_cache["doxyrunner_cache"]

    app.env.config_status = CONFIG_OK

    # make relative paths absolute again
    make_absolute_set(app.env.included, Path(manifest.topdir))
    make_absolute_set(app.env.dependencies, Path(manifest.topdir))

    # adjust sources modification time
    for file, mtime in app.env.all_docs.items():
        for dep in app.env.dependencies[file]:
            dep_path = Path(app.srcdir) / dep
            if dep_path.exists():
                dep_mtime = dep_path.stat().st_mtime
                if dep_mtime > mtime:
                    mtime = dep_mtime

        for suffix in app.env.config.source_suffix.keys():
            doc = Path(app.srcdir) / (file + suffix)
            if doc.exists():
                doc_mtime = doc.stat().st_mtime
                if doc_mtime > mtime:
                    mtime = doc_mtime
                break

        app.env.all_docs[file] = mtime

    # re-create HTML builder info
    if app.builder.name == "html":
        with open(Path(app.outdir) / ".buildinfo", "w") as f:
            info = app.builder.create_build_info()
            info.dump(f)


def save_env(app: Sphinx, env: BuildEnvironment) -> None:
    """Save environment."""
    with open(Path(app.doctreedir) / ENV_PICKLE_FILENAME, "wb") as f:
        pickle.dump(env, f)


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_config_value("ncs_cache_docset", None, "env")
    app.add_config_value("ncs_cache_build_dir", None, "env")
    app.add_config_value("ncs_cache_config", None, "env")
    app.add_config_value("ncs_cache_manifest", None, "env")

    if not bool(os.environ.get("NCS_CACHE_ENABLE", False)):
        logger.info("Cache disabled, set NCS_CACHE_ENABLE to enable it")
    else:
        app.connect("builder-inited", download_cache)
        app.connect("env-updated", save_env)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
