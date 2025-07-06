project(
  'sdl3 mixer',
  'c',
  version : '3.0.0dev'
)

cc = meson.get_compiler('c')

feature_args = ['-DMUSIC_WAV', '-DMUSIC_OGG']
if get_option('default_library') != 'static' and host_machine.system() == 'windows'
  feature_args += '-DDLL_EXPORT'
endif

sdl3_dep = dependency('sdl3')
ogg_dep = dependency('ogg')
vorbis_dep = dependency('vorbis')
vorbisfile_dep = dependency('vorbisfile')

flac_dep = dependency('libflac', required: get_option('flac'))
if flac_dep.found()
  feature_args += '-DMUSIC_FLAC_LIBFLAC'
endif

mpg123_dep = dependency('mpg123', required: get_option('mpg123'))
if mpg123_dep.found()
  feature_args += '-DMUSIC_MP3_MPG123'
endif

opus_dep = dependency('opusfile', required: get_option('opus'))
if opus_dep.found()
  feature_args += '-DMUSIC_OPUS'
endif

incdirs = include_directories('include', 'src', 'src/codecs')

sources = files(
  'src/mixer.c',
  'src/codecs/music_timidity.c',
  'src/codecs/music_mpg123.c',
  'src/codecs/load_voc.c',
  'src/codecs/music_drmp3.c',
  'src/codecs/load_aiff.c',
  'src/codecs/music_drflac.c',
  'src/codecs/music_xmp.c',
  'src/codecs/timidity/common.c',
  'src/codecs/timidity/tables.c',
  'src/codecs/timidity/readmidi.c',
  'src/codecs/timidity/instrum.c',
  'src/codecs/timidity/resample.c',
  'src/codecs/timidity/mix.c',
  'src/codecs/timidity/playmidi.c',
  'src/codecs/timidity/output.c',
  'src/codecs/timidity/timidity.c',
  'src/codecs/music_fluidsynth.c',
  'src/codecs/music_flac.c',
  'src/codecs/music_gme.c',
  'src/codecs/mp3utils.c',
  'src/codecs/music_ogg.c',
  'src/codecs/music_ogg_stb.c',
  'src/codecs/native_midi/native_midi_macosx.c',
  'src/codecs/native_midi/native_midi_win32.c',
  'src/codecs/native_midi/native_midi_common.c',
  'src/codecs/native_midi/native_midi_linux_alsa.c',
  'src/codecs/load_sndfile.c',
  'src/codecs/music_opus.c',
  'src/codecs/music_wav.c',
  'src/codecs/music_wavpack.c',
  'src/codecs/music_nativemidi.c',
  'src/utils.c',
  'src/music.c',
  'src/effect_position.c',
  'src/effects_internal.c',
  'src/effect_stereoreverse.c',
)

sdl3_mixer_lib = library(
  'sdl3mixer',
  sources,
  c_args: feature_args,
  include_directories: incdirs,
  dependencies: [vorbisfile_dep, vorbis_dep, ogg_dep, opus_dep, mpg123_dep, flac_dep, sdl3_dep],
  install: true
)

sdl3_mixer_dep = declare_dependency(
  include_directories: incdirs,
  link_with: sdl3_mixer_lib,
)
