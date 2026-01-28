#!/usr/bin/bash

set -eo pipefail

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Fedora
requires=(
	ccache # Use ccache to speed up build
	meson  # Used for meson build
)

# https://src.fedoraproject.org/cgit/rpms/engrampa.git
requires+=(
	autoconf-archive
	caja-devel
	desktop-file-utils
	file-devel
	gcc
	git
	gtk3-devel
	json-glib-devel
	libSM-devel
	make
	mate-common
	redhat-rpm-config
)

infobegin "Update system"
dnf update -y
infoend

infobegin "Install dependency packages"
dnf install -y ${requires[@]}
infoend
