#ifndef _AUDIO_SECTIONS_H__
#define _AUDIO_SECTIONS_H__

#include <vector>
#include "AudioFeatures.h"

struct AudioSectionStr
{
	double AvgPower, AvgCentroid, AvgSpread;
	float BPM;
	std::vector<unsigned int>BeatLocations;
};


class AudioSections
{
public:
	AudioSections(void);
	~AudioSections(void);
	unsigned int GetNumSections(void) const;
	float GetSectionLengthInMsecs(void) const;
	unsigned int GetSectionLengthInWindows(void) const;
	float GetGlobalBPM(void) const;
	AudioSectionStr& Section(unsigned int Index) const;
	AudioSectionStr* GetSectionsArrPtr(void) const;
	void Generate(AudioFeatures& Input, float SectionLenInMsecs);
	

protected:
	AudioSections(const AudioSections&);
	double ArrayMean(double* StartValPtr, size_t Stride, unsigned int NumVals);
	double ArrayStandardDev(double* StartValPtr, size_t Stride, unsigned int NumVals, double Mean);
	void ComputeGlobalBPM(void);

	struct IntervalStruct
	{
		unsigned int Len;
        unsigned int StartBeat;
        unsigned int EndBeat;
	};

	struct IntervalFreqStruct
	{
		IntervalFreqStruct(void);
		IntervalFreqStruct& operator=(const IntervalFreqStruct& rhs);
		unsigned int Len,Occurrences,MaxChainLen,MaxChainNumEl;
		float Weight;
		std::vector<IntervalStruct> MemberIntervals;
	};
    
    struct BPMOccurrence
	{
		float BPM;
		unsigned int Occurrences;
		BPMOccurrence& operator=(const BPMOccurrence& rhs) 
		{
			BPM = rhs.BPM;
			Occurrences = rhs.Occurrences;
			return *this;
		}
	};

	float m_fSectionLengthInMsecs;
	unsigned int m_uiSectionLengthInWindows;
	unsigned int m_uiNumSections;
	float m_fGlobalBPM;
	AudioSectionStr* m_pSectionsArr;
};

#endif
