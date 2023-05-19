po_dir = join_paths(meson.source_root(), 'po')
man1_dir = join_paths(engrampa_mandir, 'man1')

schema_data = configuration_data()
schema_data.set('GETTEXT_PACKAGE', meson.project_name())
configure_file(
  input : 'org.mate.engrampa.gschema.xml.in',
  output : 'org.mate.engrampa.gschema.xml',
  configuration : schema_data,
  install : true,
  install_dir : join_paths(datadir, 'glib-2.0', 'schemas')
)

install_data('packages.match',
  install_dir : join_paths(datadir, meson.project_name())
)

install_data('engrampa.1',
  install_dir : man1_dir
)

# .desktop file

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())
desktop_in_file = configure_file(
  input : 'engrampa.desktop.in.in',
  output : 'engrampa.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'engrampa.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

# .service files

service_data = configuration_data()
service_data.set('libexecdir', join_paths(prefix, get_option('libexecdir')))

configure_file(
  input : 'org.mate.Engrampa.service.in',
  output : 'org.mate.Engrampa.service',
  configuration : service_data,
  install : true,
  install_dir : join_paths(datadir, 'dbus-1/services')
)

# .appdata file

appdata_data = configuration_data()
appdata_data.set('PACKAGE_URL', 'https://mate-desktop.org')
appdata_in_file = configure_file(
  input : 'engrampa.appdata.xml.in.in',
  output : 'engrampa.appdata.xml.in',
  configuration : appdata_data
)
i18n.merge_file(
  input : appdata_in_file,
  output : 'engrampa.appdata.xml',
  type : 'xml',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'metainfo')
)

# Subdirectories

subdir('icons')
