#ifndef RHS_CALIBRATION_HDR
#define RHS_CALIBRATION_HDR

#include <string>

namespace rhs {
	void performCalibration(float width, float height);
	const std::string PathToCalibrationData = "calibration.yaml";
	const std::string PerspectiveTransformationName = "perspectiveTransformMatrix";
}

#endif