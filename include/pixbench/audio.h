#ifndef AUDIO_HEADER
#define AUDIO_HEADER

#include "SDL3_mixer/SDL_mixer.h"
#include "pixbench/utils.h"
#include <SDL3/SDL_audio.h>
#include <memory>
#include <string>
#include <vector>


class AudioClip {
public:
    Mix_Chunk* chunk{ nullptr };
    double length;
    int volume{ MIX_MAX_VOLUME };
};


class AudioChannelState {
public:
    bool is_active{ false };
    int channel{ -1 };
    bool is_playing{ false };
    bool is_paused{ false };
    bool is_looping{ false };
    std::shared_ptr<AudioClip> clip{ nullptr };
    double position{ 0.0 };
    int volume{ MIX_MAX_VOLUME };
};


class MusicClip {
public:
    Mix_Music* music{ nullptr };
    double length;
    int volume{ MIX_MAX_VOLUME };

    MusicClip() = default;
    MusicClip(Mix_Music* mix_music) {
        this->setMusicClip(mix_music);
    }

    ~MusicClip() {
        if (music)
            Mix_FreeMusic(music);
    }

    /*
     * Set the music pointer and calculate length duration
     */
    void setMusicClip(Mix_Music* mix_music) {
        if ( !mix_music )
            return;

        music = mix_music;
        length = Mix_MusicDuration(music);
    }
};


class MusicPlayer {
private:
    std::shared_ptr<MusicClip> m_music;
    double m_position{ 0.0 };
public:
    int volume{ MIX_MAX_VOLUME };   //!< Volume for audio played by this player
    bool is_looping{ false };       //!< Set `true` to loop the audio
    
    /*
     * Set the music clip
     */
    void setMusic(std::shared_ptr<MusicClip> music);

    /*
     * Set audio position in seconds
     */
    bool setPosition(double position);

    /*
     * Get audio position in seconds
     */
    double getPosition();
    
    /*
     * Returns `true` if this player is currently playing audio
     */
    bool isPlaying();

    /*
     * Play the audio clip, specify at (from 0 to 1) as position to start the audio
     * play.
     */
    bool play(double at=0);

    /*
     * Pause the currently playing audio.
     */
    void pause();

    /*
     * Resume paused audio at the last known position.
     */
    void resume();

    /*
     * Pause if audio is playing, otherwise play the audio clip.
     */
    void togglePlay();
};


struct SDL_AudioDevice {
    SDL_AudioDeviceID id;
    std::string device_name;
};


class AudioContext {
public:
    SDL_AudioDeviceID audio_device{ 0 };
    SDL_AudioSpec audio_spec;
    MusicPlayer* music_player{ nullptr };
    int num_channel{ MIX_DEFAULT_CHANNELS };
    
    AudioContext(
            int num_channels
            ) {
        // Prepare audio spec
        audio_spec.format = SDL_AUDIO_F32;
        audio_spec.channels = 2;
        audio_spec.freq = 44100;

        // Open audio device
        // we determined default audio device here
        SDL_AudioDeviceID opened_device = SDL_OpenAudioDevice(
                SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                &audio_spec
                );
        if ( !opened_device ) {
            // TODO: FAILED HERE
        }

        this->setAudioDevice(opened_device);

        int allocated_channels = Mix_AllocateChannels(num_channels);
        SDL_LogInfo(
                SDL_LOG_CATEGORY_AUDIO,
                "Allocated %d audio channel(s)", allocated_channels
                );
        this->num_channel = allocated_channels;

        // initiate music_player
        music_player = new MusicPlayer();
    }

    ~AudioContext() {
        this->closeCurrentAudioDevice();

        if ( this->music_player )
            delete music_player;
    }

    int numAvailableChannel() {
        return this->num_channel;
    }

    void closeCurrentAudioDevice() {
        if ( !audio_device )
            return;
        Mix_CloseAudio();
        audio_device = 0;
    }

    SDL_AudioDevice getAudioDevice() {
        SDL_AudioDevice ad;
        ad.id = this->audio_device;
        ad.device_name = SDL_GetAudioDeviceName(this->audio_device);
        return ad;
    }

    void setAudioDevice(SDL_AudioDeviceID audio_device_id) {
        // check if audio device already openned
        if ( audio_device ) {
            closeCurrentAudioDevice();
        }

        // open audio device
        bool is_success = Mix_OpenAudio(audio_device_id, &audio_spec);
        if ( !is_success )
            return;
        this->audio_device = audio_device_id;

        // const char* name = SDL_GetAudioDeviceName(audio_device_id);
        // if ( !name ) {
        //     // std::string err_msg = 
        //     //     "Can't get audio device name with id: "
        //     //     + std::to_string(audio_device_id);
        //     // return ResultError(err_msg);
        //     return;
        // }
        
        // return ResultOK;
    }

    std::vector<SDL_AudioDevice> listAvailableAudioDevices() {
        int device_count = 0;
        SDL_AudioDeviceID* device_arr = SDL_GetAudioPlaybackDevices(&device_count);

        std::vector<SDL_AudioDevice> audio_devices;
        
        if ( device_count == 0 ) {
            return audio_devices;
        }

        for (int i = 0; i < device_count; ++i) {
            const char* device_name = SDL_GetAudioDeviceName(device_arr[i]);
            if ( !device_name )
                continue;

            SDL_AudioDevice new_ad;
            new_ad.id = device_arr[i];
            new_ad.device_name = device_name;
            audio_devices.push_back(
                    new_ad
                    );
        }
        return audio_devices;
    }
};


// ==================== Audio Utility Functions ====================

/*
 * Load Audio Clip (internally as SDL_Mixer's Mix_Chunk)
 * return: Result::Ok containing std::shared_ptr<AudioClip> if the chunk is
 * successfuly loaded. Otherwise return Result::Err containing error message
 * as std::string
 */
Result<std::shared_ptr<AudioClip>, std::string> LoadAudioClip(std::string clip_path);


#endif
