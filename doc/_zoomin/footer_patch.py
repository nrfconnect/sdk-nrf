#!/usr/bin/env python3
"""
Script to move footer contentinfo to main so Zoomin does render it.

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import sys

from bs4 import BeautifulSoup


def move_contentinfo_to_main(html_file, silent=False):
    """
    Move contentinfo div from footer to bottom of main div.

    Args:
        html_file: Path to the HTML file to process
        silent: If True, suppress warning messages

    Returns:
        Modified HTML content as string
    """
    # Read the HTML file
    with open(html_file, encoding='utf-8') as f:
        html_content = f.read()

    # Parse the HTML
    soup = BeautifulSoup(html_content, 'html.parser')

    # Find the footer tag
    footer = soup.find('footer')
    if not footer:
        if not silent:
            print(f"Warning: No footer tag found in {html_file}")
        return html_content

    # Find the div with role="contentinfo" within the footer
    contentinfo_div = footer.find('div', attrs={'role': 'contentinfo'})
    if not contentinfo_div:
        if not silent:
            print(f"Warning: No div with role='contentinfo' found in footer of {html_file}")
        return html_content

    # Find the div with role="main"
    main_div = soup.find('div', attrs={'role': 'main'})
    if not main_div:
        if not silent:
            print(f"Warning: No div with role='main' found in {html_file}")
        return html_content

    # Extract the contentinfo div from its current location
    contentinfo_div.extract()

    # Append it to the bottom of the main div
    main_div.append(contentinfo_div)

    # Return the modified HTML as string
    return str(soup)


def main():
    parser = argparse.ArgumentParser(
        description='Move contentinfo div from footer to bottom of main div in HTML files',
        allow_abbrev=False
    )
    parser.add_argument(
        'input_file',
        help='Input HTML file to process'
    )
    parser.add_argument(
        '-o', '--output',
        help='Output file (default: overwrite input file)',
        default=None
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Print the modified HTML without saving'
    )
    parser.add_argument(
        '-s', '--silent',
        action='store_true',
        help='Suppress all output messages'
    )

    args = parser.parse_args()

    try:
        # Process the HTML file
        modified_html = move_contentinfo_to_main(args.input_file, silent=args.silent)

        if args.dry_run:
            if not args.silent:
                print("Modified HTML (dry run - not saved):")
                print(modified_html)
        else:
            # Determine output file
            output_file = args.output if args.output else args.input_file

            # Write the modified HTML
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(modified_html)

            if not args.silent:
                print(f"Successfully processed {args.input_file}")
                if args.output:
                    print(f"Output saved to {args.output}")
                else:
                    print("File updated in place")

    except FileNotFoundError:
        if not args.silent:
            print(f"Error: File '{args.input_file}' not found")
        sys.exit(1)
    except Exception as e:
        if not args.silent:
            print(f"Error processing file: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
