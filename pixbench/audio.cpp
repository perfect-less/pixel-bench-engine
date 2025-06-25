#include "SDL3_mixer/SDL_mixer.h"
#include "pixbench/game.h"
#include <algorithm>
#include <memory>


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
