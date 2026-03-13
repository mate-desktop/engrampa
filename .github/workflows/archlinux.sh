#!/usr/bin/bash

set -eo pipefail

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Archlinux
requires=(
	ccache # Use ccache to speed up build
	clang  # Build with clang on Archlinux
	meson  # Used for meson build
)

# https://gitlab.archlinux.org/archlinux/packaging/packages/engrampa
requires+=(
	autoconf-archive
	caja
	gcc
	gettext
	git
	glib2-devel
	gtk3
	gzip
	itstool
	libarchive
	make
	mate-common
	python
	tar
	unzip
	which
	yelp-tools
	zip
)

infobegin "Update system"
pacman --noconfirm -Syu
infoend

infobegin "Install dependency packages"
pacman --noconfirm -S ${requires[@]}
infoend
