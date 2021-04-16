//------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include "../include/plugprocessor.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

namespace Steinberg {
namespace PanningMadness {

//-----------------------------------------------------------------------------
PlugProcessor::PlugProcessor ()
{
	// register its editor class
	setControllerClass (MyControllerUID);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::initialize (FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	if (result != kResultTrue)
		return kResultFalse;

	//---create Audio In/Out buses------
	// we want a stereo Input and a Stereo Output
	addAudioInput (STR16 ("AudioInput"), Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("AudioOutput"), Vst::SpeakerArr::kStereo);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setBusArrangements (Vst::SpeakerArrangement* inputs,
                                                            int32 numIns,
                                                            Vst::SpeakerArrangement* outputs,
                                                            int32 numOuts)
{
	// we only support one in and output bus and these buses must have the same number of channels
	if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
	{
		return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setupProcessing (Vst::ProcessSetup& setup)
{
	// here you get, with setup, information about:
	// sampleRate, processMode, maximum number of samples per audio block
	mSampleRate = setup.sampleRate;
	
	return AudioEffect::setupProcessing (setup);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setActive (TBool state)
{
	if (state) // Initialize
	{
		// Allocate Memory Here
		// Ex: algo.create ();
		mRingBuffer.resize(mSampleRate * mMaxDelay + 1.0, 0.f);
		mRingBufferWritePos = 0;
	}
	else // Release
	{
		// Free Memory if still allocated
		// Ex: if(algo.isCreated ()) { algo.destroy (); }
		mRingBuffer.clear();
	}
	return AudioEffect::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::process (Vst::ProcessData& data)
{
	//--- Read inputs parameter changes-----------
	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData (index);
			if (paramQueue)
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				switch (paramQueue->getParameterId ())
				{
					case PanningMadnessParams::kPanId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							mPan = value;
						break;
					case PanningMadnessParams::kPanLawId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							mPanLaw = value;
						break;
					case PanningMadnessParams::kUseHaasId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
							mUseHaas = (value > 0.5);
						break;
					case PanningMadnessParams::kBypassId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
							mBypass = (value > 0.5);
						break;
				}
			}
		}
	}

	//--- Process Audio---------------------
	//--- ----------------------------------
	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	if (mBypass)
	{
		if (data.numSamples > 0 && data.inputs[0].channelBuffers32[0] != data.outputs[0].channelBuffers32[0])
		{
			memcpy(data.outputs[0].channelBuffers32[0], data.inputs[0].channelBuffers32[0], data.numSamples * sizeof(float));
		}

		if (data.numSamples > 0 && data.inputs[0].channelBuffers32[1] != data.outputs[0].channelBuffers32[1])
		{
			memcpy(data.outputs[0].channelBuffers32[1], data.inputs[0].channelBuffers32[1], data.numSamples * sizeof(float));
		}

		return kResultOk;
	}

	if (data.numSamples > 0 && mSampleRate > 0. && data.inputs[0].numChannels == 2 && data.outputs[0].numChannels == 2 && !mRingBuffer.empty())
	{
 		double pan = mPan * 2.0 - 1.0; // -1 to 1

		// --- Panning with Haas Effect
		if (mUseHaas)
		{
			float* in_const, * in_delay, * out_const, * out_delay;
			if (pan < 0.0)
			{
				in_const = data.inputs[0].channelBuffers32[0];
				in_delay = data.inputs[0].channelBuffers32[1];
				out_const = data.outputs[0].channelBuffers32[0];
				out_delay = data.outputs[0].channelBuffers32[1];
			}
			else
			{
				in_const = data.inputs[0].channelBuffers32[1];
				in_delay = data.inputs[0].channelBuffers32[0];
				out_const = data.outputs[0].channelBuffers32[1];
				out_delay = data.outputs[0].channelBuffers32[0];
			}

			for (int i = 0; i < data.numSamples; ++i)
			{
				//float mono = 0.5f * (data.inputs[0].channelBuffers32[0][i] + data.inputs[0].channelBuffers32[1][i]);

				if (abs(pan) <= 0.0001)
				{
					out_const[i] = in_const[i];
					out_delay[i] = in_delay[i];
					//out_const[i] = mono;
					//out_delay[i] = mono;
				}
				else
				{
					int64 readPos = mRingBufferWritePos - floor(abs(pan) * mMaxDelay * mSampleRate);
					while (readPos < 0)
					{
						readPos += mRingBuffer.size();
					}
					mRingBuffer[mRingBufferWritePos++] = in_delay[i];
					while (mRingBufferWritePos >= mRingBuffer.size())
					{
						mRingBufferWritePos -= mRingBuffer.size();
					}
					out_delay[i] = mRingBuffer[readPos];

					out_const[i] = in_const[i];
				}
			}
		}
		// --- Normal Panning
		// for panning laws, see: http://www.cs.cmu.edu/~music/icm-online/readings/panlaws/index.html
		else
		{
			float panL = 0.f, panR = 0.f;
			float pan_abs = abs(pan);
			// --- Linear Pan Law
			if (mPanLaw < 0.33)
			{
				panL = 1.f - pan_abs;
				panR = pan_abs;
			}
			// --- Constant Power Pan Law
			else if (mPanLaw < 0.66)
			{
				panL = cos(pan_abs * M_PI_2);
				panR = sin(pan_abs * M_PI_2);
			}
			// --- -4.5 dB: Compromise between other pan laws
			else
			{
				panL = sqrt(cos(mPan * M_PI_2) * (1.0 - pan_abs));
				panR = sqrt(sin(mPan * M_PI_2) * pan_abs);
			}

			float* outL = data.outputs[0].channelBuffers32[0];
			float* outR = data.outputs[0].channelBuffers32[1];
			float* inL = data.inputs[0].channelBuffers32[0];
			float* inR = data.inputs[0].channelBuffers32[1];

			for (int i = 0; i < data.numSamples; ++i)
			{
				//float mono = 0.5f * (data.inputs[0].channelBuffers32[0][i] + data.inputs[0].channelBuffers32[1][i]);
				
				if (abs(pan) <= 0.0001)
				{
					outL[i] = inL[i];
					outR[i] = inR[i];
				}
				else if (pan < 0.0)
				{
					outL[i] = inL[i] + panR * inR[i];
					outR[i] = panL * inR[i];
				}
				else
				{
					outR[i] = inR[i] + panR * inL[i];
					outL[i] = panL * inL[i];
				}
			}
		}
	}
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	// called when we load a preset or project, the model has to be reloaded

	IBStreamer streamer (state, kLittleEndian);

	if (streamer.readBool(mBypass) == false)
		return kResultFalse;

	if (streamer.readDouble(mPan) == false)
		return kResultFalse;

	if (streamer.readDouble(mPanLaw) == false)
		return kResultFalse;

	if (streamer.readBool(mUseHaas) == false)
		return kResultFalse;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::getState (IBStream* state)
{
	// here we need to save the model (preset or project)

	//double radius = mRadius;
	//double resonantFrequency = mResonantFrequency;
	//bool bypass = mBypass;

	IBStreamer streamer (state, kLittleEndian);
	streamer.writeBool (mBypass);
	streamer.writeDouble (mPan);
	streamer.writeDouble (mPanLaw);
	streamer.writeBool (mUseHaas);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace
} // namespace Steinberg
