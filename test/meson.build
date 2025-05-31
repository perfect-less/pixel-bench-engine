catch2_dep = dependency('catch2-with-main')


# entity test
t_test_e = executable(
  'ecs_entity_test', 
  [
    'pixbench/ecs_entity_test.cpp',
    './../pixbench/vector2.cpp',
    './../pixbench/utils.cpp',
    './../pixbench/resource.cpp',
    './../pixbench/components.cpp',
    './../pixbench/entity.cpp',
    './../pixbench/systems.cpp',
  ],
  dependencies: [catch2_dep, sdl3_dep],
  include_directories: engine_includes
  )
test('ECS_ENTITY_TEST', t_test_e)
