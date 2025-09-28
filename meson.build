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
subdir('src/')   # game sources

# Game executable
includes = engine_includes
sources += engine_sources
executable(
  'game', sources,
  dependencies: [sdl3_dep, sdl3_mixer_dep],
  include_directories: includes
  )
