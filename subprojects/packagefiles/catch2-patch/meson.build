project('catch2', 'cpp', version: '2.13.10')

catch2_inc = include_directories('single_include')

catch2_dep = declare_dependency(
  include_directories: catch2_inc
)

meson.override_dependency('catch2', catch2_dep)
