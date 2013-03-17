#include "AudioFeatures.h"

AudioFeatures::AudioFeatures(void):
m_dMaxFrequency(0.0),
m_dWindowLenInMsecs(0.0),
m_dWindowStrideInMsecs(0.0),
m_uiNumWindows(0),
m_pWindowFeaturesPtr(0x0)
{
}

AudioFeatures::~AudioFeatures(void)
{
	if(m_pWindowFeaturesPtr)
		delete[] m_pWindowFeaturesPtr;
}

double AudioFeatures::GetMaxFrequency(void) const
{
	return m_dMaxFrequency;
}

double AudioFeatures::GetWindowLenInMsecs(void) const
{
	return m_dWindowLenInMsecs;
}

double AudioFeatures::GetWindowStrideInMsecs(void) const
{
	return m_dWindowStrideInMsecs;
}

unsigned int AudioFeatures::GetNumWindows(void) const
{
	return m_uiNumWindows;
}

WindowFeatures& AudioFeatures::Window(unsigned int Index) const
{
	if(Index < m_uiNumWindows)
		return m_pWindowFeaturesPtr[Index];
	else
		return m_pWindowFeaturesPtr[m_uiNumWindows];
}

WindowFeatures const * AudioFeatures::GetFeaturesArrPtr(void) const
{
	return m_pWindowFeaturesPtr;
}

void AudioFeatures::SetNumWindows(unsigned int Val)
{
	m_uiNumWindows = Val;
	if(m_pWindowFeaturesPtr)
		delete[] m_pWindowFeaturesPtr;
	m_pWindowFeaturesPtr = new WindowFeatures[m_uiNumWindows];
}

void AudioFeatures::CalculateGlobalParams(unsigned int SampleRate, unsigned int WndLenInSamples, unsigned int OverlapInSamples)
{
	m_dMaxFrequency = double(SampleRate)/2.0;
	m_dWindowLenInMsecs = 1000.0*(double(WndLenInSamples)/double(SampleRate));
	m_dWindowStrideInMsecs = 1000.0*(double(WndLenInSamples-OverlapInSamples)/double(SampleRate));
}

void AudioFeatures::CalculateDeltas(void)
{
	unsigned int i,j;
	m_pWindowFeaturesPtr[0].PowerDelta = 0.0;
	m_pWindowFeaturesPtr[0].FreqCentroidDelta = 0.0;
	m_pWindowFeaturesPtr[0].FreqSpreadDelta = 0.0;
	for(i = 1; i < m_uiNumWindows; i++)
	{
		j=i-1;
		m_pWindowFeaturesPtr[i].PowerDelta = m_pWindowFeaturesPtr[i].Power - m_pWindowFeaturesPtr[j].Power;
		m_pWindowFeaturesPtr[i].FreqCentroidDelta = m_pWindowFeaturesPtr[i].FreqCentroid - m_pWindowFeaturesPtr[j].FreqCentroid;
		m_pWindowFeaturesPtr[i].FreqSpreadDelta = m_pWindowFeaturesPtr[i].FreqSpread - m_pWindowFeaturesPtr[j].FreqSpread;
	}
}

void AudioFeatures::NormalizeDeltas(void)
{
	NormalizeArray(&m_pWindowFeaturesPtr->PowerDelta, sizeof(WindowFeatures), m_uiNumWindows);
	NormalizeArray(&m_pWindowFeaturesPtr->FreqCentroidDelta, sizeof(WindowFeatures), m_uiNumWindows);
	NormalizeArray(&m_pWindowFeaturesPtr->FreqSpreadDelta, sizeof(WindowFeatures), m_uiNumWindows);
}

void AudioFeatures::NormalizeGroupDelays(void)
{
	NormalizeArray(&m_pWindowFeaturesPtr->GroupDelay,sizeof(WindowFeatures),m_uiNumWindows);
}

void AudioFeatures::CalculateRhythmInterpolation(void)
{
	unsigned int i,j;
	//Create an array of weights in the shape of a normal distribution centered on the middle element
	double BellMaxHeight = 3.0;
	double BellWidthParam = 1.2;
	unsigned int SideSamplesToInclude = 2;
	unsigned int NumWeights = 2 * SideSamplesToInclude + 1;
	double* WeightsArr;
	WeightsArr = new double[NumWeights];
    
	for(i = 0; i < SideSamplesToInclude + 1; i++)
		WeightsArr[SideSamplesToInclude + i] = BellMaxHeight * exp(0.0 - ((double)(i * i) / (2.0 * BellWidthParam * BellWidthParam)));

	for(i = 0; i < SideSamplesToInclude; i++)
		WeightsArr[i] = WeightsArr[NumWeights - i - 1];

	//Calculate the actual interpolation
	for(i = 0; i < m_uiNumWindows; i++)
	{
		m_pWindowFeaturesPtr[i].RhythmInterpolation = 0.0;
		for(j = 0; j < NumWeights; j++)
		{
			int TempIndex = i + j - SideSamplesToInclude;
			if(TempIndex >= 0 && TempIndex < (int)m_uiNumWindows)
			{
				if(m_pWindowFeaturesPtr[TempIndex].PowerDelta > 0.0)
					m_pWindowFeaturesPtr[i].RhythmInterpolation += WeightsArr[j]*m_pWindowFeaturesPtr[TempIndex].PowerDelta;
				
				m_pWindowFeaturesPtr[i].RhythmInterpolation += WeightsArr[j]*fabs(m_pWindowFeaturesPtr[TempIndex].FreqSpreadDelta);
			}
		}
	}
	delete [] WeightsArr;
	//Normalize interpolation
	NormalizeArray(&m_pWindowFeaturesPtr->RhythmInterpolation,sizeof(WindowFeatures),m_uiNumWindows);
}

//Normalize an array of double type values so that they lie between -1 and 1 while maintaining their proportionality
void AudioFeatures::NormalizeArray(double* StartValPtr, size_t Stride, unsigned int NumVals)
{
	unsigned int i;
	double MaxAbsVal = 0.0;
	double* CurrElPtr;
	for(i=0;i<NumVals;i++)
	{
		CurrElPtr = (double*)(((char*)StartValPtr)+(i*Stride));
		if(fabs(*CurrElPtr) > MaxAbsVal)
			MaxAbsVal = fabs(*CurrElPtr);
	}
	for(i=0;i<NumVals;i++)
    {
		CurrElPtr = (double*)(((char*)StartValPtr)+(i*Stride));
		*CurrElPtr = (*CurrElPtr)/MaxAbsVal;
	}
}
