project('engrampa', 'c',
  license : 'GPL2+',
  version : '1.27.1',
  meson_version : '>=0.43'
)

glib_version = '>=2.50'
gtk_version = '>=3.22'
caja_version = '>=1.17.1'
json_glib_version = '>=0.14.0'
sm_version = '>=1.0.0'
ice_version = '>=1.0.0'

gnome = import('gnome')
i18n = import('i18n')
gettext_package = meson.project_name()
prefix = get_option('prefix')
engrampa_mandir = join_paths(prefix, get_option('mandir'))
datadir = join_paths(prefix, get_option('datadir'))
privexecdir = join_paths(prefix, get_option('libexecdir'), meson.project_name())
c_comp = meson.get_compiler('c')

meson.add_install_script('postinstall.py')

mate_submodules_dep = dependency('mate-submodules', version: '1.24.0',
  fallback: ['mate-submodules', 'mate_submodules_dep'])

# Dependencies

libm_dep = c_comp.find_library('m')
thread_dep = dependency('threads')
glib_dep = dependency('glib-2.0', version : glib_version)
gthread_dep = dependency('gthread-2.0')
gtk_dep = dependency('gtk+-3.0', version : gtk_version)

# Optional dependencies

if get_option('caja-actions')
  libcaja_extension_dep = dependency('libcaja-extension', version : caja_version, required : false)
  build_caja_actions = libcaja_extension_dep.found()
else
  build_caja_actions = false
endif

libjson_glib_dep = dependency('json-glib-1.0', version : json_glib_version, required : false)
use_json_glib = libjson_glib_dep.found()
sm_dep = dependency('sm', version : sm_version)
ice_dep = dependency('ice', version : ice_version)

use_magic = false
if get_option('magic')
  libmagic_dep = c_comp.find_library('magic')
  if c_comp.compiles('''#include <magic.h>
                        int main () { magic_t m = magic_open(MAGIC_NONE); }''',
                     dependencies : libmagic_dep)
    use_magic = true
  endif
endif

cpio_path = 'cpio'
if get_option('cpio') == 'auto'
  cpio = find_program('gcpio', 'cpio', required : false)
  if cpio.found()
    cpio_path = cpio.path()
  endif
endif

# config.h

config_data = configuration_data()
config_data.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_data.set_quoted('MATELOCALEDIR', join_paths(prefix, get_option('localedir')))
config_data.set_quoted('PACKAGE_NAME', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set_quoted('VERSION', meson.project_version())
config_data.set_quoted('PKG_DATA_DIR', join_paths(prefix, get_option('datadir'), meson.project_name()))
config_data.set_quoted('ENGRAMPA_RESOURCE_UI_PATH', '/org/mate/Engrampa/ui')
config_data.set_quoted('PACKAGE_URL', 'https://mate-desktop.org')
if get_option('run-in-place')
  config_data.set_quoted('PRIVDATADIR', join_paths(meson.source_root(), 'data'))
  config_data.set_quoted('PRIVEXECDIR', join_paths(meson.source_root(), 'src', 'commands'))
  config_data.set_quoted('UIDIR', join_paths(meson.source_root(), 'data', 'ui'))
  config_data.set_quoted('SHDIR', join_paths(meson.source_root(), 'src', 'sh'))  
else
  config_data.set_quoted('PRIVDATADIR', join_paths(datadir, meson.project_name()))
  config_data.set_quoted('PRIVEXECDIR', privexecdir)  
  config_data.set_quoted('UIDIR', join_paths(datadir, meson.project_name(), 'ui'))
  config_data.set_quoted('SHDIR', join_paths(prefix, get_option('libexecdir'), meson.project_name()))
endif
if use_json_glib
  config_data.set('HAVE_JSON_GLIB', 1)
endif
if get_option('packagekit')
  config_data.set('ENABLE_PACKAGEKIT', 1)
endif
if get_option('buildtype').contains('debug')
  config_data.set('MATE_ENABLE_DEBUG', 1)
endif
if use_magic
  config_data.set('ENABLE_MAGIC', 1)
endif
config_data.set_quoted('CPIO_PATH', cpio_path)
config_file = configure_file(output : 'config.h', configuration : config_data)
config_inc = include_directories('.')

# C args

c_args = []
if get_option('buildtype').contains('debug')
  test_args = [ 
    '-Wall', 
    '-Wcast-align', 
    '-Wtype-limits', 
    '-Wclobbered', 
    '-Wempty-body', 
    '-Wignored-qualifiers', 
    '-Wmissing-prototypes',
    '-Wnested-externs', 
    '-Wpointer-arith', 
    '-Wno-sign-compare', 
    '-Wformat-security'
  ]
else
  c_args += [ '-Wno-deprecated-declarations' ]
  test_args = [ '-Wall' ]
endif
c_args += c_comp.get_supported_arguments(test_args)

# Subdirectories

subdir('data')
subdir('help')
if build_caja_actions
  subdir('caja')
endif
subdir('po')
subdir('src')

# Summary

summary = [
  'configuration summary:',
  '',
  '           project: @0@ @1@'.format(meson.project_name(), meson.project_version()),
  '            prefix: @0@'.format(prefix),
  '     Caja supports: @0@'.format(build_caja_actions),
  'PackageKit support: @0@'.format(get_option('packagekit')),
  '      Run in place: @0@'.format(get_option('run-in-place')),
  '      JSON support: @0@'.format(use_json_glib),
  '      Use libmagic: @0@'.format(use_magic),
  '              cpio: @0@'.format(cpio_path),
  ''
]
message('\n'.join(summary))
