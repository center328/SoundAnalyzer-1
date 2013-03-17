#include "AudioSections.h"
#include <iostream>

AudioSections::IntervalFreqStruct::IntervalFreqStruct(void):
Len(0),
Occurrences(0),
MaxChainLen(0),
MaxChainNumEl(0),
Weight(0.0f)
{
	MemberIntervals.clear();
}

AudioSections::IntervalFreqStruct& AudioSections::IntervalFreqStruct::operator = (const AudioSections::IntervalFreqStruct& rhs)
{
	Len = rhs.Len;
	Occurrences = rhs.Occurrences;
	MaxChainLen = rhs.MaxChainLen;
	MaxChainNumEl = rhs.MaxChainNumEl;
	Weight = rhs.Weight;
	MemberIntervals = rhs.MemberIntervals;
	return *this;
}

AudioSections::AudioSections(void):
m_uiNumSections(0),
m_pSectionsArr(0x0)
{
}

AudioSections::~AudioSections(void)
{
	if(m_pSectionsArr)
		delete [] m_pSectionsArr;
}

unsigned int AudioSections::GetNumSections(void) const
{
	return m_uiNumSections;
}

float AudioSections::GetSectionLengthInMsecs(void) const
{
	return m_fSectionLengthInMsecs;
}
	
unsigned int AudioSections::GetSectionLengthInWindows(void) const
{
	return m_uiSectionLengthInWindows;
}

float AudioSections::GetGlobalBPM(void) const
{
	return m_fGlobalBPM;
}

AudioSectionStr& AudioSections::Section(unsigned int Index) const
{
	if(Index < m_uiNumSections)
		return m_pSectionsArr[Index];
	else
		return m_pSectionsArr[m_uiNumSections-1];
}

AudioSectionStr* AudioSections::GetSectionsArrPtr(void) const
{
	return m_pSectionsArr;
}

void AudioSections::Generate(AudioFeatures& Input, float SectionLenInMsecs)
{
	//Functional parameters
	double PeakDetectionStdDevMultFactor = 1.0;
	float ToleranceOverIntervalLen = 0.1f;

	//Auxiliary variables
	unsigned int i,j,k,l,TempIndex;

	//Calculate section length in msecs and windows
	m_fSectionLengthInMsecs = fabs(SectionLenInMsecs);
	m_uiSectionLengthInWindows = (unsigned int)(floor((m_fSectionLengthInMsecs / (float)Input.GetWindowStrideInMsecs()) + 0.5f));
	//Calculate number of sections
	m_uiNumSections = Input.GetNumWindows() / m_uiSectionLengthInWindows + 1;
	//Allocae memory for section data
	if(m_pSectionsArr)
		delete[] m_pSectionsArr;
	m_pSectionsArr = new AudioSectionStr[m_uiNumSections];
	
	//Calculate Beat locations
	double* TempArr;
	TempArr = new double[m_uiSectionLengthInWindows+2];
	std::vector<IntervalStruct> IntervalsVec;
	std::vector<IntervalFreqStruct> IntervalDurationsOccurrences;
	//For each section
	for(i = 0; i < m_uiNumSections; i++)
	{
		//copy rhythm interpolation value into a temporary array that's 2 elements bigger than the section size
		//first element is actually last window of previous section
		//and last el is first window of following one.
		//This is needed to find peaks at the limit of the window.
		for(j = 0; j < m_uiSectionLengthInWindows; j++)
		{
			TempIndex = i * m_uiSectionLengthInWindows + j;
			if(TempIndex < Input.GetNumWindows())
				TempArr[j + 1] = Input.Window(TempIndex).RhythmInterpolation;
			else
				TempArr[j + 1] = 0.0;
		}
        
		if(i > 0)
			TempArr[0] = Input.Window(i * m_uiSectionLengthInWindows - 1).RhythmInterpolation;
		else
			TempArr[0] = 0.0;
        
		if(i < m_uiNumSections - 1)
			TempArr[m_uiSectionLengthInWindows + 1] = Input.Window((i + 1) * m_uiSectionLengthInWindows).RhythmInterpolation;
		else
			TempArr[m_uiSectionLengthInWindows + 1] = 0.0;

		//Calculate mean and standard deviation for array
		double Mean,StandardDev;
		Mean = ArrayMean(TempArr, sizeof(double), m_uiSectionLengthInWindows + 2);
		StandardDev = ArrayStandardDev(TempArr, sizeof(double), m_uiSectionLengthInWindows + 2, Mean);
		//Find peaks in the array
		for(j = 1; j < m_uiSectionLengthInWindows + 1; j++)
		{
			//Find a peak (value having previous and next ones smaller than itself
			if(TempArr[j - 1] < TempArr[j] && TempArr[j + 1] < TempArr[j])
			{
				//Only consider it if it is far enough from the mean
				//Use standard deviation for this, choice of value based on it is arbitrary, i.e. I'll have to try and see what works.
				if(TempArr[j] > Mean + PeakDetectionStdDevMultFactor * StandardDev)
					m_pSectionsArr[i].BeatLocations.push_back(i * m_uiSectionLengthInWindows + (j - 1));
			}
		}
		
		//To proceed with calculation, more than 1 peak needs to be found.
		//If this is not the case, BPM is set to zero
		if(m_pSectionsArr[i].BeatLocations.size() > 1)
		{
			//Calculate intervals between peaks and store them in an array:
			//Calculate number of intervals
			j = (int)m_pSectionsArr[i].BeatLocations.size() - 1;
			j = (((j * j) - j) / 2) + j;
			IntervalsVec.clear();
			IntervalsVec.resize(j);
			TempIndex = 0;
			//Determine intervals
			for(j = 0; j < m_pSectionsArr[i].BeatLocations.size() - 1; j++)
			{
				for(k = j + 1; k < m_pSectionsArr[i].BeatLocations.size(); k++)
				{
					IntervalsVec[TempIndex].StartBeat = m_pSectionsArr[i].BeatLocations[j];
					IntervalsVec[TempIndex].EndBeat = m_pSectionsArr[i].BeatLocations[k];
					IntervalsVec[TempIndex].Len = IntervalsVec[TempIndex].EndBeat - IntervalsVec[TempIndex].StartBeat;
					TempIndex++;
				}
			}
			//Filter out intervals that are too long or too short (above 600bpm => below 100ms or below 30bpm => above 2000ms)
			//Should I stil be doing this?
			for(j = 0; j < IntervalsVec.size(); j++)
			{
				if((double(IntervalsVec[j].Len) * Input.GetWindowStrideInMsecs()) < 100.0)
				{
					IntervalsVec.erase(IntervalsVec.begin() + j);
					j = 0;
				}
				else if((double(IntervalsVec[j].Len) * Input.GetWindowStrideInMsecs()) > 2000.0)
				{
					IntervalsVec.erase(IntervalsVec.begin() + j);
					j = 0;
				}
			}

			//Find recurrence of different interval lengths:
			IntervalDurationsOccurrences.clear();
			for (j = 0; j < IntervalsVec.size(); j++)
			{
				bool NewIntervalLength = true;
				for(k = 0; k < IntervalDurationsOccurrences.size(); k++)
				{
					if(IntervalsVec[j].Len == IntervalDurationsOccurrences[k].Len)
						NewIntervalLength = false;
				}
				if (NewIntervalLength)
				{
					IntervalFreqStruct TempStr;
					TempStr.Len = IntervalsVec[j].Len;
					TempStr.Occurrences = 0;
					TempStr.MaxChainLen = 0;
					TempStr.MaxChainNumEl = 0;
					TempStr.MemberIntervals.clear();
					int Tolerance = int(fabs(ceil((float)TempStr.Len * ToleranceOverIntervalLen)));
					for(k = 0; k < IntervalsVec.size(); k++)
					{
						if((abs(int(IntervalsVec[k].Len) - int(TempStr.Len))) <= Tolerance)
						{
							TempStr.Occurrences++;
							TempStr.MemberIntervals.push_back(IntervalsVec[k]);
						}
					}
					//Do not consider as candidates interval lenghts only occurring once
					if(TempStr.Occurrences > 1)
						IntervalDurationsOccurrences.push_back(TempStr);
				}
			}

			//Find the mximum chain length for each interval length: chain length is the length in windows of the interval covered 
			//by contiguous occurrences of the given interval length.
			std::vector<unsigned int> UsedStartBeats;
			std::vector<unsigned int> Row;
			std::vector<std::vector<unsigned int> > Column;
			for(j = 0; j < IntervalDurationsOccurrences.size(); j++)
			{
				IntervalDurationsOccurrences[j].MaxChainNumEl = 0;
				IntervalDurationsOccurrences[j].MaxChainLen = 0;
				UsedStartBeats.clear();
				bool NewStartBeat, Prune;
				for(k=0;k<IntervalDurationsOccurrences[j].MemberIntervals.size();k++)
				{
					NewStartBeat = true;
					for(l=0;l<UsedStartBeats.size();l++)
					{
						if(IntervalDurationsOccurrences[j].MemberIntervals[k].StartBeat == UsedStartBeats[l])
							NewStartBeat = false;
					}
					if(NewStartBeat)
					{
						Row.clear();
						Column.clear();
						Row.push_back(IntervalDurationsOccurrences[j].MemberIntervals[k].StartBeat);
						Column.push_back(Row);
						UsedStartBeats.push_back(IntervalDurationsOccurrences[j].MemberIntervals[k].StartBeat);
						while(!Column.empty())
						{
							while(!Column[Column.size() - 1].empty())
							{
								Row.clear();
								for(l=0;l<IntervalDurationsOccurrences[j].MemberIntervals.size();l++)
								{
									if(IntervalDurationsOccurrences[j].MemberIntervals[l].StartBeat == Column[Column.size()-1][0])
									{
										Row.push_back(IntervalDurationsOccurrences[j].MemberIntervals[l].EndBeat);
										UsedStartBeats.push_back(IntervalDurationsOccurrences[j].MemberIntervals[l].EndBeat);
									}
								}
								Column.push_back(Row);
							}
							if((Column.size()-2) > IntervalDurationsOccurrences[j].MaxChainNumEl)
								IntervalDurationsOccurrences[j].MaxChainNumEl = (int)Column.size()-2;
							
							if((Column[Column.size()-2][0] - Column[0][0]) > IntervalDurationsOccurrences[j].MaxChainLen)
								IntervalDurationsOccurrences[j].MaxChainLen = Column[Column.size()-2][0] - Column[0][0]; 
							
							Prune = true;
							while(Prune)
							{
								Column.erase(Column.begin()+(Column.size()-1));
								Column[Column.size()-1].erase(Column[Column.size()-1].begin() + 0);
								if(Column[Column.size()-1].size() == 0)
								{
									if(!Column[0].empty())
										Prune = true;
									else
                                    {
										Column.erase(Column.begin() + 0);
										Prune = false;
									}
								}
							}
						}
					}
				}
			}

			
			//Calculate a weight for every length.
			//This is an arbitrary value meant to mean how much the length is representative of the actual beat length (higher is more representative, or better)
			//Two factors are considered: the length of the maximum chain over the section length and the number of occurrences
			//How to best use them to compute a weight is again going to be arbitrary; it is going to boil down to trial and error.
			for(j=0;j<IntervalDurationsOccurrences.size();j++)
			{
				IntervalDurationsOccurrences[j].Weight = (float(IntervalDurationsOccurrences[j].MaxChainLen)/float(m_uiSectionLengthInWindows))*log(float(IntervalDurationsOccurrences[j].MaxChainNumEl));
			}


			//Sort them according to the weight
			for(j=0;j<IntervalDurationsOccurrences.size();j++)
			{
				for(k=j;k<IntervalDurationsOccurrences.size();k++)
				{
					if(j!=k)
					{
						if(IntervalDurationsOccurrences[k].Weight > IntervalDurationsOccurrences[j].Weight)
						{
							IntervalFreqStruct Temp;
							Temp = IntervalDurationsOccurrences[k];
							IntervalDurationsOccurrences[k] = IntervalDurationsOccurrences[j];
							IntervalDurationsOccurrences[j] = Temp;
							j=0;
							k=0;
						}
					}
				}
			}

			//Choose intervals with highest weight and calculate their mean length to determine BPM
			if(IntervalDurationsOccurrences.empty())
				m_pSectionsArr[i].BPM = 0;
			else
			{
				double AvgIntervalLen = double(IntervalDurationsOccurrences[0].Len);
				float WeightControl = IntervalDurationsOccurrences[0].Weight;
				j=1;
				while((j<IntervalDurationsOccurrences.size()) && ((WeightControl - IntervalDurationsOccurrences[j].Weight) < 0.0001f))
				{
					AvgIntervalLen += double(IntervalDurationsOccurrences[j].Len);
					j++;
				}
				AvgIntervalLen = AvgIntervalLen / j;
				m_pSectionsArr[i].BPM = float(60000.0/(AvgIntervalLen*Input.GetWindowStrideInMsecs()));
			}
		}
		//If only one peak is found there can be no intervals (of course) -> BPM is zero
		else
        {
            std::cout << "I don't want this to happen" << std::endl;
			m_pSectionsArr[i].BPM = 0;
        }
	}
	
	delete [] TempArr;

	//Compute Global BPM
	ComputeGlobalBPM();

	//Caculate the remaining parameters for sections
	for(i=0;i<m_uiNumSections;i++)
	{
		m_pSectionsArr[i].AvgPower = 0.0;
		m_pSectionsArr[i].AvgCentroid = 0.0;
		m_pSectionsArr[i].AvgSpread = 0.0;
		k=0;
		for(j=0;j<m_uiSectionLengthInWindows;j++)
		{
			TempIndex = i*m_uiSectionLengthInWindows+j;
			if(TempIndex < Input.GetNumWindows())
			{
				m_pSectionsArr[i].AvgPower += Input.Window(TempIndex).Power;
				m_pSectionsArr[i].AvgCentroid += Input.Window(TempIndex).FreqCentroid;
				m_pSectionsArr[i].AvgSpread += Input.Window(TempIndex).FreqSpreadDelta;
				k++;
			}
		}
		m_pSectionsArr[i].AvgPower = m_pSectionsArr[i].AvgPower/double(k);
		m_pSectionsArr[i].AvgCentroid = m_pSectionsArr[i].AvgCentroid/double(k);
		m_pSectionsArr[i].AvgSpread = m_pSectionsArr[i].AvgSpread/double(k);
	}
}

//Compute the mean of an array of doubles 
double AudioSections::ArrayMean(double* StartValPtr, size_t Stride, unsigned int NumVals)
{
	unsigned int i;
	double* CurrElPtr;
	double Result = 0.0;
	
	for(i=0;i<NumVals;i++)
	{
		CurrElPtr = (double*)(((char*)StartValPtr)+(i*Stride));
		Result += *CurrElPtr;
	}
	Result = Result/double(NumVals);

	return Result;
}

//Compute the standard deviation of an array of doubles
double AudioSections::ArrayStandardDev(double* StartValPtr, size_t Stride, unsigned int NumVals, double Mean)
{
	unsigned int i;
	double* CurrElPtr;
	double Result = 0.0;
	
	for(i=0;i<NumVals;i++)
	{
		CurrElPtr = (double*)(((char*)StartValPtr)+(i*Stride));
		Result += pow(((*CurrElPtr)-Mean),2.0);
	}
	Result = Result / NumVals;
	Result = sqrt(Result);

	return Result;
}

//APPROXIMATION: computes the most recurring BPM value over the songs and sets it for all sections.
//This assumes the song has constant BPM for its entire length to use all sections as individual observations
//in order to have a better chance at estimating the true BPM value.
void AudioSections::ComputeGlobalBPM(void)
{
	float Tolerance = 3.0f;

	unsigned int i,j;
	bool NewBPMValFound;
	BPMOccurrence CurrBPMOccurrence;
	std::vector<BPMOccurrence> BPMOccurrencesVec;
	BPMOccurrencesVec.clear();

	//Floor all BPM values
	for(i=0;i<m_uiNumSections;i++)
    {
        //std::cout << m_pSectionsArr[i].BPM << std::endl;
        m_pSectionsArr[i].BPM = floor(m_pSectionsArr[i].BPM);
    }

	//Find recurrences for every BPM value in the array within a specified tolerance
	for(i=0;i<m_uiNumSections;i++)
	{
		NewBPMValFound = true;
		for(j=0;j<BPMOccurrencesVec.size();j++)
		{
			if(m_pSectionsArr[i].BPM == BPMOccurrencesVec[j].BPM)
				NewBPMValFound = false;
		}
		if(NewBPMValFound)
		{
			CurrBPMOccurrence.BPM = m_pSectionsArr[i].BPM;
			CurrBPMOccurrence.Occurrences = 0;
			for(j=0;j<m_uiNumSections;j++)
			{
				if(fabs(CurrBPMOccurrence.BPM - m_pSectionsArr[j].BPM) < Tolerance)
					CurrBPMOccurrence.Occurrences++;
			}
			BPMOccurrencesVec.push_back(CurrBPMOccurrence);
		}
	}

	//Sort occurrences array by number of occurrences
	for(i=0;i<BPMOccurrencesVec.size();i++)
	{
		for(j=i;j<BPMOccurrencesVec.size();j++)
		{
			if(i!=j)
			{
				if(BPMOccurrencesVec[j].Occurrences > BPMOccurrencesVec[i].Occurrences)
				{
					CurrBPMOccurrence = BPMOccurrencesVec[j];
					BPMOccurrencesVec[j] = BPMOccurrencesVec[i];
					BPMOccurrencesVec[i] = CurrBPMOccurrence;
				}
			}
		}
	}

	//Calculate average of most recurring BPM values
	j=0;
	while((j <BPMOccurrencesVec.size()) && (BPMOccurrencesVec[j].Occurrences==BPMOccurrencesVec[0].Occurrences))
		j++;

	CurrBPMOccurrence.BPM = 0.0f;
	for(i=0;i<j;i++)
		CurrBPMOccurrence.BPM += BPMOccurrencesVec[i].BPM;
	CurrBPMOccurrence.BPM = CurrBPMOccurrence.BPM/float(j);

	//Record Global BPM
	m_fGlobalBPM = CurrBPMOccurrence.BPM;
}
