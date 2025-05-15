project('midirun', 'cpp', 'c', version: '1.2.1')

gtk = dependency('gtkmm-4.0', version: '>=4.0.0')
toml = dependency('tomlplusplus')
rtmidi = dependency('rtmidi')

deps = [gtk, toml, rtmidi]

src = [
  'src/main.cpp',
  'include/deamon.hpp',
  'src/deamon.cpp',
  'include/log.hpp',
  'src/signal_state.cpp',
  'include/signal_state.hpp',
]

executable('midirun', src, dependencies: deps, install: true)

# Install systemd user service
user_service = 'midirun.service'
user_service_target_dir = join_paths(get_option('prefix'), 'lib', 'systemd', 'user')

install_data(user_service, install_dir: user_service_target_dir)
