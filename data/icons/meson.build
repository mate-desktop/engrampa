apps_icons = [
  ['16x16', 'engrampa.png'],
  ['22x22', 'engrampa.png'],
  ['24x24', 'engrampa.png'],
  ['32x32', 'engrampa.png'],
]

actions_icons = [
  ['16x16', 'add-folder-to-archive.png', 'extract-archive.png', 'add-files-to-archive.png'],
  ['24x24', 'add-folder-to-archive.png', 'extract-archive.png', 'add-files-to-archive.png'],
]

foreach icon: actions_icons
  install_data(
    join_paths(icon[0], 'actions', icon[1]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'actions')
  )
  install_data(
    join_paths(icon[0], 'actions', icon[2]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'actions')
  )
  install_data(
    join_paths(icon[0], 'actions', icon[3]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'actions')
  )
endforeach

foreach icon: apps_icons
  install_data(
    join_paths(icon[0], 'apps', icon[1]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'apps')
  )
endforeach

install_subdir('scalable', install_dir : join_paths(datadir, 'icons', 'hicolor'))
