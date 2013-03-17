#ifndef _OSX_AUDIO_LOADER_H__
#define _OSX_AUDIO_LOADER_H__

#include "AudioLoader.h"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>

class OSXAudioLoader : public AudioLoader
{
public:
    OSXAudioLoader();
    virtual ~OSXAudioLoader();
    
    bool loadFromMP3(const std::string& path, AudioInfo& info);
    bool loadFromOGG(const std::string& path, AudioInfo& info);
    bool loadFromWAV(const std::string& path, AudioInfo& info);
    
private:
    ALCcontext* context;
    ALCdevice* device;
};

#endif
