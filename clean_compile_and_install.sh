#!/usr/bin/env bash
set -e

BUILDDIR="/home/hal/Builds/Mosaic/"
BUILTPKGSDIR="/home/hal/Builds/"

mkdir -p "${BUILDDIR}"
rsync -avz --cvs-exclude --delete ./ "${BUILDDIR}"  # Removes CMake cache files, forcing a fresh rebuild.
#rsync -avz --cvs-exclude ./ "${BUILDDIR}"

pushd .
cd "${BUILDDIR}"
makepkg --syncdeps --install --noconfirm
mv "${BUILDDIR}/"*.pkg.tar.* "${BUILTPKGSDIR}/"
popd

