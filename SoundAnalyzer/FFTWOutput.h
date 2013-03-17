#ifndef _FFTW_OUTPUT_H__
#define _FFTW_OUTPUT_H__

#include "fftw3/fftw3.h"

class FFTWOutput
{
public:
	FFTWOutput(void);
	~FFTWOutput(void);
	double* const Element(unsigned int Window, unsigned int FreqBin) const;

	unsigned int SampleRate;
	unsigned int WndLenInSamples;
	unsigned int OverlapInSamples;
	unsigned int NumFreqBins;
	unsigned int NumWindows;
	double* WindowPowersPtr;
	fftw_complex* OutputPtr;

protected:
	FFTWOutput(const FFTWOutput&);
};

#endif
