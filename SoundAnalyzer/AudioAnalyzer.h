#ifndef _AUDIO_ANALYZER_H__
#define _AUDIO_ANALYZER_H__

#include <cmath>
#include "fftw3/fftw3.h"
#include "AudioFeatures.h"
#include "FFTWOutput.h"
#include "AudioTrack.h"

class AudioAnalyzer
{
public:
	enum WindowFunctionEnum { HANN, HAMMING };

	AudioAnalyzer(void);
	~AudioAnalyzer(void);
	//Parameter setting accessors
	void SetWindowingFunction(WindowFunctionEnum Function);
	void SetWindowLen(float WindowLenInSecs);
	void SetOverlap(float NormalizedOverlap);
	void UsePadding(bool UsePadding);
	//Main computing functions
    void ComputeFFT(const AudioTrack* Input, unsigned int ChannelID, FFTWOutput& Output);
	void ComputeAudioFeatures(FFTWOutput& Input, AudioFeatures& Output);

protected:
	//Copy constructor, here to make sure users don't copy the class
	AudioAnalyzer(const AudioAnalyzer&);
	//Internal usage auxiliary functions
    void ComputeTransientValues(const AudioTrack* Input);
    void ConvertToAmpliPhase(double* val);
	void UnwrapArrayPhase(double* StartValPtr, size_t Stride, unsigned int NumVals);
	double ComputeArrayPower(double* StartValPtr, size_t Stride, unsigned int NumVals);
	double ComputeArraySpectralCentroid(fftw_complex* StartValPtr, unsigned int NumVals);
	double ComputeArraySpectralSpread(fftw_complex* StartValPtr, unsigned int NumVals, double NormalizedCenterFrequency);
	void NormalizeArray(double* StartValPtr, size_t Stride, unsigned int NumVals);
	//Transformation settings
    double (*windowingFunc)(unsigned int, double);
    float m_fWndLenInSecs;
	float m_fOverlap;
	bool m_bUsePadding;
	//Transient values (they change for every input file processed)
	unsigned int m_uiSamplesPerChannel;
	unsigned int m_uiNumWindows;
	unsigned int m_uiWndLenInSamples;
	unsigned int m_uiPaddedWndLenInSamples;
	unsigned int m_uiWndStride;
	unsigned int m_uiOverlapInSamples;
	unsigned int m_uiNumFreqBins;
	unsigned int m_uiSampleRate;
	//PI
	const double m_dMathPI;
};

static inline double HannFunc(unsigned int j, double factor)
{
    return 0.5 - (0.5 * cos(j * factor));
}

static inline double HammingFunc(unsigned int j, double factor)
{
    return 0.53836 - (0.46164 * cos(j * factor));
}


static inline double fastInvSqrt(double val)
{
    double x2 = val * .5, y = val;
    int i = *(int*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(double*)&i;
    y *= 1.5 - (x2 * y * y);
    // Second Iteration: slower sqrt, higher precision
    //y *= 1.5 - (x2 * y * y);
    
    return y;
}

static inline float fastInvSqrtAsm(float val)
{
    float reciprocal;
    __asm__ __volatile__ (
                        "movss %1, %%xmm0\n"
                        "rsqrtss %%xmm0, %%xmm1\n"
                        "movss %%xmm1, %0\n"
                        :"=m"(reciprocal)
                        :"m"(val)
                        :"xmm0", "xmm1"
                        );
        
    //return reciprocal
    return (0.5 * (reciprocal + 1.0 / (val * reciprocal)));
}

#endif
