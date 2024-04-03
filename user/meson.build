bim_sources = [
  'd-bus.c',
  'main.c',
  'settings.c'
]

bim_deps = [
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
]

executable('bim', bim_sources,
  dependencies: bim_deps,
  install_dir: bin_dir,
  install: true,
)