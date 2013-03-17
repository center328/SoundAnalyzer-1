#include "AudioAnalyzer.h"
#include <iostream>

AudioAnalyzer::AudioAnalyzer(void):
m_dMathPI(2.0 * acos(0.0)),
windowingFunc(&HannFunc),
m_fWndLenInSecs(0.1f),
m_bUsePadding(true),
m_fOverlap(0.5f)
{
}

AudioAnalyzer::~AudioAnalyzer(void)
{
}

void AudioAnalyzer::SetWindowingFunction(WindowFunctionEnum Function)
{
    if (Function == HANN)
        windowingFunc = &HannFunc;
    else
        windowingFunc = &HammingFunc;
}

void AudioAnalyzer::SetWindowLen(float WindowLenInSecs)
{
	m_fWndLenInSecs = WindowLenInSecs;
}
	
void AudioAnalyzer::SetOverlap(float NormalizedOverlap)
{
	m_fOverlap = NormalizedOverlap;
}
	
void AudioAnalyzer::UsePadding(bool UsePadding)
{
	m_bUsePadding = UsePadding;
}

void AudioAnalyzer::ComputeFFT(const AudioTrack* Input, unsigned int ChannelID, FFTWOutput& Output)
{
	unsigned int i, j;
    
	//Calculate transient values
    ComputeTransientValues(Input);

    double scaleFactor = 1.0 / 32767.0;
    double normalizeFactor = 2.0 / m_uiPaddedWndLenInSamples;
    double funcFactor = 2.0 * m_dMathPI / (m_uiWndLenInSamples - 1);

	//Copy transient parameter to output
	Output.NumFreqBins = m_uiNumFreqBins;
	Output.NumWindows = m_uiNumWindows;
	Output.SampleRate = m_uiSampleRate;
	Output.WndLenInSamples = m_uiWndLenInSamples;
	Output.OverlapInSamples = m_uiOverlapInSamples;

	//Create the output arrays
	if(Output.WindowPowersPtr)
		delete [] Output.WindowPowersPtr;
	Output.WindowPowersPtr = new double[m_uiNumWindows];
	if(Output.OutputPtr)
		delete [] Output.OutputPtr;
	Output.OutputPtr = new fftw_complex[m_uiNumWindows * m_uiNumFreqBins];

	//Set up FFTW
	double *Tempinput;
	fftw_complex *Tempoutput;
	fftw_plan FFTWPlan;
	Tempinput = (double*)fftw_malloc(sizeof(double) * m_uiPaddedWndLenInSamples);
	Tempoutput = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * m_uiNumFreqBins);
	FFTWPlan = fftw_plan_dft_r2c_1d(m_uiPaddedWndLenInSamples, Tempinput, Tempoutput, FFTW_MEASURE);
	memset(Tempinput, 0x0, sizeof(double) * m_uiPaddedWndLenInSamples);

	//Perform Discrete Fourier Transform
    unsigned int TempIndex;
    unsigned int channels = Input->getChannelsCount();
    const short* samples = Input->getSamples();
	for(i = 0; i < m_uiNumWindows; i++)
	{
        Output.WindowPowersPtr[i] = 0.0;
		for(j = 0; j < m_uiWndLenInSamples; j++)
		{
            //Populate input array with samples from current window
			TempIndex = i * m_uiWndStride + j;
			if(TempIndex < m_uiSamplesPerChannel)
                Tempinput[j] = (double)samples[TempIndex * channels + ChannelID];
			else
				Tempinput[j] = 0;
            
            //Calculate window power
            Output.WindowPowersPtr[i] += Tempinput[j] * Tempinput[j];
            
            //Scale input signal to [-1,1] and apply window function
            Tempinput[j] *= scaleFactor * windowingFunc(j, funcFactor);
		}

		//Execute fourier transform
		fftw_execute(FFTWPlan);
		
		//Normalizing output of fourier transform:
		for(j = 0; j < m_uiNumFreqBins; j++)
		{
			//As input signal is purely real, its spectrum is hemitian, i.e. symmetric with respect to the origin 0Hz, or DC "frequency"; 
			//to save time only the positive half of the spectrum is computed.
			//Additionally, the computed fourier transform is unnormalized: the coefficients are left proportional to the number of samples analyzed (the window length)
			//Results are therefore divided by the window length and doubled
			Tempoutput[j][0] *= normalizeFactor;
			Tempoutput[j][1] *= normalizeFactor;
		}

		//Store result in output array
		memcpy((void*)(Output.OutputPtr + (i * m_uiNumFreqBins)), (void*)Tempoutput, sizeof(fftw_complex) * m_uiNumFreqBins);
	}
    
	//Release temporary arrays
	fftw_destroy_plan(FFTWPlan);
	fftw_free(Tempinput);
	fftw_free(Tempoutput);

	//Conversion to amplitude and phase information, if requested
	for(i = 0; i < m_uiNumWindows; i++)
		for(j = 0; j < m_uiNumFreqBins; j++)
            ConvertToAmpliPhase(Output.Element(i,j));

	//Phase unwrapping (start at element 2 as el 0 is DC and has no phase; end at first to last element for the same reason)
	for(i = 0; i < m_uiNumWindows; i++)
		UnwrapArrayPhase(&Output.Element(i,1)[1], sizeof(fftw_complex), m_uiNumFreqBins - 2);
}

void AudioAnalyzer::ComputeAudioFeatures(FFTWOutput& Input, AudioFeatures& Output)
{
	Output.CalculateGlobalParams(Input.SampleRate,Input.WndLenInSamples,Input.OverlapInSamples);
	Output.SetNumWindows(Input.NumWindows);
	WindowFeatures* f;
    fftw_complex* ptr = Input.OutputPtr;
    unsigned int freqBin = Input.NumFreqBins;
    
    //Calculate features values
	for(unsigned int i = 0; i < Output.GetNumWindows(); i++)
	{
        f = &Output.Window(i);
        f->Power = Input.WindowPowersPtr[i];
		f->FreqCentroid = ComputeArraySpectralCentroid(ptr + i * freqBin, freqBin);
		f->FreqSpread = ComputeArraySpectralSpread(ptr + i * freqBin, freqBin, f->FreqCentroid);
		f->GroupDelay = Input.Element(i,freqBin - 2)[1];
	}
    
	//Calculate feature deltas
	Output.CalculateDeltas();
	//Normalize feature deltas
	Output.NormalizeDeltas();
	//Normalize group delays
	Output.NormalizeGroupDelays();
	//Calculate rhythm interpolation
	Output.CalculateRhythmInterpolation();
}

void AudioAnalyzer::ComputeTransientValues(const AudioTrack* Input)
{
	//Copy sampling frequency to output (could come in handy)
    m_uiSampleRate = Input->getSampleRate();
	//Determine the window length in samples
    m_uiWndLenInSamples = (unsigned int)(floor((m_fWndLenInSecs*float(Input->getSampleRate()))+0.5f));
	//Determine the overlap in samples (function of window length and overlap)
	m_uiOverlapInSamples = (unsigned int)(m_uiWndLenInSamples*m_fOverlap);
	//Determine the window stride (windowlen - overlap)
	m_uiWndStride = m_uiWndLenInSamples - m_uiOverlapInSamples;
  
	//Calculate number of windows
    m_uiSamplesPerChannel = (int)Input->getSamplesCount() / Input->getChannelsCount();
	if(m_uiSamplesPerChannel <= m_uiWndLenInSamples)
		m_uiNumWindows = 1;
	else
	{
		m_uiNumWindows = (m_uiSamplesPerChannel - m_uiWndLenInSamples) / m_uiWndStride;
		if((m_uiSamplesPerChannel - m_uiWndLenInSamples) % m_uiWndStride != 0)
			m_uiNumWindows++;
		
		m_uiNumWindows++;
	}
    
	//Padded window length:
	m_uiPaddedWndLenInSamples = 1;
	//If padding has been disabled by the user, the padded window length is equal to the actual one
	if(!m_bUsePadding)
		m_uiPaddedWndLenInSamples = m_uiWndLenInSamples;
	else
	{
		//Otherwise, calculate the padded window's length so that it's equal to the smallest power of 2 greater than the actual window's length
		while(m_uiPaddedWndLenInSamples < m_uiWndLenInSamples)
            m_uiPaddedWndLenInSamples *= 2;
	}

	//Calculate the number of output frequency bins (this is a function of the length of the input to the FFT, in this case it's the length of the padded window)
	m_uiNumFreqBins = m_uiPaddedWndLenInSamples / 2 + 1;
}

//Converts a complex number into polar form
void AudioAnalyzer::ConvertToAmpliPhase(double* val)
{
	double Amplitude,Phase;
    
    double first = val[0], second = val[1];
    
    //Amplitude = 1 / fastInvSqrt(first * first + second * second);
    Amplitude = sqrt(first * first + second * second);
	Phase = atan(second / first);
	if(first == 0.0)
	{
		if(second < 0.0)
			Phase = -M_PI_2;
		else if(second == 0.0)
			Phase = 0.0;
		else
            Phase = M_PI_2;
	}
	else if(first < 0.0)
	{
		if(second < 0.0)
			Phase -= m_dMathPI;
		else
			Phase += m_dMathPI;
	}
	val[0] = Amplitude;
    val[0] = Phase;
}

//Unwraps the phase of an array of double values with custom stride
void AudioAnalyzer::UnwrapArrayPhase(double* StartValPtr, size_t Stride, unsigned int NumVals)
{
	unsigned int i;
	double *PtrCurr,*PtrPrev;
	double CurrVal, PrevVal;
    double div = 1 / (2.0 * m_dMathPI);
    double pi2 = 2.0 * m_dMathPI;
	for(i = 1; i < NumVals; i++)
	{
		PtrCurr = (double*)(((char*)StartValPtr) + (i * Stride));
		PtrPrev = (double*)(((char*)StartValPtr) + ((i - 1) * Stride));
		CurrVal = *PtrCurr;
		PrevVal = *PtrPrev;
        
		if(PrevVal - CurrVal > m_dMathPI)
		{
			CurrVal += pi2 * floor((PrevVal-CurrVal) * div);
			if(PrevVal - CurrVal > m_dMathPI)
				CurrVal += pi2;
		}
		else if(CurrVal - PrevVal > m_dMathPI)
		{
			CurrVal -= pi2 * floor((CurrVal - PrevVal) * div);
			if(CurrVal - PrevVal > m_dMathPI)
				CurrVal -= pi2;
		}
		*PtrCurr = CurrVal;
	}
}

//Compute the power of an array of values with custom stride as if they were representing samples of a waveform
double AudioAnalyzer::ComputeArrayPower(double* StartValPtr, size_t Stride, unsigned int NumVals)
{
	double Result = 0.0;
	for(unsigned int i = 0;i < NumVals; i++)
		Result += pow(*(double*)(((char*)StartValPtr) + (i * Stride)), 2.0);

	return Result;
}

//Computes the normalized spectral centroid for an array of complex values in amplitude/phase form
//Actual value is equal to normalized one times nyquist freq (half of sample rate)
double AudioAnalyzer::ComputeArraySpectralCentroid(fftw_complex* StartValPtr, unsigned int NumVals)
{
	unsigned int i;
	double TotalPow, HalfPow, LHSPow, FractionalPart, Res;
	
	TotalPow = 0.0;
	for(i = 0; i < NumVals; i++)
		TotalPow += StartValPtr[i][0] * StartValPtr[i][0];

	if(TotalPow <= 0.0)
		return 0.0;

	HalfPow = TotalPow / 2.0;

	LHSPow = 0.0;
	i = 0;
	LHSPow += StartValPtr[0][0] * StartValPtr[0][0];
	while(LHSPow < HalfPow)
	{
		i++;
		LHSPow += StartValPtr[i][0] * StartValPtr[i][0];
	}
    double temp = StartValPtr[i][0] * StartValPtr[i][0];
	LHSPow -= temp;
	FractionalPart = (HalfPow - LHSPow) / temp;
	Res = i + FractionalPart;
	Res = Res / NumVals;
    
    return Res;
}

//Computes the normalized spectral spread about NormalizedCenterFrequency
double AudioAnalyzer::ComputeArraySpectralSpread(fftw_complex* StartValPtr, unsigned int NumVals, double NormalizedCenterFrequency)
{
	double Result = 0.0;
    double rec = 1 / NumVals;
    
	for(unsigned int i = 0; i < NumVals; i++)
		Result += StartValPtr[i][0] * StartValPtr[i][0] * fabs(((i + 0.5) * rec) - NormalizedCenterFrequency);
    
	return Result;
}

//Normalize an array of double type values so that they lie between -1 and 1 while maintaining their proportionality
void AudioAnalyzer::NormalizeArray(double* StartValPtr, size_t Stride, unsigned int NumVals)
{
	unsigned int i;
	double MaxAbsVal = 0.0;
	double* CurrElPtr;
	for(i = 0; i < NumVals; i++)
	{
		CurrElPtr = (double*)(((char*)StartValPtr) + (i * Stride));
		if(fabs(*CurrElPtr) > MaxAbsVal)
			MaxAbsVal = fabs(*CurrElPtr);
	}
    MaxAbsVal = 1 / MaxAbsVal;
	for(i = 0; i < NumVals; i++)
	{
		CurrElPtr = (double*)(((char*)StartValPtr) + (i * Stride));
		*CurrElPtr = (*CurrElPtr) * MaxAbsVal;
	}
}
