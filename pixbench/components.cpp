#include "pixbench/ecs.h"
#include "pixbench/vector2.h"


void Transform::SetPosition(Vector2 position) {
    Vector2 parent_position = this->globalPosition - this->localPosition;
    this->globalPosition = position;
    this->localPosition = position - parent_position;
}

void Transform::SetLocalPosition(Vector2 localPosition) {
    Vector2 parent_position = this->globalPosition - this->localPosition;
    this->localPosition = localPosition;
    this->globalPosition = localPosition + parent_position;
}


// ==================== AudioPlayer ====================

void AudioPlayer::setClip(std::shared_ptr<AudioClip> audio_clip) {
    clip = audio_clip;
    m_is_need_sync = true;
}


void AudioPlayer::play() {
    if ( !m_is_playing )
        m_is_playing = true;
    else
        m_is_need_to_replay = true;
    m_is_need_sync = true;
}


void AudioPlayer::stop() {
    // resetting assigned channel should be done by AudioSystem
    m_is_playing = false;
    m_is_need_sync = true;
}


double AudioPlayer::getPosition() {
    return m_last_known_position;
}


void AudioPlayer::pause() {
    m_is_paused = true;

    m_is_need_sync = true;
}


void AudioPlayer::resume() {
    if ( !m_is_playing )
        return;

    m_is_paused = false;
    m_is_need_sync = true;
}


void AudioPlayer::togglePlay() {
    if ( !m_is_playing ) {
        play();
        return;
    }

    if ( !m_is_paused )
        this->pause();
    else
        this->resume();

    m_is_need_sync = true;
}


bool AudioPlayer::isPlaying() {
    return m_is_playing;
}


bool AudioPlayer::isFinished() {
    return m_is_finished;
}


bool AudioPlayer::isPaused() {
    return m_is_paused;
}


bool AudioPlayer::__isNeedSync() {
    return m_is_need_sync;
}

void AudioPlayer::__setIsPlaying(bool is_playing) {
    m_is_playing = is_playing;
}

void AudioPlayer::__setIsFinished(bool is_finished) {
    m_is_finished = is_finished;
}

int AudioPlayer::__setAssignedChannel(int channel) {
    if (channel < -1)
        channel = -1;

    m_assigned_channel = channel;
    return m_assigned_channel;
}


void AudioPlayer::__resetNeedSyncStatus() {
    m_is_need_sync = false;
}
