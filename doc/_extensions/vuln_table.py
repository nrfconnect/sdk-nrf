"""
Copyright (c) 2022 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

This extension will produce generated content on known vulnerabilities.
Information on the vulnerabilities is downloaded from a cache in Azure,
and stored in the build environment.
"""

from docutils import nodes
from sphinx.util.docutils import SphinxDirective
from sphinx.application import Sphinx
from azure.storage.blob import ContainerClient
from sphinx.util import logging
import requests
import json

from page_filter import FilterDropdown, read_versions

__version__ = "0.1.0"

AZ_CONN_STR_PUBLIC = ";".join(
    (
        "DefaultEndpointsProtocol=https",
        "EndpointSuffix=core.windows.net",
        "AccountName=ncsdocsa",
    )
)
"""Azure connection string (public acces)."""

AZ_CONTAINER = "ncs-doc-vuln"
"""Azure container."""

CVE_DATABASE_URL = "https://cve.mitre.org/cgi-bin/cvename.cgi?name="
"""Incomplete CVE database URL, missing CVE ID"""

logger = logging.getLogger(__name__)


def find_affected_versions(firstv, fixv, versions):
    """List versions from first affected (inclusive) to fixed (exclusive)"""

    firstv = f"v{firstv.replace('.', '-')}"
    fixv = f"v{fixv.replace('.', '-')}"

    if firstv not in versions:
        return []

    affected = [firstv]
    for i in range(versions.index(firstv) + 1, len(versions)):
        if versions[i] == fixv:
            break
        affected.append(versions[i])

    return affected


def create_cve_links(cve_ids):
    ids = cve_ids.replace(",", " ").split()

    refs = []
    for id in ids:
        para = nodes.paragraph()
        refuri = CVE_DATABASE_URL + id
        para += nodes.reference(text=id, refuri=refuri)
        refs.append(para)
    return refs


class VulnTable(SphinxDirective):
    def create_reference(self, vuln):
        """Create a reference and a corresponding target node"""

        advisory_id = str(self.env.new_serialno("vuln") + 1).zfill(5)
        vuln["Advisory ID"] = advisory_id
        target_id = f"vuln-{advisory_id}"
        target_node = nodes.target("", "", ids=[target_id])
        vuln["target_node"] = target_node

        refuri = self.env.app.builder.get_relative_uri("", self.env.docname)
        refuri += "#" + target_id
        ref = nodes.reference(text=advisory_id, internal=True)
        ref["refdocname"] = self.env.docname
        ref["refuri"] = refuri
        return ref

    def run(self):
        if not hasattr(self.env, "vuln_cache"):
            return [nodes.paragraph(text="No vulnerabilities found")]

        cache = self.env.vuln_cache
        keys = cache["schema"]
        table = nodes.table()
        tgroup = nodes.tgroup(cols=len(keys))
        for key in keys:
            if key in ["First affected version", "Fixed version", "CVSS"]:
                colspec = nodes.colspec(colwidth=0.3)
            elif key in ["CVE ID"]:
                colspec = nodes.colspec(colwidth=1.5)
            else:
                colspec = nodes.colspec(colwidth=1)
            tgroup.append(colspec)
        table += tgroup

        # table head row
        thead = nodes.thead()
        tgroup += thead
        row = nodes.row()

        for key in keys:
            entry = nodes.entry()
            entry += nodes.paragraph(text=key)
            row += entry
        thead.append(row)

        # table body
        rows = []
        for vuln in cache["body"]:
            row = nodes.row()
            version_classes = find_affected_versions(
                vuln["First affected version"],
                vuln["Fixed version"],
                self.env.nrf_versions,
            )
            vuln["affected_version_classes"] = version_classes
            row["classes"].extend(version_classes)
            component_class_name = vuln["Component"].replace(" ", "_").lower()
            row["classes"].append(component_class_name)
            row["classes"].append("hideable")
            rows.append(row)

            for key in keys:
                entry = nodes.entry()
                para = nodes.paragraph()
                # Advisory ID links to the corresponding detailed section
                if key == "Advisory ID":
                    para += self.create_reference(vuln)
                # CVE column links to database
                elif key == "CVE ID":
                    para += create_cve_links(vuln[key])
                else:
                    para += nodes.Text(vuln[key])
                entry += para
                row += entry

        tbody = nodes.tbody()
        tbody.extend(rows)
        tgroup += tbody

        # Include a component filter
        all_components = {vuln["Component"] for vuln in cache["body"]}
        create_tuple = lambda c: (c.replace(" ", "_").lower(), c)
        content = list(map(create_tuple, all_components))
        component_select_node = FilterDropdown("components", content)

        return [component_select_node, table]


def create_list_item(title, value):
    """Return a list item on the format <strong>title: </strong>value"""

    para = nodes.paragraph()
    para += nodes.strong(text=title + ": ")
    para += nodes.Text(value)
    return nodes.list_item("", para)


def create_bullet_list(vuln):
    """Return a bullet list of all known information about a vulnerability"""

    bulletlist = nodes.bullet_list()
    for description, value in vuln.items():
        if description in ["Summary", "target_node", "affected_version_classes"]:
            continue
        bulletlist += create_list_item(description, value)

    return bulletlist


class VulnDetails(SphinxDirective):
    def run(self):
        if not hasattr(self.env, "vuln_cache"):
            return []

        vulnerabilities = self.env.vuln_cache["body"]
        for vuln in vulnerabilities:
            bulletlist = create_bullet_list(vuln)

            self.state.section(
                title=vuln["Summary"],
                source="",
                style="",
                lineno=self.lineno,
                messages=[vuln["target_node"], bulletlist],
            )

            self.state.parent[-1]["classes"].extend(vuln["affected_version_classes"])
            component_class_name = vuln["Component"].replace(" ", "_").lower()
            self.state.parent[-1]["classes"].append(component_class_name)

        return []


def download_vuln_table(app: Sphinx) -> None:
    """Download vulnerability table from Azure"""

    # Check if local cache exists
    if hasattr(app.env, "vuln_cache"):
        local_cache = app.env.vuln_cache
    else:
        logger.info("No vulnerability cache found locally")
        local_cache = None

    # Check internet connection
    try:
        requests.get("https://ncsdocsa.blob.core.windows.net", timeout=3)
    except (requests.ConnectionError, requests.Timeout):
        logger.info("Could not retrieve vulnerability information online")
        if local_cache:
            logger.info("Using local vulnerability cache")
            app.env.vuln_cache = local_cache
        return

    cc = ContainerClient.from_connection_string(AZ_CONN_STR_PUBLIC, AZ_CONTAINER)
    remote_files = sorted(
        [b for b in cc.list_blobs()], key=lambda b: b.last_modified, reverse=True
    )

    target = remote_files[0]
    if local_cache and local_cache["md5"] == target.content_settings["content_md5"]:
        logger.info("Up to date vulnerability table found in cache")
        return

    bc = cc.get_blob_client(target)
    res = bc.download_blob().content_as_text()
    app.env.vuln_cache = json.loads(res)
    app.env.vuln_cache["md5"] = target.content_settings["content_md5"]
    logger.info("Vulnerability table cached locally")


def setup(app: Sphinx):
    app.add_directive("vuln-table", VulnTable)
    app.add_directive("vuln-details", VulnDetails)

    app.connect("builder-inited", download_vuln_table)
    app.connect("builder-inited", read_versions)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
