#include "AudioTrack.h"

#ifdef __APPLE__
#include "OSXAudioLoader.h"
#else
#include "AudioLoader.h"
#endif

AudioTrack::~AudioTrack()
{
    info.sampleRate = 0;
    info.channelsCount = 0;
    info.samples = nullptr;
    initialized = false;
}

bool AudioTrack::initFromFile(const std::string &path)
{
    if (!loader)
#ifdef __APPLE__
        loader = new OSXAudioLoader();
#endif
    
    return loader->loadFromWAV(path, info);
}
