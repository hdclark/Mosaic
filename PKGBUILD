# Maintainer: Hal Clark <gmail.com[at]hdeanclark>
pkgname=mosaic
pkgver=20170926_122300
pkgver() {
  date +%Y%m%d_%H%M%S
}
pkgrel=1

pkgdesc="A simplistic OpenGL-based flashcard viewer that implements a modified version of the SuperMemo algorithm (v2)."
url="http://www.halclark.ca"
arch=('x86_64' 'i686' 'armv7h')
license=('unknown')
depends=(
   'sqlite3'
   'imagemagick'
   'freeglut'
   'ygor'
)
#optdepends=( )
makedepends=(
   'cmake'
)
# conflicts=()
# replaces=()
# backup=()
# install='foo.install'
# source=("http://www.server.tld/${pkgname}-${pkgver}.tar.gz"
#         "foo.desktop")
# md5sums=(''
#          '')

#options=(!strip staticlibs)
options=(strip staticlibs)

build() {
  cmake ../ -DCMAKE_INSTALL_PREFIX=/usr
  make -j 4
}

package() {
  make -j 4 DESTDIR="${pkgdir}" install
}

# vim:set ts=2 sw=2 et:
