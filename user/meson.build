project(
  'my_game', 'cpp',
  version: '0.0.1',
  default_options: [
    'c_std=c11',
    'cpp_std=c++11',
    'default_library=static',
    ]
  )

pbengine_dep = dependency('pixel-bench-engine')

# cmath
add_project_arguments('-D_USE_MATH_DEFINES', language: 'cpp')

link_args = []
if host_machine.system() == 'windows'
  link_args += '--static'
endif

# executable
executable(
  'game',
  'src/init.cpp',
  dependencies: [pbengine_dep],
  link_args: link_args
)
