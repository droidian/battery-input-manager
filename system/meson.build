bim_sources = [
  'd-bus.c',
  'history.c',
  'main.c',
  'settings.c',
  'suspend.c'
]

bim_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('libcjson')
]

executable('bim', bim_sources,
  dependencies: bim_deps,
  install_dir: sbin_dir,
  install: true,
)