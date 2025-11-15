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

# cmath
add_project_arguments('-D_USE_MATH_DEFINES', language: 'cpp')

# engine configs
conf = configuration_data({
  'MAX_ENTITIES': get_option('max_entities'),
  'MAX_COMPONENTS': get_option('max_components'),
  'MAX_TAGS': get_option('max_tags'),
  'FIXED_UPDATE_RATE': get_option('fixed_update_rate'),
  'RAYCAST_SEGMENT_LENGTH': get_option('raycast_segment_length'),
  'MAX_POLYGON_VERTEX': get_option('max_polygon_vertex'),
  'PHYSICS_DEBUG_DRAW': get_option('physics_debug_draw'),
  'PHYSICS_DEBUG_DRAW_SHOW_ENTITY_ID': get_option('physics_debug_draw_show_entity_id'),
  'PHYSICS_DEBUG_DRAW_SHOW_BOUNDING_BOX': get_option('physics_debug_draw_show_bounding_box'),
  })

configure_file(
  input: 'include/pixbench/engine_config.h.in',
  output: 'engine_config.h',
  # build_subdir: 'pixbench',
  configuration: conf
)

conf_inc = include_directories('.')

# Includes and dependencies
engine_includes = [include_directories('include/'), conf_inc]
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
# subdir('test/')  # unit tests

# Pixbench includes and sources
includes = engine_includes
sources += engine_sources

# Static library
pixbench_lib = static_library(
  'pixbench', sources,
  dependencies: [sdl3_dep, sdl3_mixer_dep],
  include_directories: includes,
  install: false
  )

# Dependency exports
pixbench_dep = declare_dependency(
  include_directories: includes,
  link_with: pixbench_lib,
  dependencies: [sdl3_dep, sdl3_mixer_dep],
  )

meson.override_dependency('pixel-bench-engine', pixbench_dep)
