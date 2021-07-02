#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxMotiveCameraSettings.h"

#include "NPTrackingTools.h"

class MotiveCamera : public MotiveCameraSettings {
public:

	MotiveCamera();
	~MotiveCamera();

	// Setup params for the RUI per camera
	void setupParams();
	// TODO: Disabling parameters works with RUI, but RUI does not allow
	// parameter groups to be disabled. As a result, this function does not
	// currently work as intended.
	//void disableParams();


	// ====================================
	// IDENTIFIABLE PARAMETERS
	// (params used to identify this camera)

	// Identifiers for each camera
	int index = -1;				// index in the array of active cameras from TT_CameraCount()
	int ID = -1;				// unique ID assigned to every camera (displayed on cameras)
	int serial = -1;			// unique for a camera
	string name = "";			// name and serial number


	// ====================================
	// UNSETTABLE, UNCHANGING PARAMETERS
	// (cannot be set by user, don't change over the course of execution)

	// Number of available gain levels
	int imagerGainLevels = 1; 

	// Image Params
	glm::ivec2 resolution;


	// ====================================
	// UNSETTABLE, CHANGING PARAMETERS
	// (cannot be set by user; may change over the course of execution)

	// Is the camera currently online?
	bool bConnected = false;

	// Location Params
	// (can change if continuous calibration is enabled)
	glm::vec3 position;
	glm::quat orientation;

	// Camera temperature
	float temperature; // celcius


	// ====================================
	// SETTABLE PARAMETERS
	// (can be set by user)

	// (These are inherited from MotiveCameraSettings

	// -------------------------------------

	// This pushes video type, exposure, thresh and intensity to the camera
	bool pushSettings();
	bool bPushSettings = false;


	// ====================================
	// MISC HELPERS

	// Flag whether this camera may be misaligned.
	bool flagPossibleMisalignment = false;
	float avgFlagPossibleMisalignment = 0.0;
	uint64_t lastPossibleMisalignmentTimeMS = 0;

	// call this after disconnecting
	void clearFrameData();

	// Frame Data:
	// Markers
	vector<glm::vec2> markers2DRaw; // unfiltered by size, roundness

	// Drawing 
	//bool bDraw = false;

	// TODO: 2D FILTER SETTINGS

private:

	// also: intrinsic opencv parameters

	// Get this camera's fingerprint
	string fpt();

	bool bSetupParams = false;
};

