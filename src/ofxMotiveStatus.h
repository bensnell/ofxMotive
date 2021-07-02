#pragma once

#include "ofMain.h"

#include "NPTrackingTools.h"

#include "ofxRemoteUIServer.h"
#include "ofxMotiveReconstruction.h"
#include "ofxMotiveCameraSet.h"

// Use this listener to check for camera bumps
class MotiveStatus : private TTAPI cTTAPIListener {
public:

	MotiveStatus();
	~MotiveStatus();

	void setup();

	void attachListeners();
	void detachListeners();

	void update(MotiveReconstruction& recon, MotiveCameraSet& cams);

	// Is it possible that the system is out of alignment? (i.e. that calibration
	// needs to be done again?)
	bool maybeNeedsCalibration() { return bMaybeNeedsCalibration; }

	// Is it possible that Motive's calibration file is empty (default)?
	bool maybeEmptyCalibration() { return bMaybeEmptyCalibration; }

private:
	
	// How many markers fill each bin for a given camera?
	class MarkerCounter {
	public:
		bool bInit = false; // has this counter already been initialized?

		float total = 0;		// total # visible 2D markers
		float contr = 0;		// # 2D markers contributing to reconstruction
		float missing = 0;		// # 2D markers that should be contr (but aren't)
		
		// 0.95 default easing, 0 for no easing
		void update(float _total, float _contr, float _missing, float _easeParam) {
			float e = bInit ? _easeParam : 0;
			bInit = true;
			// Ease all values
			total = glm::mix(_total, total, _easeParam);
			contr = glm::mix(_contr, contr, _easeParam);
			missing = glm::mix(_missing, missing, _easeParam);
		}
	};
	// Map from camera to MarkerCounts
	map<int, MarkerCounter*> counters;
	float markerCountEasingParam = 0.8; // (reference FPS is 60)

	// This override's Motive's function
	bool ApplyContinuousCalibrationResult();
	bool flagApplyContinuousCalibration = false;
	// When was the last time Motive's API called to apply 
	// a continuous calibration result?
	uint64_t lastContinuousCalibrationCalledTimeMS = 0; // milliseconds
	// Should continuous calibration results be allowed to applied?
	bool bPassthroughAutoCal = true;

	void updateCameraMisalignmentFlags(MotiveReconstruction& recon, MotiveCameraSet& cams);
	// Does motive maybe need calibration?
	bool bMaybeNeedsCalibration = false;
	// Should we use sensor fusion across these two sources of information
	// about system and camera calibration, respectively?
	//	1.	[System] Motive API's Notifications about whether calibration may need to
	//		be completed again, and
	//	2.	[Cameras] Observed camera displacements / misalignments from marker data.
	// Possible Modes include:
	//		FUSE_SYSTEM_AND_CAMERAS
	//		-	These two sources of data are fused to provide more
	//			reliable indicators of an uncalibrated system.
	//		-	Cameras will not individually be marked as misaligned unless
	//			the API also reports a global misalignment.
	//		-	The system will not globally be marked as misaligned until at least one
	//			camera has been observed to be misalignment.
	//		-	A timeout is used to overlap measurements from different sources to
	//			provide the sensor fusion data.
	//		FUSE_SYSTEM_OR_CAMERAS
	//		-	Cameras will only be flagged if cameras observe a displacement.
	//		-	System will be flagged if either the system or any cameras 
	//			recently report a displacement.
	//		-	A timeout is used to support fusion
	//		FUSE_NONE
	//		-	Sources of data are not fused. The global and local misalignment flags 
	//			may not correspond. No timeout is used.
	enum FusionMode {
		FUSE_SYSTEM_AND_CAMERAS = 0,
		FUSE_SYSTEM_OR_CAMERAS,
		FUSE_NONE
	};
	FusionMode fusionMode = FUSE_SYSTEM_AND_CAMERAS;
	// Timeout for sensor fusion (in seconds)
	float fusionTimeoutSec = 5;
	// Ease the camera flags (as a buffer to noisy data).
	// (reference FPS is 60)
	// 0 to turn off the ease
	float cameraFlagEase = 0.92;
	// What is the threshold for flagging misalignment for each camera?
	float cameraFlagThreshold = 0.75;

	// Convert an easing parameter from one frame rate to another
	float convertEaseParam(float param, float fromFPS, float toFPS) {
		return exp(log(param) * fromFPS / toFPS);
	}

	// Is the calibration file empty?
	// If so, then we will observe the cameras sitting in a
	// perfect line.
	// (epsilon in meters)
	void updateEmptyCalibrationFlags(MotiveCameraSet& _cams);
	bool isCalibrationEmpty(MotiveCameraSet& cams, float epsilon = 0.001);
	bool bMaybeEmptyCalibration = false;
};
