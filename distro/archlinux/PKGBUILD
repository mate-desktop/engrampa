# Maintainer: Spyros Stathopoulos <foucault.online@gmail.com>
# Contributor: Alessio Sergi <asergi at archlinux dot us>
pkgname=mate-extract
pkgver=2011.08.31
pkgrel=1
pkgdesc="Archive manipulator for MATE"
arch=('i686' 'x86_64')
license=('GPL')
depends=('desktop-file-utils' 'mate-conf' 'hicolor-icon-theme')
makedepends=('mate-doc-utils' 'intltool')
conflicts=('file-roller' 'file-roller2')
optdepends=('unrar: for RAR uncompression'
'zip: for ZIP archives' 'unzip: for ZIP archives'
'p7zip: 7zip compression utility' 'arj: for ARJ archives'
'unace: extraction tool for the proprietary ace archive format')
options=('!libtool' '!emptydirs')
install=mate-extract.install
url="http://matsusoft.com.ar/projects"
groups=('mate-extras')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
md5sums=('4f26837dca3752cc9d17cdbcb16c4ed5')

build() {
	cd "$srcdir/$pkgname"

	mate-doc-prepare --force
	autoreconf -i
	aclocal

	./configure --prefix=/usr \
		--disable-static \
		--localstatedir=/var \
		--libexecdir=/usr/lib/${pkgname} \
		--disable-scrollkeeper \
		--disable-schemas-install \
		--sysconfdir=/etc \
		--disable-packagekit
	make
}

package() {
	cd "$srcdir/$pkgname"
	make MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1 DESTDIR="$pkgdir/" install
	install -d -m755 "${pkgdir}/usr/share/mateconf/schemas"
	mateconf-merge-schema "${pkgdir}/usr/share/mateconf/schemas/${pkgname}.schemas" --domain ${pkgname} "${pkgdir}/etc/mateconf/schemas/*.schemas"
	rm -f "${pkgdir}/etc/mateconf/schemas/*.schemas"
}

# vim:set ts=2 sw=2 et:
