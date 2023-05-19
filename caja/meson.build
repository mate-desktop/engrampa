shared_module('caja-engrampa',
  sources : [
    'caja-engrampa.c',
    'engrampa-module.c'
  ],
  dependencies : [
    libm_dep,
    thread_dep,
    glib_dep,
    gthread_dep,
    gtk_dep,
    libcaja_extension_dep,
    use_json_glib ? libjson_glib_dep : [],
  ],
  include_directories : config_inc,
  c_args : c_args,
  install : true,
  install_dir : libcaja_extension_dep.get_pkgconfig_variable('extensiondir')
)

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())
desktop_data.set('PACKAGE_URL', 'https://mate-desktop.org')
desktop_in_file = configure_file(
  input : 'libcaja-engrampa.caja-extension.desktop.in.in',
  output : 'libcaja-engrampa.caja-extension.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'libcaja-engrampa.caja-extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'caja', 'extensions')
)
