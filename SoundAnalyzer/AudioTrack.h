#ifndef _AUDIO_TRACK_H__
#define _AUDIO_TRACK_H__

#include <string>
#include "AudioLoader.h"

class AudioTrack
{
public:
    AudioTrack() : initialized(false), info(AudioInfo()), loader(NULL) {}
    ~AudioTrack();
    
    bool initFromFile(const std::string& path);
    
    bool isInitialized() const { return initialized; }
    
    size_t getSamplesCount() const { return info.samples->size() / info.channelsCount; }
    unsigned int getChannelsCount() const { return info.channelsCount; }
    const int16_t* getSamples() const { return info.samples->empty() ? NULL : &info.samples->at(0); }
    unsigned int getSampleRate() const { return info.sampleRate; }
    
private:
    bool initialized;
    AudioInfo info;
    AudioLoader* loader;
};

#endif