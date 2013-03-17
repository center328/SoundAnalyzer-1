#include "FFTWOutput.h"

FFTWOutput::FFTWOutput(void):
WndLenInSamples(0),
OverlapInSamples(0),
NumFreqBins(0),
NumWindows(0),
WindowPowersPtr(NULL),
OutputPtr(NULL)
{
}

FFTWOutput::~FFTWOutput(void)
{
	if(WindowPowersPtr)
		delete [] WindowPowersPtr;
	if(OutputPtr)
		delete [] OutputPtr;
}

double* const FFTWOutput::Element(unsigned int Window, unsigned int FreqBin) const
{
	if(Window < NumWindows && FreqBin < NumFreqBins)
		return (double*)OutputPtr + (Window * NumFreqBins + FreqBin);
	else
		return 0;
}
