//------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Steinberg {
namespace PanningMadness {

//-----------------------------------------------------------------------------
class PlugProcessor : public Vst::AudioEffect
{
public:
	PlugProcessor ();

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API setBusArrangements (Vst::SpeakerArrangement* inputs, int32 numIns,
	                                       Vst::SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;

	tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& setup) SMTG_OVERRIDE;
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE;

//------------------------------------------------------------------------
	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;

	static FUnknown* createInstance (void*)
	{
		return (Vst::IAudioProcessor*)new PlugProcessor ();
	}

protected:
	bool mBypass = false;

	Vst::ParamValue mPan = 0.0;
	Vst::ParamValue mPanLaw = 0.0;
	bool mUseHaas = false;

	Vst::SampleRate mSampleRate = 0;
	std::vector<float> mRingBuffer;
	int64 mRingBufferWritePos = 0;

	const double mMaxDelay = 0.05;
	
	//BiquadFilter mFilters[2];
};

//------------------------------------------------------------------------
//class BiquadFilter
//{
//public:
//	inline void setCoeffs(float _b0, float _b1, float _b2, float _a1, float _a2)
//	{
//		b0 = _b0;
//		b1 = _b1;
//		b2 = _b2;
//		a1 = _a1;
//		a2 = _a2;
//	}

//	inline float process(float input)
//	{
//		//float res = (b0 * input + b1 * z1 + b2 * z2) / (1 + a1 * y1 + a2 * y2);
//		float res = b0 * input + b1 * z1 + b2 * z2 - a1 * y1 - a2 * y2;
//		y2 = y1;
//		y1 = res;
//		z2 = z1;
//		z1 = input;
//		return res;
//	}

//protected:
//	float b0 = 0., b1 = 0., b2 = 0., a1 = 0., a2 = 0.;
//	float z1 = 0., z2 = 0., y1 = 0., y2 = 0.;
//};
} // namespace
} // namespace Steinberg
