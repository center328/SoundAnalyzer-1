#ifndef _AUDIO_LOADER_H__
#define _AUDIO_LOADER_H__

#include <string>
#include "AudioInfo.h"

class AudioLoader
{
public:
    AudioLoader() : initialized(false) {}
    virtual ~AudioLoader() {}
    
    virtual bool loadFromMP3(const std::string& path, AudioInfo& info) = 0;
    virtual bool loadFromOGG(const std::string& path, AudioInfo& info) = 0;
    virtual bool loadFromWAV(const std::string& path, AudioInfo& info) = 0;
    
protected:
    bool initialized;
};

#endif
