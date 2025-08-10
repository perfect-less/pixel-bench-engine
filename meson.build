# Project configs
project(
  'pixelbench', 'cpp',
  version: '0.0.1',
  license: 'MIT',
  default_options: [
    'c_std=c11',
    'cpp_std=c++11',
    'default_library=static',
    ]
  )

# Imports
fs = import('fs')

# Includes and dependencies
engine_includes = [include_directories('include/')]
sdl3_dep = dependency('sdl3')
sdl3_mixer_dep = dependency('sdl3_mixer')

# Sources
engine_sources = [
  'pixbench/main.cpp',
  'pixbench/game.cpp',
  'pixbench/vector2.cpp',
  'pixbench/utils.cpp',
  'pixbench/resource.cpp',
  'pixbench/components.cpp',
  'pixbench/entity.cpp',
  'pixbench/systems.cpp',
  'pixbench/rendering.cpp',
  'pixbench/audio.cpp',
  'pixbench/physics.cpp',
  'pixbench/hierarchy.cpp',
  ]

sources = []

# Subdir inclusions
subdir('test/')  # unit tests

# Handle game subdir, compile demo if demo is set using -Ddemo=<demo-name>
demo_name = get_option('demo')
if demo_name == ''
  subdir('src/')   # game sources
else
  demo_path = join_paths('demo', demo_name)
  if fs.exists(join_paths(demo_path, 'meson.build'))
    subdir(demo_path)   # demo sources
  else
    error('Demo "' + demo_name + '" does not exist')
  endif
endif

# Game executable
includes = engine_includes
sources += engine_sources
executable(
  'game', sources,
  dependencies: [sdl3_dep, sdl3_mixer_dep],
  include_directories: includes
  )
