#include "pixbench/audio.h"
#include "SDL3_mixer/SDL_mixer.h"
#include <algorithm>
#include <memory>


// ==================== Loader ====================
Result<std::shared_ptr<AudioClip>, std::string> LoadAudioClip(std::string clip_path) {
    auto res = Result<std::shared_ptr<AudioClip>, std::string>();

    std::shared_ptr<AudioClip> clip = std::make_shared<AudioClip>();
    Mix_Chunk* chunk = Mix_LoadWAV(clip_path.c_str());
    if ( !chunk ) {
        std::string err_message =
            "Can't open file '"
            ;
        err_message.append(
                clip_path
                );
        err_message.append(
                SDL_GetError()
                );
        return res.Err(err_message);
    }

    clip->chunk = chunk;
    // TODO: Calculate chunk duration

    return res.Ok(clip);
}

// ==================== MusicPlayer ====================

void MusicPlayer::setMusic(std::shared_ptr<MusicClip> music) {
    if ( this->isPlaying() )
        Mix_HaltMusic();
    
    m_music = music;
}


bool MusicPlayer::setPosition(double position) {
    if ( !m_music )
        return false;

    bool is_set = Mix_SetMusicPosition(position);
    return is_set;
}


double MusicPlayer::getPosition() {
    if (m_music)
        return Mix_GetMusicPosition(m_music->music);

    return 0;
}


bool MusicPlayer::isPlaying() {
    return Mix_PlayingMusic() && !Mix_PausedMusic();
}


bool MusicPlayer::play(double at) {
    if ( !m_music )
        return false;

    int loops = 0;      // play once and stops
    if (is_looping)
        loops = -1;
    bool is_playing = Mix_PlayMusic(m_music->music, loops);
    if ( !is_playing )
        return false;

    bool is_set = this->setPosition(at);
    return is_set;
}


void MusicPlayer::pause() {
    if (this->isPlaying()) {
        Mix_PauseMusic();
    }
}


void MusicPlayer::resume() {
    if ( !(this->isPlaying()) && m_music ) {
        if (Mix_PausedMusic())
            Mix_ResumeMusic();
        else {
            double start_pos = Mix_GetMusicPosition(m_music->music);
            start_pos = std::max(0.0, start_pos);
            this->play(start_pos);
        }
    }
}


void MusicPlayer::togglePlay() {
    if (this->isPlaying()) {
        this->pause();
    }
    else {
        this->resume();
    }
}

void MusicPlayer::setVolume(int volume) {
    Mix_VolumeMusic(volume);
    this->_volume = Mix_VolumeMusic(-1);
}

void MusicPlayer::setVolume(float volume) {
    const int volume_int = std::min(std::max(.0f, volume), 1.0f) * MIX_MAX_VOLUME;
    this->setVolume(volume_int);
}

int MusicPlayer::volume() {
    return this->_volume;
}
