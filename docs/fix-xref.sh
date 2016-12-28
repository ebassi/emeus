#!/bin/sh

devdocs_url="https://developer.gnome.org"

dir=`pwd`

for file in $dir/*.html; do
  echo "Fixing cross-references in ${file}..."
  sed -i \
        -e "s|/usr/share/gtk-doc/html/gobject|${devdocs_url}/gobject/stable|" \
        -e 's|/usr/share/gtk-doc/html/glib|https://developer.gnome.org/glib/stable|' \
        -e 's|/usr/share/gtk-doc/html/gtk3|https://developer.gnome.org/gtk3/stable|' \
        -e 's|\.\./glib|https://developer.gnome.org/glib/stable|' \
        -e 's|\.\./gobject|https://developer.gnome.org/gobject.stable|' \
        -e 's|\.\./gtk3|https://developer.gnome.org/gtk3/stable|' \
  ${file}
done
