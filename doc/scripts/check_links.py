### This script reads links from a list of link files and checks if all links
### are valid.
### Errors are logged to the error file specified in the configuration file.

import os, sys, re
from configparser import ConfigParser,ExtendedInterpolation
from argparse import ArgumentParser
import requests
from requests.exceptions import ConnectionError, MissingSchema


### Class to store links from a links file with ID (the link text),
### URL, and type (url or replacement)

class Link:

    # Match the relevant lines in a link file
    url = re.compile("^.. _`(.+)`:( +(\S+)\s*)?\s*$")
    replacement = re.compile("^.. (|.+|) +replace:: +(\S+)\s*$")

    def __init__(self, file, line):

        self.file = file

        # "normal" URL definitions
        if line.startswith(".. _`"):
            result = self.url.search(line)
            if result:
                self.id = result.group(1)
                self.type = "url"
                if result.group(3):
                    self.url = result.group(3)
                else:
                    self.url = ""
            else:
                sys.exit("Invalid line in " + file + ": " + line)
        # replacements
        elif line.startswith(".. |"):
            result = self.replacement.search(line)
            if result:
                self.id = result.group(1)
                self.url = result.group(2)
                self.type = "replacement"
            else:
                sys.exit("Invalid line in " + file + ": " + line)

    def __str__(self):
        return self.id + ": " + self.url + " (" + self.file +")"

### Class to store link errors with a type, severity, and additional
### information (the link, duplicate link, status/error code)

class LinkError:

    error_types = { 'duplicate': (1, "URL defined more than once"),
                    'forbidden': (2, "URL cannot be crawled"),
                    'error': (6, "URL cannot be accessed"),
                    'redirect': (3, "URL redirect"),
                    'connection_error': (5, "Connection time-out"),
                    'relative_link': (4, "URL is defined incorrectly"),
                  }

    def __init__(self, type):
        self.type = type
        self.severity = self.error_types[type][0]
        self.link = None
        self.dup = None
        self.code = None

    def set_link(self, link):
        self.link = link

    def set_dup(self, link):
        self.dup = link

    def set_code(self, status_code):
        self.code = status_code

    def __str__(self):
        if self.severity > 4:
            out = "ERROR! "
        elif self.severity > 2:
            out = "WARNING! "
        else:
            out = "INFO - "
        out += self.error_types[self.type][1] + ":\n"
        if self.link:
            out += str(self.link)
        if self.dup:
            out += "\n" + str(self.dup)
        if self.code:
            out += "\nStatus code: " + str(self.code)

        return out

### Read the configuration from command line and configuration file

def init():

    configuration = {}

    parser=ArgumentParser(description="This script reads links from a list of \
                                       link files and checks if all links are \
                                       valid. \
                                       Errors are logged to the error file \
                                       specified in the configuration file.")
    parser.add_argument("-c","--config", help="The configuration file.",
                        required=True)
    parser.add_argument("-r","--root", help="The root folder for the link \
                                             files. If not specified, \
                                             $ZEPHYR_BASE/../ is used.",
                        required=False)
    args = parser.parse_args()

    config = ConfigParser()
    config.read(args.config)

    # Check and set the root folder, based on the provided argument
    # or ZEPHYR_BASE
    if args.root:
        if not (os.path.exists(args.root)):
            sys.exit("The specified root folder does not exist.")
        else:
            root_folder = args.root
    elif 'ZEPHYR_BASE' in os.environ:
        if not (os.path.exists(os.environ['ZEPHYR_BASE']+"/../")):
            sys.exit("$ZEPHYR_BASE/../ (the default root folder) does not "
                     "exist. Specify the correct root folder through --root, "
                     "or fix the ZEPHYR_BASE environment variable.")
        else:
            root_folder = os.environ['ZEPHYR_BASE']+"/../"
    else:
        sys.exit("The root folder cannot be determined. "
                 "Specify it through --root, or set the ZEPHYR_BASE "
                 "environment variable to use $ZEPHYR_BASE/../.")

    # Resolve and check the link files that are provided in the config file
    try:
        link_files = config['Input']['link_files'].split("\n")
        link_files = [os.path.normpath(root_folder+f) for f in link_files]
        for link_file in link_files:
            if not (os.path.exists(link_file)):
                sys.exit(link_file + " does not exist.")
        if not len(link_files) > 0:
            sys.exit("You must specify one or more link files in the "
                     "configuration file.")
    except KeyError as key:
        sys.exit(str(key) + " must be specified in the configuration file.")

    configuration['link_files'] = link_files

    # If set, get the output file
    try:
        configuration['error_log'] = config['Output']['error_log']
    except KeyError:
        configuration['error_log'] = None


    # If set, read the URL exceptions (URLs that the script reports as invalid,
    # but that are actually valid) from the config file
    try:
        configuration['URLexceptions'] = config['Exceptions']['invalid_urls'].split("\n")
    except KeyError:
        configuration['URLexceptions'] = []

    # If set, read the URL wildcard exceptions (start of URLs that the script
    # reports as invalid, but that are actually valid) from the config file
    try:
        configuration['URLwildcards'] = config['Exceptions']['invalid_wildcards'].split("\n")
    except KeyError:
        configuration['URLwildcards'] = []

    # If set, read the list of URLs that we want to redirect from the
    # config file
    try:
        configuration['redirects'] = config['Exceptions']['redirects'].split("\n")
    except KeyError:
        configuration['redirects'] = []

    return configuration


### Collect all links from the link files

def collect_links(link_files):

    links = []

    for f in link_files:
        with open(f,"r") as lines:
            for line in lines:
                line = line.rstrip("\n")
                # skip empty lines and comments
                if line == "" or line.startswith(".. #"):
                    continue
                # create links from URL and replacement lines
                elif line.startswith(".. _`") or line.startswith(".. |"):
                    links.append(Link(f, line))
                else:
                    sys.exit("Invalid line in " + f + ": " + line)

    return links

### Check all links and find duplicates and invalid URLs

def check_links(links, URLexceptions, URLwildcards, redirects):

    errors = []

    # Check for duplicate URLs
    # (No need to check for duplicate IDs, since they cause errors in Sphinx)

    # Create a dictionary of (url,file) and see if entries occur more than once
    URLs = {}

    for link in links:
        # ignore replacements and empty URLs
        if not link.url == "" and not link.type == "replacement":
            if (link.url,link.file) in URLs:
                err = LinkError("duplicate")
                err.set_link(URLs[(link.url,link.file)])
                err.set_dup(link)
                errors.append(err)

            URLs[(link.url,link.file)] = link


    # Check if the URLs exist and work
    for link in links:
        # don't check URLs that are in the exception list or empty
        if link.url in URLexceptions or link.url == "":
            continue
        else:
            # check if the URL is covered by a wildcard and if so, break
            for url in URLwildcards:
                if link.url.startswith(url):
                    break
            else:
                # otherwise check the URL
                print("Checking " + link.url)
                try:
                    test = requests.head(link.url)
                    # 403 is common if the server doesn't allow robots
                    # therefore, treat as a different category
                    if test.status_code == 403:
                        err = LinkError("forbidden")
                        err.set_link(link)
                        err.set_code(test.status_code)
                        errors.append(err)
                    # all status code >= 400 is an error
                    elif test.status_code >= 400:
                        err = LinkError("error")
                        err.set_link(link)
                        err.set_code(test.status_code)
                        errors.append(err)
                    # redirects should be updated (unless we want them)
                    elif (test.status_code == 301 or test.status_code == 308)\
                         and not link.url in redirects:
                        err = LinkError("redirect")
                        err.set_link(link)
                        err.set_code(test.status_code)
                        errors.append(err)

                # Handle exceptions from requests module
                except ConnectionError as error:
                    err = LinkError("connection_error")
                    err.set_link(link)
                    err.set_code(error)
                    errors.append(err)
                except MissingSchema as error:
                    err = LinkError("relative_link")
                    err.set_link(link)
                    err.set_code(error)
                    errors.append(err)

    return errors

### Print the errors to the output file or stdout

def print_errors(filename,errors):

    # sort by severity
    errors.sort(key=lambda x: x.severity, reverse = True)

    if filename:
        try:
            with open(filename,"w") as error_log:

                for err in errors:
                    error_log.write(str(err))
                    error_log.write("\n\n")

                print("Output written to "+filename)

        except IOError:

            # print to stdout if the file cannot be created
            print("Output file "+filename+" cannot be created.")
            return print_errors(None,errors)

    else:

        # print to stdout
        print()
        for err in errors:
            print(err)
            print()

    # return the highest severity as error code
    if len(errors) > 0:
        return errors[0].severity
    else:
        return 0


# Main:

config = init()

links = collect_links(config['link_files'])

errors = check_links(links, config['URLexceptions'], config['URLwildcards'], config['redirects'])

error_code = print_errors(config['error_log'],errors)

if error_code > 2:
    sys.exit("There are issues that must be fixed. Check the error log.")
else:
    sys.exit(0)
