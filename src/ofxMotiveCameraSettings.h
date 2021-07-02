#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

#include "NPTrackingTools.h"

enum MotiveCameraMode {
	MOTIVE_CAMERA_MODE_INVALID = -1,
	MOTIVE_CAMERA_MODE_SEGMENT, // 0
	MOTIVE_CAMERA_MODE_RAW_GRAYSCALE, // 1
	MOTIVE_CAMERA_MODE_OBJECT, // 2
	MOTIVE_CAMERA_MODE_PRECISION, // 4
	MOTIVE_CAMERA_MODE_MJPEG // 6
};

// Camera settings that can be set by the user
class MotiveCameraSettings {
public:

	MotiveCameraSettings();
	~MotiveCameraSettings();

	// Setup params for the RUI per camera
	void shareParams(string fpt = "");
	void unshareParams(string fpt = "");

	// Copy settings from another object
	void copySettingsFrom(MotiveCameraSettings& a);

	// ====================================
	// SETTABLE PARAMETERS
	// (can be set by user)

	// Is the camera enabled for reconstruction?
	eCameraStates camState = Camera_Enabled;

	// Capture params
	int exposure = 0; // in microseconds
	int threshold = 0;
	int intensity = 0; // intensity of IR light
	int frameRate;
	int imagerGain;

	// Image mode
	MotiveCameraMode camMode = MOTIVE_CAMERA_MODE_INVALID;
	// Helpers for getting and setting NP(T)racking(T)ools' native video types
	void setTTVideoType(int _type);
	int getTTVideoType();

	// Image Quality
	// initialized to arbitrary number
	// [0, 100] (low --> high quality)
	int mjpegQuality = 50;

};

