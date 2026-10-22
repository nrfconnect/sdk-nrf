#!/usr/bin/env python
#
# Toolchain bundle index (v3).
#
# Script to manage toolchain bundles index
# It allows to:
#   * add new entry to index file
#   * validate index file
# Run nrfutil_toolchain_index.py --help for usage details

import argparse
import hashlib
import json
import os
import pathlib
import shutil
import sys
from pathlib import Path
from abc import ABC, abstractmethod

import jsonschema


class IndexValidationError(Exception):
    pass


class NrfutilToolchainIndex(ABC):
    _SCHEMA = {}

    @staticmethod
    @abstractmethod
    def validate_version_uniqueness(content):
        pass

    def validate_content(self, content):
        try:
            jsonschema.validate(schema=self._SCHEMA, instance=content)
            self.validate_version_uniqueness(content)
        except jsonschema.exceptions.ValidationError as e:
            raise IndexValidationError(e)

    def validate_file(self, path_to_index_json):
        content = self.read_json(path_to_index_json)
        self.validate_content(content)

    def add_bundle_entry_to_index(self, version: str, path_to_bundle: str, path_to_index_json: str, **kwargs):
        index_content = self.read_json(path_to_index_json)
        entry_for_new_version = self.create_bundle_entry(version, path_to_bundle, **kwargs)
        index_content.append(entry_for_new_version)
        self.validate_content(index_content)

        self.backup_index(path_to_index_json)
        self.write_json(index_content, path_to_index_json)

    def backup_index(self, path_to_index_json):
        path_to_backup_index_json = self.get_backup_file_name(path_to_index_json)
        shutil.copy(path_to_index_json, path_to_backup_index_json)

    def copy_entry_to_index(self, path_to_source_index_json, path_to_index_json, version):
        source_content = self.read_json(path_to_source_index_json)
        entry = self.get_bundle_entry(source_content, version)
        index_content = self.read_json(path_to_index_json)
        index_content.append(entry)

        self.validate_content(index_content)
        self.backup_index(path_to_index_json)
        self.write_json(index_content, path_to_index_json)

    @staticmethod
    def read_json(path_to_index_json):
        with open(path_to_index_json, 'r') as index_json:
            return json.load(index_json)

    @staticmethod
    def write_json(index_content, path_to_index_json):
        with open(path_to_index_json, 'w') as index_file:
            json.dump(index_content, index_file, indent=4)

    @staticmethod
    def calculate_sha512(path_to_bundle):
        filebytes = Path(path_to_bundle).read_bytes()
        return hashlib.sha512(filebytes).hexdigest()

    @staticmethod
    def get_backup_file_name(path_to_index_json):
        return os.path.join(os.path.dirname(path_to_index_json), f"backup_{os.path.basename(path_to_index_json)}")

    @abstractmethod
    def create_bundle_entry(self, identifier: str, path_to_bundle: str, **kwargs) -> dict:
        pass

    @abstractmethod
    def get_bundle_entry(self, index_content, version):
        pass

    @abstractmethod
    def get_filename(self, path_to_index_json, version):
        pass


class NrfutilToolchainIndexV3(NrfutilToolchainIndex):
    _ITEM_SCHEMA_V3 = {
        "type": "object",
        "properties": {
            "key": {
                "type": "string"
            },
            "json_api_version": {
                "enum": [2]
            },
            "metadata": {
                "type": "object",
                "properties": {
                    "filename": {"type": "string"},
                    "version": {"type": "string"},
                    "sha512": {"type": "string"},
                    "timestamp": {"type": "integer"},
                },
                "required": ["filename", "version", "sha512", "timestamp"],
                "additionalProperties": False,
            },
        },
        "required": ["key", "metadata", "json_api_version"],
        "additionalProperties": False,
    }

    _ITEM_SCHEMA_V2 = {
            "type": "object",
            "properties": {
                "version": {
                    "type": "string"
                },
                "json_api_version": {
                    "enum": [1]
                },
                "toolchains": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "name": {"type": "string"},
                            "version": {"type": "string"},
                            "sha512": {"type": "string"},
                        },
                        "required": ["name", "version", "sha512"],
                        "additionalProperties": False,
                    }
                },
            },
            "required": ["version", "toolchains", "json_api_version"],
            "additionalProperties": False,
        }

    _SCHEMA = {
        "type": "array",
        "items": {
            "anyOf": [
                _ITEM_SCHEMA_V2,
                _ITEM_SCHEMA_V3,
            ]
        }
    }

    @staticmethod
    def validate_version_uniqueness(content):
        keys = [entry.get('key') or entry.get('version') for entry in content]
        duplicates = set([x for x in keys if keys.count(x) > 1])
        if duplicates:
            raise IndexValidationError(f"There are multiple entries with same version keys: {duplicates}")

    def create_bundle_entry(self, identifier: str, path_to_bundle: str, timestamp: int = 0, **kwargs) -> dict:
        sha512 = self.calculate_sha512(path_to_bundle)

        return {
            'json_api_version': 2,
            'key': identifier,
            'metadata': {
                'version': identifier,
                'filename': os.path.basename(path_to_bundle),
                'sha512': sha512,
                'timestamp': timestamp
            }
        }

    def get_bundle_entry(self, index_content, version):
        copied_entry = dict()
        for item in index_content:
            if item['json_api_version'] != 2:
                continue
            if item["key"] == version:
                copied_entry = dict(item)
        assert copied_entry, "Cannot find version key from json file"
        return copied_entry

    def add_tag_entry_to_index(self, version: str, path_to_index_json: str, tagname: str,**kwargs):
        index_content = self.read_json(path_to_index_json)
        entry_for_new_tag = self.get_bundle_entry(index_content, version)
        entry_for_new_tag['key'] = tagname
        index_content.append(entry_for_new_tag)
        self.validate_content(index_content)
        self.backup_index(path_to_index_json)
        self.write_json(index_content, path_to_index_json)

    def get_filename(self, path_to_index_json, version):
        index_content = self.read_json(path_to_index_json)
        self.validate_content(index_content)
        entry = self.get_bundle_entry(index_content, version)
        print(entry['metadata']['filename'])

class NrfutilToolchainIndexCli(ABC):
    def __init__(self, main_parser):
        self.main_parser = main_parser
        self.subparsers = self.main_parser.add_subparsers(
            title=f'Available commands in schema version {self.get_version()}',
            required=True, dest='command')
        self.toolchain_index = NrfutilToolchainIndexFactory.get(self.get_version())
        self.known_commands = {}
        self.add_update_command()
        self.add_validate_command()
        self.add_copy_command()
        self.add_filename_command()

    def run_command(self):
        args = self.main_parser.parse_args()
        command_to_run = self.known_commands[args.command]
        command_to_run(args)

    @abstractmethod
    def get_version(self):
        pass

    def add_update_command(self):
        self.known_commands['update'] = lambda args: self.toolchain_index.add_bundle_entry_to_index(
            version=args.bundle_version,
            path_to_bundle=args.bundle_file,
            path_to_index_json=args.index_file
        )
        update_parser = self.subparsers.add_parser(
            'update',
            description='''Add VERSION entry pointing to BUNDLE_FILE to INDEX_FILE json.
    
    Backup file will be created in same directory as INDEX_FILE.''',
            help="Add new entry to index.json")

        update_parser.add_argument(
            '--index-file',
            help="Path to index.json file",
            type=pathlib.Path,
            required=True
        )
        update_parser.add_argument(
            '--bundle-file',
            help="Path to toolchain bundle (tar.gz) file",
            type=pathlib.Path,
            required=True
        )
        update_parser.add_argument(
            '--bundle-version',
            help="Version (key in index) of bundle",
            type=str,
            required=True
        )
        return update_parser

    def add_validate_command(self):
        self.known_commands['validate'] = lambda args: self.toolchain_index.validate_file(
            path_to_index_json=args.index_file
        )
        validate_parser = self.subparsers.add_parser('validate', help="Verify if INDEX_FILE follows schema")
        validate_parser.add_argument(
            '--index-file',
            help="Path to index.json file",
            type=pathlib.Path,
            required=True
        )
        return validate_parser
    
    def add_copy_command(self):
        self.known_commands['copy'] = lambda args: self.toolchain_index.copy_entry_to_index(
            path_to_index_json=args.index_file,
            path_to_source_index_json=args.source_index_file,
            version=args.bundle_version
        )
        copy_parser = self.subparsers.add_parser('copy', help="Copy version from source index to target index")
        copy_parser.add_argument(
            '--index-file',
            help="Path to target index.json file",
            type=pathlib.Path,
            required=True
        )
        copy_parser.add_argument(
            '--source-index-file',
            help="Path to source index.json file",
            type=pathlib.Path,
            required=True
        )
        copy_parser.add_argument(
            '--bundle-version',
            help="Version (key in index) of bundle",
            type=str,
            required=True
        )
        return copy_parser

    def add_filename_command(self):
        self.known_commands['filename'] = lambda args: self.toolchain_index.get_filename(
            path_to_index_json=args.index_file,
            version=args.bundle_version
        )
        filename_parser = self.subparsers.add_parser('filename', help="Print bundle filename")
        filename_parser.add_argument(
            '--index-file',
            help="Path to target index.json file",
            type=pathlib.Path,
            required=True
        )
        filename_parser.add_argument(
            '--bundle-version',
            help="Version (key in index) of bundle",
            type=str,
            required=True
        )
        return filename_parser


class NrfutilToolchainIndexV3Cli(NrfutilToolchainIndexCli):
    def __init__(self, main_parser):
        super().__init__(main_parser)
        self.add_tag_command()

    def get_version(self):
        return 3

    def add_update_command(self):
        update_parser = super().add_update_command()
        self.known_commands['update'] = lambda args: self.toolchain_index.add_bundle_entry_to_index(
            version=args.bundle_version,
            path_to_bundle=args.bundle_file,
            path_to_index_json=args.index_file,
            timestamp=args.timestamp,
        )
        update_parser.add_argument(
            '--timestamp',
            help="Timestamp of tarball creation",
            type=int,
            required=True
        )

    def add_tag_command(self):
        tag_bundle = self.subparsers.add_parser('tag', help="create tag for existing version of bundle")
        self.known_commands['tag'] = lambda args: self.toolchain_index.add_tag_entry_to_index(
            path_to_index_json=args.index_file,
            version=args.existing_key,
            tagname=args.new_key
        )
        tag_bundle.add_argument(
            '--index-file',
            help="Path to index.json file",
            type=pathlib.Path,
            required=True
        )
        tag_bundle.add_argument(
            '--existing-key',
            help="Existing version (key in index) of bundle",
            type=str,
            required=True
        )
        tag_bundle.add_argument(
            '--new-key',
            help="Name of new key tag that points to existing key",
            type=str,
            required=True
        )

class NrfutilToolchainIndexFactory:
    @staticmethod
    def get(version: int) -> NrfutilToolchainIndex:
        if version == 3:
            return NrfutilToolchainIndexV3()
        else:
            raise RuntimeError(f"Not implemented version: {version}")


def get_cli() -> NrfutilToolchainIndexCli:
    main_parser = argparse.ArgumentParser(
        prog='Nrfutil Toolchain Index Manager',
        description='Script to add new entries to toolchain bundle index',
        add_help=True
    )
    main_parser.add_argument(
        '--schema-version',
        default=3,
        type=int,
        choices=[3]
    )

    # Parse args to find schema version
    # Skip --help argument to enable help option on subparsers
    first_parsing_args = main_parser.parse_known_args([a for a in sys.argv if a not in ('--help', '-h')])

    version = first_parsing_args[0].schema_version

    # FLOW
    if version == 3:
        cli = NrfutilToolchainIndexV3Cli(main_parser)
    else:
        raise RuntimeError(f"No Cli implemented for version: {version}")
    return cli


if __name__ == "__main__":
    cli = get_cli()
    cli.run_command()
