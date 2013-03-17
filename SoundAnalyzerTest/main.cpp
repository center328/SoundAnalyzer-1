#include "AudioAnalyzer.h"
#include "AudioSections.h"
#include "FFTWOutput.h"
#include <iostream>
#include <OpenAL/al.h>
#include <dlfcn.h>
#include "lame.h"
#include "lame_global_flags.h"
#include <functional>

int main (int argc, const char * argv[])
{
    /*
    void* lame = dlopen("/usr/local/lib/libmp3lame.dylib", RTLD_NOW);
    
    if (!lame)
        std::cout << "Fail! No library found" << std::endl;
    
    typedef std::function<lame_global_flags*()> lame_init_func;
    lame_init_func lame_init;
    
    lame_init = (lame_global_struct*(*)())dlsym(lame, "lame_init");
    
    if (!lame_init)
        std::cout << "Fail! No function found" << std::endl;
    
    lame_init();
    
    dlclose(lame);
    */
    
    AudioTrack track;
    track.initFromFile(argv[1]);
    
    /*
    unsigned int alSource;
    unsigned int alSampleSet;
    alGenSources(1, &alSource);
    alGenBuffers(1, &alSampleSet);
    alBufferData(alSampleSet, AL_FORMAT_STEREO16, track.getSamples(), (int)track.getSamplesCount(), track.getSampleRate());
    alSourcei(alSource, AL_BUFFER, alSampleSet);
    alSourcei(alSource, AL_LOOPING, AL_TRUE);
    alSourcePlay(alSource);
    */
    
    AudioAnalyzer FourierUtil;
	FFTWOutput FourierOutput;
	AudioFeatures TrackFeatures;
	AudioSections TrackSections;
	FourierUtil.SetWindowLen(.02f);
	FourierUtil.SetOverlap(.5f);
	FourierUtil.SetWindowingFunction(AudioAnalyzer::HANN);
	FourierUtil.UsePadding(true);
    
    std::cout << "Generating full periodogram..." << std::endl;
    FourierUtil.ComputeFFT(&track, 0, FourierOutput);
    
    std::cout << "Generating feature vector..." << std::endl;
    FourierUtil.ComputeAudioFeatures(FourierOutput, TrackFeatures);

    std::cout << "Determining beat locations and BPM..." << std::endl;
    TrackSections.Generate(TrackFeatures, 2000.0f);
    
    std::cout << "Track BPM estimation: " << TrackSections.GetGlobalBPM() << " BPM" << std::endl;
    
    return 0;
}
