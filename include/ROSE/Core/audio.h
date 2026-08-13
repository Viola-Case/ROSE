/**

  @file       audio.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       12.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/api.h>
namespace ROSE {

  class ROSE_API(Core) AudioTrack {
  public:
    AudioTrack();
    AudioTrack(const AudioTrack &);
    ~AudioTrack();
  private:
    friend class AudioSystem;
    void *m_track{nullptr};
  };

  /*!
   * This doesn't work yet.
   */
  class ROSE_API(Core) AudioSystem {
    void *m_mixer{nullptr};
    TypedHashMap<UUID, AudioTrack> m_tracks{};
    AudioSystem();
    ~AudioSystem();

  public:
    static AudioSystem &Get();
    void PlayAudio(UUID);
    void StopAudio(UUID);
  };

  // something something hrtf later
}