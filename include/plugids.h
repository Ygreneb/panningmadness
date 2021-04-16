//------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#pragma once

namespace Steinberg {
namespace PanningMadness {

// HERE are defined the parameter Ids which are exported to the host
enum PanningMadnessParams : Vst::ParamID
{
	kBypassId = 100,

	kPanId = 200,
	kPanLawId,
	kUseHaasId
};


// HERE you have to define new unique class ids: for processor and for controller
// you can use GUID creator tools like https://www.guidgenerator.com/
static const FUID MyProcessorUID (0x6C0286DA, 0x424440F1, 0xA3191E3E, 0x01129597);
static const FUID MyControllerUID (0x4D087E11, 0x295749DA, 0xB97DD1FD, 0x1E0A8F2F);

//------------------------------------------------------------------------
} // namespace PanningMadness
} // namespace Steinberg
