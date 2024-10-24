"""
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension will produce generated content on known vulnerabilities.
Information on the vulnerabilities is downloaded from a cache in Azure,
and stored in the build environment.
"""

from typing import List, Union
from docutils import nodes
from sphinx.util.docutils import SphinxDirective
from sphinx.application import Sphinx
from azure.storage.blob import ContainerClient
from pathlib import Path
from sphinx.util import logging
from docutils.core import publish_doctree
from docutils.nodes import system_message
import io
import requests
import json
import re

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

AZ_CONTAINER = "ncs-doc-generated-reports"
"""Azure container."""

REPORT_PREFIX = "vuln_reports"
"""Prefix of all vulnerability reports in the Azure container."""

CVE_DATABASE_URL = "https://cve.mitre.org/cgi-bin/cvename.cgi?name="
"""Incomplete CVE database URL, missing CVE ID"""

logger = logging.getLogger(__name__)


def area_str(name: str) -> str:
    """Transform an area name into a html area identifier.

    Args:
        name: The area name.

    Returns:
        An area identifier usable as a html class.
    """
    return name.replace(" ", "_").replace(".", "-").lower() + "_area"


class VulnTable(SphinxDirective):
    """This class creates a new directive ``vuln-table``.

    The table displays summary information on vulnerabilities retrieved from
    a cache in Azure.

    This class uses funcionality from the ``PageFilter`` class.
    """

    @staticmethod
    def create_cve_links(cve_ids: str) -> List[nodes.Element]:
        """Transform CVE IDs to links to the CVE database.

        Args:
            cve_ids: Comma separated string of CVE IDs.

        Returns:
            A list of links.
        """

        ids = cve_ids.split(",")

        refs = []
        for id in ids:
            para = nodes.paragraph()
            if re.match(r"CVE-\d+-\d+", id):
                refuri = CVE_DATABASE_URL + id
                para += nodes.reference(text=id, refuri=refuri)
            else:
                para += nodes.Text(id)
            refs.append(para)
        return refs

    def create_reference(self, vuln: dict) -> nodes.Element:
        """Create a reference and a corresponding target node.

        A unique "vulnerability ID" is associated with the vulnerability, and a link
        is created to its details section.

        Args:
            vuln: Data on the vulnerability.

        Returns:
            A reference object to the target section, containing the vulnerability
            ID as text.
        """

        refuri = self.env.app.builder.get_relative_uri("", Path(self.env.docname).name)
        refuri += "#" + nodes.make_id(vuln["Summary"])
        ref = nodes.reference(text=vuln["Vulnerability ID"], internal=True)
        ref["refdocname"] = self.env.docname
        ref["refuri"] = refuri
        return ref

    def run(self) -> List[nodes.Element]:
        """Create a filterable table displaying information on vulnerabilities.

        Returns:
            An area filter and the table.
        """

        if not hasattr(self.env, "vuln_cache"):
            return [nodes.paragraph(text="No vulnerabilities found")]

        cache = self.env.vuln_cache
        keys = cache["schema"]
        table = nodes.table()
        tgroup = nodes.tgroup(cols=len(keys))
        for key in keys:
            if key in ["Fix versions", "CVSS", "Repositories"]:
                colspec = nodes.colspec(colwidth=0.2)
            elif key in ["CVE ID", "Affected versions"]:
                colspec = nodes.colspec(colwidth=2)
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
        vformat = lambda version: f"v{version.replace('.', '-')}"
        for vuln in cache["body"]:
            row = nodes.row()
            version_classes = list(map(vformat, vuln["Affected versions"]))
            vuln["affected_version_classes"] = version_classes
            row["classes"].extend(version_classes)
            area_class_names = [area_str(name) for name in vuln["Area"]]
            row["classes"].extend(area_class_names)
            row["classes"].append("hideable")
            rows.append(row)

            for key in keys:
                entry = nodes.entry()
                para = nodes.paragraph()
                # Vulnerability ID links to the corresponding detailed section
                if key == "Vulnerability ID":
                    para += self.create_reference(vuln)
                # CVE column links to database
                elif key == "CVE ID":
                    para += self.create_cve_links(vuln[key])
                elif isinstance(vuln[key], list):
                    para += nodes.Text(", ".join(vuln[key]))
                else:
                    para += nodes.Text(vuln[key])
                entry += para
                row += entry

        tbody = nodes.tbody()
        tbody.extend(rows)
        tgroup += tbody

        # Include an area filter
        all_areas = {area for vuln in cache["body"] for area in vuln["Area"]}
        create_tuple = lambda c: (area_str(c), c)
        content = list(map(create_tuple, all_areas))
        area_select_node = FilterDropdown("areas", content)

        return [area_select_node, table]


class VulnDetails(SphinxDirective):
    """This class creates a new directive ``vuln-details``.

    Bulletlist sections detailing the available vulnerabilities are inserted
    into the page, and are filterable on areas and affected version
    number.
    """

    @staticmethod
    def create_description(text: str) -> nodes.Element:
        """Look for and transform links in the description.

        Links are on the following format:

            [displayed text|https://link.to.site|]

        The | on the end is optional. If no links are found,
        ``create_list_item`` is used instead.

        Args:
            text: CVE dexcription.

        Returns:
            A list item containing ``text`` with any links transformed.
        """

        url_reg = r"(https?://[^\s/$.?#].[^\s]*?)"
        link_reg = r"(\[([^|]+?)\|" + url_reg + r"(?:\|)?\])"
        locate_link_reg = r"^(.*?)" + link_reg + r"(.*)"

        # RE groups:
        #     1. Text before link start
        #     2. Entire link               [displayed text|https://link.to.site|]
        #     3. Displayed text of link    displayed text
        #     4. URL of link               https://link.to.site
        #     5. Text after link end

        if not re.search(link_reg, text):
            return VulnDetails.create_list_item("Description", text)

        para = nodes.paragraph()
        para += nodes.strong(text="Description: ")

        match = re.search(locate_link_reg, text)
        while match:
            para += nodes.Text(match.group(1))
            para += nodes.reference(
                text=match.group(3), refuri=match.group(4), internal=False
            )
            text = match.group(5)
            match = re.search(locate_link_reg, text)

        para += nodes.Text(text)
        return nodes.list_item("", para)

    @staticmethod
    def create_list_item(title: str, value: Union[str, List[str]]) -> nodes.Element:
        """Return a list item on the format <strong>title: </strong>value.

        If the value is a list, it is transformed to a comma separated string.

        Args:
            title: Title of the list item, put in bold.
            value: The list item text.
        """

        para = nodes.paragraph()
        para += nodes.strong(text=title + ": ")
        if isinstance(value, list):
            value = ", ".join(value)
        para += nodes.Text(value)
        return nodes.list_item("", para)

    @staticmethod
    def create_cve_bulletlist(ids: str) -> nodes.Element:
        """Return a list item containing a link or a sublist of links.

        Args:
            ids: Comma separated string of CVE IDs.

        Returns:
            A list item with the CVE IDs transformed to links.
        """

        para = nodes.paragraph()
        para += nodes.strong(text="CVE ID: ")
        for id in ids.split(","):
            if re.match(r"CVE-\d+-\d+", id):
                refuri = CVE_DATABASE_URL + id
                para += nodes.reference(text=id, refuri=refuri)
                para += nodes.Text("  ")
            else:
                para += nodes.Text(id + "  ")
        return nodes.list_item("", para)

    @staticmethod
    def create_bullet_list(vuln: dict) -> nodes.Element:
        """Return a bullet list of all known information about a vulnerability.

        Args:
            vuln: Data and metadata on the vulnerability.

        Returns:
            A bullet list object with containing all information except the
            metadata.
        """

        bulletlist = nodes.bullet_list()
        for description, value in vuln.items():
            if description in ["Summary", "affected_version_classes"]:
                continue
            elif not value:
                continue
            elif description == "CVE description":
                bulletlist += VulnDetails.create_description(value)
            elif description == "CVE ID":
                bulletlist += VulnDetails.create_cve_bulletlist(value)
            else:
                bulletlist += VulnDetails.create_list_item(description, value)

        return bulletlist

    @staticmethod
    def fix_formatting(string: str, fallback: str) -> str:
        """Transform a string into valid RST.

        Args:
            string: Original RST string.
            fallback: Fallback string if the original cannot be displayed.

        Returns:
            A valid RST string.
        """

        # Check for formatting issues
        def has_issues(rst: str) -> bool:
            tree = publish_doctree(
                rst, settings_overrides={"warning_stream": io.StringIO()}
            )
            return any([isinstance(child, system_message) for child in tree.children])

        if not has_issues(string):
            return string

        # Remove last occurrence of odd enclosing formatting elements
        for element in ["``", "`", "**", "*"]:
            if string.count(element) % 2 != 0:
                string = "".join(string.rsplit(element))
        if not has_issues(string):
            return string

        # Convert title to fixed-space literal
        string = f"``{string}``"
        if not has_issues(string):
            return string

        return fallback

    def run(self) -> List[nodes.Element]:
        """Insert vulnerability details sections into the page.

        Information on the vulnerabilities must be present in
        ``self.env.vuln_cache`` to work.

        The inserted sections will have affected versions and areas as
        class names so that they can be filtered.

        Returns:
            An empty list. This extension only alters the existing page.
        """

        if not hasattr(self.env, "vuln_cache"):
            return []

        vulnerabilities = self.env.vuln_cache["body"]
        vuln_by_years = {}
        for vuln in vulnerabilities:
            year = vuln["Vulnerability ID"][:4]
            if year in vuln_by_years:
                vuln_by_years[year].append(vuln)
            else:
                vuln_by_years[year] = [vuln]

        for year, vuln_list in sorted(vuln_by_years.items(), reverse=True):
            year_section = []
            for vuln in vuln_list:
                classes = vuln["affected_version_classes"] + [
                    area_str(name) for name in vuln["Area"]
                ]
                vuln_section = nodes.section(
                    ids=[f"vuln-{vuln['Vulnerability ID']}"], classes=classes
                )
                vuln_section += nodes.title(
                    "", self.fix_formatting(vuln["Summary"], "<Invalid issue title>")
                )
                vuln_section += self.create_bullet_list(vuln)
                year_section.append(vuln_section)

            self.state.section(
                title=f"Vulnerabilities {year}",
                source="",
                style="=",
                lineno=self.lineno,
                messages=year_section,
            )

        return []


def download_vuln_table(app: Sphinx) -> None:
    """Download vulnerability table from Azure.

    The vulnerabilities are stored in ``app.env.vuln_cache``.

    Args:
        app: The Sphinx app to store the vulnerabilities in.
    """

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
        [b for b in cc.list_blobs(name_starts_with=REPORT_PREFIX)],
        key=lambda b: b.last_modified,
        reverse=True,
    )

    target = remote_files[0]
    if local_cache and local_cache["md5"] == target.content_settings["content_md5"]:
        logger.info("Up to date vulnerability table found in cache")
        return

    bc = cc.get_blob_client(target)
    res = bc.download_blob().content_as_text()
    app.env.vuln_cache = json.loads(res)
    sort_func = lambda v: v["Vulnerability ID"]
    app.env.vuln_cache["body"] = list(sorted(app.env.vuln_cache["body"], key=sort_func))
    app.env.vuln_cache["md5"] = target.content_settings["content_md5"]
    logger.info("Vulnerability table cached locally")


def setup(app: Sphinx) -> dict:
    """Setup the ``vuln-table`` and ``vuln-details`` extensions.

    Necessary information are retrieved at the "builder-inited" stage.

    Args:
        app: The Sphinx app.
    """

    app.add_directive("vuln-table", VulnTable)
    app.add_directive("vuln-details", VulnDetails)

    app.connect("builder-inited", download_vuln_table)
    app.connect("builder-inited", read_versions)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
