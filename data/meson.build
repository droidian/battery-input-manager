# DBus service
configure_file(
  input: 'org.adishatz.Bim.service.in',
  output: 'org.adishatz.Bim.service',
  configuration: config_h,
  install: true,
  install_dir: dbus_service_dir
)

# User service
configure_file(
  input: 'battery-input-manager_user.service.in',
  output: 'battery-input-manager.service',
  configuration: config_h,
  install: true,
  install_dir: systemd_user_dir
)

# DBus configuration
install_data(
  'org.adishatz.Bim.conf',
  install_dir: dbus_conf_dir
)

# Devices json
install_data(
  'devices.json',
  install_dir: bim_data_dir
)

gnome.compile_resources(
  meson.project_name(),
  meson.project_name() + '.gresource.xml',
  gresource_bundle: true,
  source_dir: '.',
  install_dir: bim_data_dir,
  install: true,
)

schema_src = 'org.adishatz.Bim.gschema.xml'
compiled = gnome.compile_schemas(build_by_default: true,
                                 depend_files: files(schema_src))

install_data(schema_src,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
  args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif

gnome.post_install(glib_compile_schemas: true)