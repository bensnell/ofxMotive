#include "ofxMotiveCamera.h"

// -----------------------------------------------------------
MotiveCamera::MotiveCamera() {

}

// -----------------------------------------------------------
MotiveCamera::~MotiveCamera() {

}

// -----------------------------------------------------------
void MotiveCamera::setupParams() {

	// Don't setup the same params twice
	if (bSetupParams) return;

	RUI_NEW_GROUP("ofxMotive Camera " + ofToString(serial));
	RUI_SHARE_PARAM_WCN("ID "+fpt(), ID, -1, 1000);
	RUI_SHARE_PARAM_WCN("Connected "+fpt(), bConnected);
	MotiveCameraSettings::shareParams(fpt());
	// Push settings to this camera
	RUI_SHARE_PARAM_WCN("Push camera settings "+fpt(), bPushSettings);

	// Draw a camera feed
	// Set a mask

	bSetupParams = true;
}

// -----------------------------------------------------------
//void MotiveCamera::disableParams() {
//
//	if (!bSetupParams) return;
//	
//	//RUI_NEW_GROUP("ofxMotive Camera " + ofToString(serial));
//	RUI_REMOVE_PARAM_WCN("ID " + fpt());
//	RUI_REMOVE_PARAM_WCN("Connected " + fpt());
//	MotiveCameraSettings::unshareParams(fpt());
//	RUI_REMOVE_PARAM_WCN("Push camera settings " + fpt());
//
//	bSetupParams = false;
//}

// -----------------------------------------------------------
void MotiveCamera::clearFrameData() {
	markers2DRaw.clear();
}

// -----------------------------------------------------------
bool MotiveCamera::pushSettings() {

	bool success = true;
	if (!TT_SetCameraSettings(
		index,
		getTTVideoType(),
		exposure,
		CLAMP(threshold, 0, 255),
		CLAMP(intensity, 0, 15))) {
		ofLogNotice("ofxMotive : Camera " + ofToString(serial)) << "Could not set camera video type, exposure, threshold or intensity";
		success &= false;
	}

	if (!TT_SetCameraFrameRate(index, frameRate)) {
		ofLogNotice("ofxMotive : Camera " + ofToString(serial)) << "Could not set frame rate";
		success &= false;
	}

	if (!TT_SetCameraMJPEGHighQuality(index, CLAMP(mjpegQuality, 0, 100))) {
		ofLogNotice("ofxMotive : Camera " + ofToString(serial)) << "Could not set MJPEG quality";
		success &= false;
	}
	
	imagerGain = CLAMP(imagerGain, 0, imagerGainLevels - 1);
	TT_SetCameraImagerGain(index, imagerGain);

	if (!TT_SetCameraState(index, camState)) {
		ofLogNotice("ofxMotive : Camera " + ofToString(serial)) << "Could not set camera state";
		success &= false;
	}

	// set mask
	// save a frame

	RUI_PUSH_TO_CLIENT();

	return success;
}

// -----------------------------------------------------------

string MotiveCamera::fpt() {
	return "[" + ofToString(serial) + "]";
}

// -----------------------------------------------------------

// -----------------------------------------------------------

// -----------------------------------------------------------

// -----------------------------------------------------------

// -----------------------------------------------------------

// -----------------------------------------------------------