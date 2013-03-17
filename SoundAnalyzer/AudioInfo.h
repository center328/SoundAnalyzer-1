#ifndef _AUDIO_INFO_H__
#define _AUDIO_INFO_H__

#include <vector>

typedef struct AudioInfo
{
public:
    unsigned int channelsCount;
    std::vector<int16_t>* samples;
    unsigned int sampleRate;
} AudioInfo;

#endif
