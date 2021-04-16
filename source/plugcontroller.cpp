//------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include "../include/plugcontroller.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

namespace Steinberg {
namespace PanningMadness {

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugController::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		//---Create Parameters------------
		parameters.addParameter (STR16 ("Bypass"), 0, 1, 0,
									 Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
									 PanningMadnessParams::kBypassId);

		parameters.addParameter(new Vst::RangeParameter(
									STR16("Pan"), PanningMadnessParams::kPanId, nullptr,
									-1.0, 1.0, 0.0, 0, Vst::ParameterInfo::kCanAutomate));
		
		Vst::StringListParameter* panLawParameter = new Vst::StringListParameter(
			STR16("Pan Law"), PanningMadnessParams::kPanLawId, nullptr,
			Vst::ParameterInfo::kIsList);
		panLawParameter->appendString(STR16("Linear"));
		panLawParameter->appendString(STR16("Constant Power"));
		panLawParameter->appendString(STR16("-4.5 dB"));
		parameters.addParameter(panLawParameter);

		parameters.addParameter(STR16("Use Haas"), 0, 1, 0,
									Vst::ParameterInfo::kNoFlags,
									PanningMadnessParams::kUseHaasId);
	}
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read our parameters and bypass value...
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	bool boolval;
	double doubleval;

	if (streamer.readBool (boolval) == false)
		return kResultFalse;
	setParamNormalized (kBypassId, boolval);

	if (streamer.readDouble(doubleval) == false)
		return kResultFalse;
	setParamNormalized(kPanId, doubleval);

	if (streamer.readDouble(doubleval) == false)
		return kResultFalse;
	setParamNormalized(kPanLawId, doubleval);

	if (streamer.readBool(boolval) == false)
		return kResultFalse;
	setParamNormalized(kUseHaasId, boolval);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace
} // namespace Steinberg
