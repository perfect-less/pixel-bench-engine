# Add game specific sources to compilation list
this_dir = 'demo/platformer'
source_files = [
  'init.cpp',
  'chars/player.cpp',
  'util/game_manager.cpp',
  ]

this_include = include_directories('include')
includes += this_include

foreach source_filename : source_files
  sources += join_paths(this_dir, source_filename)
endforeach
