"""
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension adds four custom roles:
    :merge-base:`<docset>`
    :merge-base-short:`<docset>`
    :previous-version-merge-base:`<docset>`
    :previous-version-merge-base-short:`<docset>`

These roles will be replaced with a long or short SHA for the docset in question.
For a docset to be used with these roles, its upstream URL and main branch must
specified with the config option `fetch_upstream_repos`. This option takes a
list on the format [(URL, branch-name), ...]

The role takes an optional argument, indicated with a ":", to specify the
formatting of the output SHA. Currently the only available option is "literal".

Example of use:
- :merge-base:`mcuboot`
- :previous-version-merge-base-short:`zephyr:literal`

The extension also adds a directive, "zephyr-migration-guide".
This directive retrieves Zephyr migration guides from the SHA of the previous
version's merge-base and the last version's merge-base, and creates a new
migration guide based on all new entries since former.

For example, if the previous version's merge base is at Zephyr version
3.2.99, and the last version's merge-base is at 3.5.0, the outputted guide will
include any changes made to migration-guide-3.3.rst since the SHA at 3.2.99, and
the entirety of migration-guide-3.4.rst and migration-guide-3.5.rst. Any
duplicate entries are filtered out. Two entries must be more than 5% different
to each other to be considered separate entries
(Levenshtein distance / paragraph length >= 0.05).

Example of use:
.. zephyr-migration-guide::
"""

from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective
from typing import List, Dict, Callable, Tuple
from sphinx.util import logging
from west.manifest import Manifest, ImportFlag
from pathlib import Path
from tempfile import TemporaryDirectory
from page_filter import read_versions
from Levenshtein import distance
import subprocess
import re

__version__ = "0.1.0"

logger = logging.getLogger(__name__)


def git(*args, cwd: Path = None, check: bool = True, capture_output: bool = True):
    """Perform a git command in a given directory.

    Args:
        args: unpacked list of arguments to the git command
        cwd: Path to the directory to perform the git command in
        check: Raise subprocess.CalledProcessError if a non-zero exit code is returned
        capture_output: Return the stdout of the command if True, else return the exit code

    Returns:
        The exit code or the stdout of the git command, based on the capture_output
        parameter.
    """
    ret = subprocess.run(
        ("git",) + args,
        check=check,
        cwd=cwd,
        capture_output=capture_output,
        text=True,
    )
    if capture_output:
        return ret.stdout.strip()
    return ret


def get_zephyr_version(version_file: str) -> Dict[str, str]:
    """Parse a Zephyr version file.

    Args:
        version_file: The content of a Zephyr version file as one string

    Returns:
        A dictionary representation of a Zephyr file.
    """
    version = {}
    for line in version_file.split("\n"):
        if not line.strip():
            continue
        match = re.match(r"(\w+)\s*=\s*([\w\d_-]*)", line)
        if match:
            label, value = match.groups()
            version[label] = value
        else:
            logger.warning(f"unexpected line in Zephyr VERSION file: {line}")
    return version


def find_migration_guides(
    rev_start: str, rev_end: str, cwd: Path
) -> Dict[str, Dict[str, List[str]]]:
    """Find all relevant Zephyr migration guides between to SHAs.

    Relevant guides involve the latest guide for the start SHA, its version
    in the end SHA and all new migration guides from the end SHA.

    Args:
        rev_start: SHA of the initial revision
        rev_end: SHA of the final revision
        cwd: Path to a git repository that has the history of upstream Zephyr

    Returns:
        An assembled migration guide based on the found migration guides.
        See assemble_migration_guide for specifics on the data structure.
    """

    def git_show_file(revision: str, filename: str) -> str:
        try:
            return git("show", f"{revision}:{filename}", cwd=cwd)
        except subprocess.CalledProcessError:
            logger.debug(f"Could not find file '{filename}' in revision {revision}")
            return None

    def get_guide_file(revision: str, version: Dict[str, str]) -> str:
        major, minor = next_version(version)
        return git_show_file(
            revision, f"doc/releases/migration-guide-{major}.{minor}.rst"
        ) or git_show_file(revision, f"doc/releases/migration-guide-{major + 1}.0.rst")

    def next_version(zephyr_version: Dict[str, str]) -> Tuple[int, int]:
        minor = int(zephyr_version["VERSION_MINOR"])
        if zephyr_version["PATCHLEVEL"] == "99":
            minor += 1
        major = int(zephyr_version["VERSION_MAJOR"])
        return major, minor

    def get_new_migration_guides(start_version, end_version, guides) -> List[str]:
        start_major, start_minor = next_version(start_version)
        end_major, end_minor = next_version(end_version)

        new_guides = []
        for (major, minor), guide in sorted(guides.items()):
            if (
                not (start_major <= major <= end_major)
                or start_major == major
                and minor <= start_minor
            ):
                continue
            elif major == end_major and minor > end_minor:
                break
            new_guides.append(git_show_file(rev_end, guide))

        return new_guides

    start_version_file = git_show_file(rev_start, "VERSION")
    end_version_file = git_show_file(rev_end, "VERSION")
    if not start_version_file or not end_version_file:
        print("One of the version files could not be found")
        return None

    start_version = get_zephyr_version(start_version_file)
    end_version = get_zephyr_version(end_version_file)

    all_guides = {}
    for guide in git("ls-tree", "--name-only", rev_end, "doc/releases/", cwd=cwd).split(
        "\n"
    ):
        m = re.match(r"doc/releases/migration-guide-(\d+)\.(\d+)\.rst$", guide)
        if m:
            all_guides[tuple(map(int, m.groups()))] = guide

    # The latest version of the previous migration guide
    guide_file = get_guide_file(rev_end, start_version)

    # Previous migration guide at the time of rev_start
    prev_guide_file = get_guide_file(rev_start, start_version)

    # Latest version of the latest migration guides
    next_guide_files = get_new_migration_guides(start_version, end_version, all_guides)

    return assemble_migration_guide(prev_guide_file, guide_file, next_guide_files)


def assemble_migration_guide(
    base: str, current: str, next: List[str]
) -> Dict[str, Dict[str, List[str]]]:
    """Collect changes made since the second-last downstream release to the last release.

    The changes include everything in the `next` list, and the differences from
    `base` to `current`.

    The returned data structure is a dictionary mapping all required and recommended
    changes to a mapping of titles to paragraphs that should belong to the guide
    under that title. Paragraphs that do not belong to a title are listed under
    'untitled'.

    Args:
        base: The latest migration guide at the start revision.
        current: The base migration guide at the end revision.
        next: List of new migration guides introduced since the start revision.

    Returns:
        All new paragraphs in a data structure representing the hierarchy
        required|recommended -> title -> paragraph
    """

    def flatten(parsed_guide: Dict[str, Dict[str, List[str]]]) -> Dict[str, List[str]]:
        flattened = {}
        for change_type in ("required", "recommended"):
            if not change_type in parsed_guide:
                flattened[change_type] = []
            else:
                flattened[change_type] = [
                    section
                    for title_sections in parsed_guide[change_type].values()
                    for section in title_sections
                ]
        return flattened

    def add_difference(base, new, add_to_base=False) -> Dict[str, Dict[str, List[str]]]:
        target = base if add_to_base else {}
        flattened = flatten(base)

        for change_type in ("required", "recommended"):
            if change_type not in target:
                target[change_type] = {}
            for title, sections in new[change_type].items():
                if title not in target[change_type]:
                    target[change_type][title] = []

                for section in sections:
                    if not has_section(flattened[change_type], section):
                        target[change_type][title].append(section)

        return target

    def extract_sections(guide, after, to=r"") -> Dict[str, List[str]]:
        match = re.search(rf"{after}(.*){to}", guide, re.DOTALL)
        if match and match.groups():
            text = match.group(1)
        else:
            print(rf"no matches for '{after}(.*){to}'")
            return {}

        sections = {"untitled": []}
        section = ""
        title = "untitled"
        for line in text.split("\n"):
            line += "\n"
            if line.startswith("* "):
                if section.strip():
                    sections[title].append(section)
                section = line
            elif re.match(r"\s", line):
                section += line
            elif re.match(r"^[\w\d]", line):
                if section.strip():
                    sections[title].append(section)
                    section = ""
                title = line
                if not title in sections:
                    sections[title] = []
        if section.strip():
            sections[title].append(section)

        return sections

    def has_section(sections: List[str], target: str) -> bool:
        for section in sections:
            if distance(section, target) < len(target) * 0.05:
                return True
        return False

    def parse_guide(guide: str) -> Dict[str, Dict[str, List[str]]]:
        if not guide:
            return {"required": {}, "recommended": {}}
        required = extract_sections(
            guide, after=r"Required [Cc]hanges\n.*?\n?", to=r"\nRecommended [Cc]hanges"
        )
        recommended = extract_sections(guide, after=r"Recommended [Cc]hanges\n.*?\n?")
        return {"required": required, "recommended": recommended}

    if base and not current:
        logging.warning("The migration guide has been removed since the last release")

    parsed_base_guide = parse_guide(base)
    parsed_current_guide = parse_guide(current)
    changes = add_difference(parsed_base_guide, parsed_current_guide)
    for next_guide in next:
        parsed_guide = parse_guide(next_guide)
        add_difference(changes, parsed_guide, add_to_base=True)

    return changes


def get_shas_and_migration_guides(app: Sphinx) -> None:
    """Perform git operations to retrieve merge-bases and migration guides.

    The results are stored in the environment and only refetched when the
    last minor version changes.
    """

    manifest = Manifest.from_topdir()

    # Get latest version
    versions = read_versions()

    # Get previous version
    for version in versions[1:]:
        if not version.endswith("99") or not "-dev" in version:
            previous_version = version
            break

    if (
        hasattr(app.env, "merge_base_shas")
        and app.env.merge_base_shas["version"] == previous_version
    ):
        logger.info("Updated merge base SHAs found in cache")
        return

    logger.info("fetching merge-bases and migration guides")
    data = git("show", f"v{previous_version}:west.yml", cwd=manifest.repo_abspath)
    last_manifest = Manifest.from_data(data, import_flags=ImportFlag.IGNORE)

    shas = {
        "merge-base": {},
        "merge-base-short": {},
        "previous-version-merge-base": {},
        "previous-version-merge-base-short": {},
    }

    current_previous_projects = zip(
        manifest.get_projects(app.config.fetch_upstream_repos),
        last_manifest.get_projects(app.config.fetch_upstream_repos),
    )
    for project, previous_project in current_previous_projects:
        # Set default prefetched values
        for lookup_table in shas.values():
            lookup_table[project.name] = "<SHA not updated>"

        url, branch = app.config.fetch_upstream_repos[project.name]

        with TemporaryDirectory(project.name) as tmp:
            # Fetch necessary data
            git("clone", project.abspath, tmp)
            git("fetch", project.url, project.revision, cwd=tmp, capture_output=False)
            git(
                "fetch",
                project.url,
                previous_project.revision,
                cwd=tmp,
                capture_output=False,
            )
            git("fetch", "-t", url, branch, cwd=tmp, capture_output=False)

            # Calculate merge bases
            merge_base = git("merge-base", project.revision, "FETCH_HEAD", cwd=tmp)
            previous_merge_base = git(
                "merge-base", previous_project.revision, "FETCH_HEAD", cwd=tmp
            )

            shas["merge-base"][project.name] = merge_base
            shas["previous-version-merge-base"][project.name] = previous_merge_base
            shas["merge-base-short"][project.name] = merge_base[:10]
            shas["previous-version-merge-base-short"][
                project.name
            ] = previous_merge_base[:10]

            if project.name == "zephyr":
                app.env.zephyr_migration_guide = find_migration_guides(
                    previous_merge_base, merge_base, tmp
                )

    shas["version"] = previous_version
    app.env.merge_base_shas = shas


def commit_replace(app: Sphinx) -> Callable:
    """Create a role function to replace SHAs for a given docset.

    Any docset given must also be configured in conf.py with the option
    `fetch_upstream_repos`.
    """

    def commit_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        shas = app.env.merge_base_shas
        node = nodes.Text("")
        docset = text
        option = ""
        if ":" in text:
            info = text.split(":")
            if len(info) > 2:
                logger.error(f"{lineno}: Too many arguments '{rawtext}'")
                return [node], []
            else:
                docset, option = info

        if docset in shas[name]:
            node = nodes.Text(shas[name][docset])
        else:
            logger.error(f"{lineno}: Could not find SHA for {rawtext}")

        if option == "literal":
            literal = nodes.literal()
            literal += node
            node = literal
        elif option != "":
            logger.error(f"{lineno}: Unknown option '{option}'")

        return [node], []

    return commit_role


class ZephyrMigrationGuide(SphinxDirective):
    """A directive that generates a migration guide based on changes made
    to the upstream migration guide(s) between the last releases."""

    def run(self):
        if not hasattr(self.env, "zephyr_migration_guide"):
            return [nodes.paragraph(text="Zephyr migration guide source not found")]

        guide = self.env.zephyr_migration_guide

        for change_type, change_title in (
            ("required", "Required Changes"),
            ("recommended", "Recommended Changes"),
        ):
            untitled_bullet_list = nodes.bullet_list()
            titled_sections = []
            for title, sections in guide[change_type].items():
                if not sections:
                    continue

                if title == "untitled":
                    bullet_list = untitled_bullet_list
                else:
                    bullet_list = nodes.bullet_list()
                for text in sections:
                    self.state.nested_parse(
                        nodes.raw("", text=text, format="rst"), 0, bullet_list
                    )

                if title == "untitled":
                    continue

                section = nodes.section(ids=[f"migration-guide-{title}-{change_type}"])
                section += nodes.title("", title)
                section += bullet_list
                titled_sections.append(section)

            if len(untitled_bullet_list.children) == 0:
                text = nodes.Text(f"There are currently no {change_title.lower()}.")
                list_item = nodes.list_item()
                list_item += text
                untitled_bullet_list += list_item

            self.state.section(
                title=change_title,
                source="",
                style="`",
                lineno=self.lineno,
                messages=[untitled_bullet_list, *titled_sections],
            )

        return []


def setup(app: Sphinx):
    app.add_config_value("fetch_upstream_repos", None, "env")
    app.connect("builder-inited", get_shas_and_migration_guides)
    role_func = commit_replace(app)
    app.add_role("merge-base", role_func)
    app.add_role("previous-version-merge-base", role_func)
    app.add_role("merge-base-short", role_func)
    app.add_role("previous-version-merge-base-short", role_func)
    app.add_directive("zephyr-migration-guide", ZephyrMigrationGuide)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
