/**

  @file       audio.cpp
  @brief      
  @details    ~
  @author     Viola Case
  @date       13.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/

#include <ROSE/ROSE.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace ROSE {

  AudioTrack::AudioTrack() {
    //m_track = MIX_CreateTrack()
  }
  AudioTrack::AudioTrack(const AudioTrack &) {

  }
  AudioTrack::~AudioTrack() {
    MIX_DestroyTrack(static_cast<MIX_Track *>(m_track));
  }

  AudioSystem::AudioSystem() {
    if (!MIX_Init()) {
      Log(LogLevel::Error, "SDL_mixer failed to initialize:\n\t{}",SDL_GetError());
      return;
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);


  }

  AudioSystem::~AudioSystem() {
    MIX_DestroyMixer(static_cast<MIX_Mixer *>(m_mixer));
    MIX_Quit();
  }

}