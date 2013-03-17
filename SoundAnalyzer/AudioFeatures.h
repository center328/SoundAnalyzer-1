#ifndef _AUDIO_FEATURES_H__
#define _AUDIO_FEATURES_H__

#include <cmath>

struct WindowFeatures
{
	double Power;
	double PowerDelta;
	double FreqCentroid;
	double FreqCentroidDelta;
	double FreqSpread;
	double FreqSpreadDelta;
	double GroupDelay;
	double RhythmInterpolation;
};

class AudioFeatures
{
public:
	AudioFeatures(void);
	~AudioFeatures(void);
	double GetMaxFrequency(void) const;
	double GetWindowLenInMsecs(void) const;
	double GetWindowStrideInMsecs(void) const;
	unsigned int GetNumWindows(void) const;
	WindowFeatures& Window(unsigned int Index) const;
	WindowFeatures const * GetFeaturesArrPtr(void) const;
	void SetNumWindows(unsigned int Val);
	void CalculateGlobalParams(unsigned int SampleRate, unsigned int WndLenInSamples, unsigned int OverlapInSamples);
	void CalculateDeltas(void);
	void NormalizeDeltas(void);
	void NormalizeGroupDelays(void);
	void CalculateRhythmInterpolation(void);
	
	
protected:
	AudioFeatures(const AudioFeatures&);
	void NormalizeArray(double* StartValPtr, size_t Stride, unsigned int NumVals);

	double m_dMaxFrequency;
	double m_dWindowLenInMsecs;
	double m_dWindowStrideInMsecs;
	unsigned int m_uiNumWindows;
	WindowFeatures* m_pWindowFeaturesPtr;
};

#endif
