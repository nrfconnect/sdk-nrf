#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Generates SPDX 3.0.1 report in the JSON-LD format.

The report describes the same data as the SPDX 2.3 report generated from the
"templates/raport.spdx.jinja" template.
'''

import re
from datetime import datetime, timezone
from pathlib import Path

from args import args
from data_structure import Data, FileInfo, Package
from output_pre_process import DEPENDENCY_PLACEHOLDER_ID
from output_template import (
    download_location,
    get_ncs_version,
    group_by,
    log_output_written,
    package_roots,
    spdx_file_name,
    verification_code,
)
from spdx_python_model import v3_0_1 as spdx

SPEC_VERSION = '3.0.1'
DOCUMENT_NAME = 'west-ncs-sbom-report'
CREATOR_COMMENT = 'Generated with west-ncs-sbom.'

PRIMARY_PURPOSES = {
    'APPLICATION': spdx.software_SoftwarePurpose.application,
    'SOURCE': spdx.software_SoftwarePurpose.source,
    'OTHER': spdx.software_SoftwarePurpose.other,
}

# SPDX 2.3 external reference types (see git_info_detector.classify_external_ref)
EXTERNAL_IDENTIFIER_TYPES = {
    'purl': spdx.ExternalIdentifierType.packageUrl,
    'cpe23Type': spdx.ExternalIdentifierType.cpe23,
}

# Download locations that SPDX 2.3 uses to say that the location is unknown or absent
UNKNOWN_LOCATIONS = ('NONE', 'NOASSERTION')

NO_ASSERTION_LICENSE = spdx.expandedlicensing_IndividualLicensingInfo.NoAssertionLicense


def parse_timestamp(value: str) -> datetime:
    '''Parse a UTC ISO 8601 timestamp, e.g. "2026-01-02T03:04:05Z".'''
    return datetime.fromisoformat(value.replace('Z', '+00:00'))


class Spdx3Builder:
    '''Builds the SPDX 3.0.1 object graph from the pipeline data.'''

    def __init__(self, data: Data):
        self.data = data
        self.package_roots = package_roots(data)
        # The same namespace as the SPDX 2.3 "DocumentNamespace" field.
        self.namespace = f'http://spdx.org/spdxdocs/west-ncs-sbom-{data.report_uuid}'
        self.objects = []
        self.content = []
        self.relationship_counter = 0
        self.organizations: dict[str, spdx.Organization] = {}
        self.license_expressions: dict[str, spdx.simplelicensing_LicenseExpression] = {}
        self.custom_licenses: dict[str, spdx.expandedlicensing_CustomLicense] = {}
        self.uses_expanded_licensing = False
        self.creation_info = self._create_creation_info()
        self._create_creators()

    def _id(self, local_name: str) -> str:
        return f'{self.namespace}/{local_name}'

    def _add(self, element, content: bool = True):
        '''Register the element in the document and, if content, in the SBOM.'''
        element.creationInfo = self.creation_info
        self.objects.append(element)
        if content:
            self.content.append(element)
        return element

    @staticmethod
    def _create_creation_info() -> spdx.CreationInfo:
        creation_info = spdx.CreationInfo()
        creation_info._id = '_:creationinfo'
        creation_info.specVersion = SPEC_VERSION
        creation_info.created = datetime.now(timezone.utc).replace(microsecond=0)
        creation_info.comment = CREATOR_COMMENT
        return creation_info

    def _create_creators(self):
        '''The agent that created the document and the tool it used.

        SPDX 2.3 only names the tool. SPDX 3.0 requires an agent, so the agent
        is created from the same information as the tool.
        '''
        ncs_version = get_ncs_version()
        name = f'west-ncs-sbom-{ncs_version}' if ncs_version else 'west-ncs-sbom'
        agent = spdx.SoftwareAgent()
        agent._id = self._id('agent')
        agent.name = name
        tool = spdx.Tool()
        tool._id = self._id('tool')
        tool.name = name
        self.creation_info.createdBy = [self._add(agent, content=False)]
        self.creation_info.createdUsing = [self._add(tool, content=False)]

    def _relationship(
        self, from_, relationship_type, to: list, completeness=None, content: bool = True
    ) -> spdx.Relationship:
        self.relationship_counter += 1
        relationship = spdx.Relationship()
        relationship._id = self._id(f'relationship-{self.relationship_counter}')
        relationship.from_ = from_
        relationship.relationshipType = relationship_type
        relationship.to = to
        if completeness is not None:
            relationship.completeness = completeness
        return self._add(relationship, content=content)

    def _organization(self, name: str) -> spdx.Organization:
        if name not in self.organizations:
            organization = spdx.Organization()
            organization._id = self._id(f'organization-{len(self.organizations) + 1}')
            organization.name = name
            self.organizations[name] = self._add(organization)
        return self.organizations[name]

    def _license_expression(self, expression: str) -> spdx.simplelicensing_LicenseExpression:
        '''Return the license expression element, creating it on the first use.'''
        if expression not in self.license_expressions:
            element = spdx.simplelicensing_LicenseExpression()
            index = len(self.license_expressions) + 1
            element._id = self._id(f'license-expression-{index}')
            element.simplelicensing_licenseExpression = expression
            # Point every custom license identifier at its definition in this document.
            for token in re.split(r'[\s()]+', expression):
                custom_license = self.custom_licenses.get(token.upper())
                if custom_license is None:
                    continue
                entry = spdx.DictionaryEntry()
                entry.key = token
                entry.value = custom_license._id
                element.simplelicensing_customIdToUri.append(entry)
            self.license_expressions[expression] = self._add(element)
        return self.license_expressions[expression]

    def _license_targets(self, expressions: 'list[str]') -> list:
        '''License elements for the expressions, NoAssertionLicense if there is none.'''
        expressions = [x for x in dict.fromkeys(expressions) if x]
        if not expressions:
            self.uses_expanded_licensing = True
            return [NO_ASSERTION_LICENSE]
        return [self._license_expression(x) for x in expressions]

    def _create_custom_licenses(self):
        '''Licenses that are not on the SPDX License List, with their full text.'''
        for license_id in self.data.licenses_sorted:
            lic = self.data.licenses[license_id]
            if lic.is_expr or not lic.text:
                continue
            element = spdx.expandedlicensing_CustomLicense()
            element._id = self._id(lic.friendly_id)
            element.name = lic.name or lic.friendly_id
            element.simplelicensing_licenseText = lic.text
            if lic.url:
                element.expandedlicensing_seeAlso = [lic.url]
            self.custom_licenses[lic.friendly_id.upper()] = self._add(element)
            self.uses_expanded_licensing = True

    def _create_package(
        self, package: Package, index: int, files: 'list[FileInfo]'
    ) -> spdx.software_Package:
        element = spdx.software_Package()
        element._id = self._id(f'package-{index}')
        element.name = package.name or package.url or 'unknown-package'
        if package.version:
            element.software_packageVersion = package.version
        if package.supplier:
            element.suppliedBy = self._organization(package.supplier)
        location = download_location(package)
        if location not in UNKNOWN_LOCATIONS:
            element.software_downloadLocation = location
        if package.browser_url:
            element.software_homePage = package.browser_url
        if package.purl:
            element.software_packageUrl = package.purl
        identifiers = []
        if package.cpe:
            identifiers.append(('cpe23Type', package.cpe))
        identifiers.extend((ref_type, locator) for _, ref_type, locator in package.external_refs)
        for ref_type, locator in identifiers:
            identifier = spdx.ExternalIdentifier()
            identifier.externalIdentifierType = EXTERNAL_IDENTIFIER_TYPES[ref_type]
            identifier.identifier = locator
            element.externalIdentifier.append(identifier)
        if package.primary_package_purpose:
            element.software_primaryPurpose = PRIMARY_PURPOSES[package.primary_package_purpose]
        if package.built_date:
            element.builtTime = parse_timestamp(package.built_date)
        if files:
            code = spdx.PackageVerificationCode()
            code.algorithm = spdx.HashAlgorithm.sha1
            # verification_code() sorts in place, keep the files in the path order.
            code.hashValue = verification_code(list(files))
            element.verifiedUsing.append(code)
        return self._add(element)

    def _create_file(self, file: FileInfo, index: int) -> spdx.software_File:
        element = spdx.software_File()
        element._id = self._id(f'file-{index}')
        element.name = spdx_file_name(file, self.package_roots)
        element.software_fileKind = spdx.software_FileKindType.file
        checksum = spdx.Hash()
        checksum.algorithm = spdx.HashAlgorithm.sha1
        checksum.hashValue = file.sha1
        element.verifiedUsing.append(checksum)
        if file.local_modifications:
            element.comment = 'This file is modified locally.'
        if file.copyright_texts:
            element.software_copyrightText = '\n'.join(sorted(file.copyright_texts))
        self._add(element)
        self._relationship(
            element,
            spdx.RelationshipType.hasConcludedLicense,
            self._license_targets([file.license_expr_friendly]),
        )
        self._relationship(
            element,
            spdx.RelationshipType.hasDeclaredLicense,
            self._license_targets(file.infile_lics),
        )
        return element

    def _create_packages(self) -> 'list[spdx.software_Package]':
        files_by_package = group_by(self.data.files, 'package')
        packages = {}
        file_counter = 0
        # Numbering matches SPDXRef-package-N and SPDXRef-file-N of the SPDX 2.3 report.
        for index, package_id in enumerate(self.data.packages_sorted, start=1):
            if package_id == DEPENDENCY_PLACEHOLDER_ID:
                continue
            files = files_by_package.get(package_id, [])
            package = self._create_package(self.data.packages[package_id], index, files)
            packages[package_id] = package
            self._relationship(
                package, spdx.RelationshipType.hasConcludedLicense, self._license_targets([])
            )
            self._relationship(
                package, spdx.RelationshipType.hasDeclaredLicense, self._license_targets([])
            )
            file_elements = []
            for file in files:
                file_counter += 1
                file_elements.append(self._create_file(file, file_counter))
            if file_elements:
                self._relationship(package, spdx.RelationshipType.contains, file_elements)
        for package_id, package in packages.items():
            dependencies = self.data.packages[package_id].dependencies
            targets = [packages[dep_id] for dep_id in dependencies if dep_id in packages]
            if targets:
                self._relationship(package, spdx.RelationshipType.dependsOn, targets)
            if DEPENDENCY_PLACEHOLDER_ID in dependencies:
                # Dependencies are unknown, SPDX 3 has a dedicated way to say it.
                self._relationship(
                    package,
                    spdx.RelationshipType.dependsOn,
                    [spdx.IndividualElement.NoAssertionElement],
                    completeness=spdx.RelationshipCompleteness.noAssertion,
                )
        return list(packages.values())

    def _create_document(self, packages: 'list[spdx.software_Package]'):
        sbom = spdx.software_Sbom()
        sbom._id = self._id('sbom')
        sbom.software_sbomType = [
            spdx.software_SbomType.build
            if getattr(args, 'build_dir', None)
            else spdx.software_SbomType.source
        ]
        sbom.rootElement = packages
        sbom.element = list(self.content)
        self._add(sbom, content=False)
        data_license = self._license_expression('CC0-1.0')
        document = spdx.SpdxDocument()
        document._id = self._id('document')
        document.name = DOCUMENT_NAME
        document.dataLicense = data_license
        document.profileConformance = [
            spdx.ProfileIdentifierType.core,
            spdx.ProfileIdentifierType.software,
            spdx.ProfileIdentifierType.simpleLicensing,
        ]
        if self.uses_expanded_licensing:
            document.profileConformance.append(spdx.ProfileIdentifierType.expandedLicensing)
        document.rootElement = [sbom]
        if packages:
            # The SPDX 2.3 "SPDXRef-DOCUMENT DESCRIBES" relationships.
            self._relationship(document, spdx.RelationshipType.describes, packages, content=False)
        document.element = list(self.objects)
        self._add(document, content=False)

    def build(self) -> spdx.SHACLObjectSet:
        self._create_custom_licenses()
        self._create_document(self._create_packages())
        object_set = spdx.SHACLObjectSet()
        object_set.add(self.creation_info)
        for element in self.objects:
            object_set.add(element)
        return object_set


def generate(data: Data, output_file: 'Path|str'):
    '''Generate SPDX 3.0.1 JSON-LD output_file from data.'''
    output_file = Path(output_file)
    object_set = Spdx3Builder(data).build()
    with open(output_file, 'wb') as fd:
        spdx.JSONLDSerializer().write(object_set, fd, force_at_graph=True, indent=2)
    log_output_written(output_file)
