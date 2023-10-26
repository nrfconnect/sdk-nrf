#!/bin/bash

# ----------------------------------------------------------------------
# Helper script for creating and pushing release tags in west.yml
# repositories.
#
# IMPORTANT:
#
#     ONLY USE THIS IF YOU ARE AUTHORIZED TO MAKE SIGNED RELEASE TAGS.
#     Contact the NCS release team if you need more information.
#
# To use:
#
#     1. Fill in the PROJECT_TAGS data below.
#
#     2. Go to your nrf repository clone, and update your workspace
#        to match the manifest you want to tag:
#
#          cd ~/ncs/nrf
#          git checkout $SDK_NRF_REVISION_YOU_WANT
#          west update
#
#     3. Create tags matching PROJECT_TAGS:
#
#          ./scripts/tag_west_repos.sh tag-all
#
#     4. Push those tags to GitHub:
#
#          ./scripts/tag_west_repos.sh push-all
#
# If you make a mistake and want to delete your local tags before
# pushing them and starting over, run 'tag_west_repos.sh remove-all'.

# ----------------------------------------------------------------------
# Configure the tags by editing this section.

declare -A PROJECT_TAGS

# Set the tag names for all the projects below.
#
#     - keys are project names in west.yml
#     - values are the tags you want to make
#
# nrfx/nRF/srcx repositories: vX.Y.Z(-rcN)
PROJECT_TAGS[nrfxlib]=""
PROJECT_TAGS[find-my]=""
PROJECT_TAGS[sidewalk]=""
PROJECT_TAGS[matter]=""
PROJECT_TAGS[nrf-802154]=""

# OSS repositories: vX.Y.Z-ncsN(-I)(-rcM)
PROJECT_TAGS[zephyr]=""
PROJECT_TAGS[mcuboot]=""
PROJECT_TAGS[trusted-firmware-m]=""
PROJECT_TAGS[mbedtls]=""










# ----------------------------------------------------------------------
# Everything below this line is implementation details.

SCRIPT=$(basename "$0")

hline() {
    # Helper function for printing a horizontal line
    printf '%*s\n' "$(tput cols)" '' | tr ' ' -
}

tag() {
    # Synopsis:
    #
    #    tag <project-name> <tag-name>
    #
    # Create a tag named <tag-name> in the west.yml project named
    # <project-name>, at the current manifest-rev.
    #
    # The tag message is '<repository-path> <tag-name>', where
    # <repository-path> is something like 'sdk-foo' for project 'foo'.

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    sha=$(west list -f '{sha}' "$project")
    remote_basename=$(west list -f '{url}' "$project" | xargs basename)
    local_path=$(west list -f '{abspath}' "$project")
    message="$remote_basename $tagname"

    echo "$project": creating tag "$tagname" for "$sha" in "$local_path"
    git -C "$local_path" tag -s -a -m "$message" "$tagname" "$sha" || exit 1

    echo "$project": verifying tag
    git -C "$local_path" tag -v "$tagname" || exit 1
}

push_tag() {
    # Synopsis:
    #
    #   push_tag <project-name> <tag-name>
    #
    # Push the tag named <tag-name> into the remote URL
    # for the west.yml project named <project-name>.

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    local_path=$(west list -f '{abspath}' "$project")
    url=$(west list -f '{url}' "$project")

    echo "$project": pushing tag "$tagname" to "$url"
    git -C "$local_path" push "$url" "$tagname"
    echo "$project": pushed tag "$url/releases/tag/$tagname"
}

remove_tag() {
    # Synopsis:
    #
    #   remove_tag <project-name> <tag-name>
    #
    # Locally delete the tag named <tag-name> in the
    # west.yml project named <project-name>

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    local_path=$(west list -f '{abspath}' "$project")

    echo "$project": removing tag "$tagname" in "$local_path"

    git -C "$local_path" tag -d "$tagname"
}

check_tags() {

    for project in "${!PROJECT_TAGS[@]}"; do
        if [ -z "${PROJECT_TAGS[$project]}" ]; then
	    echo "error: empty tag name" 1>&2
	    exit 1
	fi
    done
}

tag_all() {
    # Creates all the tags in the PROJECT_TAGS array.

    for project in "${!PROJECT_TAGS[@]}"; do
        tag "$project" "${PROJECT_TAGS[$project]}"
    done
}

push_all() {
    # Pushes all the tags in the PROJECT_TAGS array
    # to the main nrfconnect repositories on GitHub.

    for project in "${!PROJECT_TAGS[@]}"; do
        push_tag "$project" "${PROJECT_TAGS[$project]}"
    done
}

remove_all() {
    # Removes all the tags in the PROJECT_TAGS array.

    for project in "${!PROJECT_TAGS[@]}"; do
        remove_tag "$project" "${PROJECT_TAGS[$project]}"
    done
}

command="$1"
shift

check_tags
case "$command" in
    tag-all)
	# Create tags in all the repositories in the local working trees.
	# Verify the GPG signatures on each tag. Failed verification is a fatal error.
	tag_all
	;;
    remove-all)
	# Remove any tags created by 'tag-all' from local repositories (for debugging)
	remove_all
	;;
    push-all)
	# Push tags created by 'tag-all' to the remote.
	push_all
	;;
    *)
	echo unknown or missing command
	echo example: \'"$SCRIPT" tag-all\'
	echo all commands: tag-all remove-all push-all
	;;
esac
