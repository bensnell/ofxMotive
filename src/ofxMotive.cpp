#include "ofxMotive.h"

// --------------------------------------------------------------
ofxMotive::ofxMotive() {

	status = new MotiveStatus();
}

// --------------------------------------------------------------
ofxMotive::~ofxMotive() {
	stop();
	waitForThread(false, 3000);
}

// --------------------------------------------------------------
void ofxMotive::setupParams() {

	RUI_NEW_GROUP("ofxMotive");
	RUI_SHARE_PARAM_WCN("Motive- Force Try Connect", bForceTryConnect);
	RUI_SHARE_PARAM_WCN("Motive- Try Connect Timestep", tryConnectTimestep, 0, 10000);
	RUI_SHARE_PARAM_WCN("Motive- Max Try Connect Attempts", maxTryConnectAttempts, 1, 10);
	RUI_SHARE_PARAM_WCN("Motive- Profile Path", profilePath);
	RUI_SHARE_PARAM_WCN("Motive- Reload Profile", bForceLoadProfile);
	RUI_SHARE_PARAM_WCN("Motive- Save Profile", bSaveProfile);
	RUI_SHARE_PARAM_WCN("Motive- Calibration Path", calibrationPath);
	RUI_SHARE_PARAM_WCN("Motive- Reload Calibration", bForceLoadCalibration);
	RUI_SHARE_PARAM_WCN("Motive- Force Disconnect", bForceDisconnect);
	RUI_SHARE_PARAM_WCN("Motive- Process All Frames", bProcessAllFrames);
	RUI_SHARE_PARAM_WCN("Motive- Flush Camera Queues", bFlushCameraQueues);

	status->setup();

	reconstruction.setupParams();

	cameras.setupParams();
}

// --------------------------------------------------------------
void ofxMotive::setCalibrationPath(string _calibrationPath) {
	calibrationPath = _calibrationPath;
	if (isState(MOTIVE_CONNECTED)) loadCalibration();
}

// --------------------------------------------------------------
void ofxMotive::setProfilePath(string _profilePath) {
	profilePath = _profilePath;
	if (isState(MOTIVE_CONNECTED)) loadProfile();
}

// --------------------------------------------------------------
void ofxMotive::start() {

	// Begin the thread that will connect.
	// If blocking is true, then any lock on this thread will wait until its acquired. Otherwise, 
	// attempting to lock it will either return true or false.
	
	if (!isThreadRunning()) startThread(bBlocking);
	connect();
}

// --------------------------------------------------------------
MotiveState ofxMotive::getState() {
	return state;
}

// --------------------------------------------------------------
bool ofxMotive::isState(MotiveState _state) {
	return state == _state;
}

// --------------------------------------------------------------
bool ofxMotive::isConnected() {
	return isState(MOTIVE_CONNECTED);
}

// --------------------------------------------------------------
void ofxMotive::connect() {
	// Change the state of this thread
	if (isConnected()) {
		ofLogVerbose("ofxMotive") << "ofxMotive is already connected.";
	}
	else if (isState(MOTIVE_TRY_CONNECT)) {
		ofLogVerbose("ofxMotive") << "ofxMotive is already trying to connect";
	}
	else {
		startTryConnect();
	}
}

// --------------------------------------------------------------
void ofxMotive::startTryConnect() {
	setState(MOTIVE_TRY_CONNECT);
	lastTryConnectTime = -tryConnectTimestep;
	tryConnectCounter = 0;
}

// --------------------------------------------------------------
void ofxMotive::stop() {
	// Disconnect the system and cameras
	disconnect();

	// Stop the main thread
	if (isThreadRunning()) {
		waitForThread(true, 3000);
		//stopThread(); // this crashes
	}
}

// --------------------------------------------------------------
void ofxMotive::disconnect() {
	// If we're connected, disconnect all cameras first
	if (!isState(MOTIVE_DISCONNECTED)) {
		setState(MOTIVE_DISCONNECTED);
		shutdown();
	}
}

// --------------------------------------------------------------
void ofxMotive::setState(MotiveState _state) {
	state = _state;
};

// --------------------------------------------------------------
bool ofxMotive::isSuccess(NPRESULT result, string fnctName) {
	if (result == NPRESULT_SUCCESS) return true;
	string resultString(TT_GetResultString(result));
	ofLogError("ofxMotive" + (fnctName.empty() ? "" : (" : " + fnctName))) << resultString;
	return false;
}

// --------------------------------------------------------------
bool ofxMotive::initialize() {
	bool bSuccess = isSuccess(TT_Initialize(), "Initialize");
	if (bSuccess) status->attachListeners();
	return bSuccess;
}

// --------------------------------------------------------------
bool ofxMotive::shutdown() {
	// this automatically saves the system calibration file "System Calibration.cal"
	status->detachListeners();
	bool bSuccess = isSuccess(TT_Shutdown(), "Shutdown");
	if (!bSuccess) status->attachListeners();
	return bSuccess;
}

// --------------------------------------------------------------
bool ofxMotive::loadProfile() {
	if (profilePath == "") return false;
	bool success = isSuccess(TT_LoadProfile(profilePath.c_str()), "Load Profile");
	if (success) ofLogNotice("ofxMotive") << "Loaded Profile: " << profilePath;
	return success;
}

// --------------------------------------------------------------
bool ofxMotive::saveProfile() {
	if (profilePath == "") return false;
	bool success = isSuccess(TT_SaveProfile(profilePath.c_str()), "Save Profile");
	if (success) ofLogNotice("ofxMotive") << "Saved Profile: " << profilePath;
	return success;
}

// --------------------------------------------------------------
bool ofxMotive::loadCalibration() {
	if (calibrationPath == "") return false;
	bool success = isSuccess(TT_LoadCalibration(calibrationPath.c_str()), "Load Calibration");
	if (success) ofLogNotice("ofxMotive") << "Loaded Calibration: " << calibrationPath;
	return success;
}

// --------------------------------------------------------------
void ofxMotive::threadedFunction() {

	while (isThreadRunning()) {

		// Update parameters if any have changed
		if (bForceTryConnect) {
			bForceTryConnect = false;
			RUI_PUSH_TO_CLIENT();
			startTryConnect();
		}
		if (bForceLoadProfile) {
			bForceLoadProfile = false;
			RUI_PUSH_TO_CLIENT();
			loadProfile();
		}
		if (bSaveProfile) {
			bSaveProfile = false;
			RUI_PUSH_TO_CLIENT();
			saveProfile();
		}
		if (bForceLoadCalibration) {
			bForceLoadCalibration = false;
			RUI_PUSH_TO_CLIENT();
			loadCalibration();
		}
		if (bForceDisconnect) {
			bForceDisconnect = false;
			RUI_PUSH_TO_CLIENT();
			disconnect();
		}
		if (bFlushCameraQueues) {
			bFlushCameraQueues = false;
			RUI_PUSH_TO_CLIENT();
			cameras.flushQueues();
		}

		// Do something different depending on the state
		switch (state) {
		case MOTIVE_TRY_CONNECT: {

			// Try to connect every timestep
			unsigned long thisTime = ofGetElapsedTimeMillis();
			if (thisTime - lastTryConnectTime >= tryConnectTimestep) {
				lastTryConnectTime = thisTime;
				tryConnectCounter++;

				ofLogNotice("ofxMotive") << "Motive is attempting to connect...";

				// Check if the license is already being used
				isSuccess(TT_TestSoftwareMutex(), "Test Software Mutex");

				// Try to initialize the system
				if (initialize()) {
					ofLogNotice("ofxMotive") << "Motive has connected";
					setState(MOTIVE_CONNECTED);
					loadProfile(); // load profile
					loadCalibration();
					update(); // init camera list
				}
			}

			// If we've surpassed the limit, then stop trying to connect
			if (tryConnectCounter >= maxTryConnectAttempts) {
				ofLogNotice("ofxMotive") << "Exceeded max number of connection attempts. Exiting.";
				setState(MOTIVE_DISCONNECTED);
			}

		}; break;
		case MOTIVE_CONNECTED: {

			// Get all new information from the cameras
			processNewData();

		}; break;
		case MOTIVE_DISCONNECTED: default: {

			// Don't run this thread too much if disconnected
			ofSleepMillis(200);

		}; break;
		}
	}
}

// --------------------------------------------------------------
bool ofxMotive::update(bool bSingleFrame) {
	
	// Update the system and retrieve the result
	NPRESULT result = NPRESULT_SUCCESS;
	if (bSingleFrame) {
		result = TT_UpdateSingleFrame();
	}
	else {
		result = TT_Update();
	}
	
	// Return a bool and print any other useful information
	if (result == NPRESULT_SUCCESS) return true;
	else if (result != NPRESULT_NOFRAMEAVAILABLE) {
		isSuccess(result, "Update");
	}
	return false;
}

// --------------------------------------------------------------
void ofxMotive::processNewData() {

	// While we can update the camera feed, update the information we're receiving.
	// This returns true if there is another frame.
	while (update(bProcessAllFrames)) {

		// Update the camera information (fps, serial numbers, etc.)
		cameras.update();

		// Get the 3D information
		lock(); // This must be locked since other processes use the reconstruction markers
		reconstruction.update();
		unlock();

		// Update the status of the system
		status->update(reconstruction, cameras);

		// Update the event
		MotiveEventArgs args;
		args.maybeNeedsCalibration = status->maybeNeedsCalibration();
		// Add all markers
		for (int i = 0; i < reconstruction.markers.size(); i++) {
			MotiveOutputMarker o;
			o.position = reconstruction.markers[i].position;
			o.cuid = reconstruction.markers[i].ID;
			args.markers.push_back(o);
		}
		// Add all active cameras
		auto activeCams = cameras.getActiveCameras();
		for (int i = 0; i < activeCams.size(); i++) {
			MotiveOutputCamera o;
			o.ID = activeCams[i]->ID;
			o.serial = activeCams[i]->serial;
			o.position = activeCams[i]->position;
			o.orientation = activeCams[i]->orientation;
			o.maybeNeedsCalibration = activeCams[i]->flagPossibleMisalignment;
			args.cameras.push_back(o);
		}

		ofNotifyEvent(newDataReceived, args);
	}
}

// --------------------------------------------------------------
MotiveCameraSet* ofxMotive::getCameras() {
	return &cameras;
}

// --------------------------------------------------------------
int ofxMotive::getNumCameras() {
	return cameras.getNumCameras();
}

// --------------------------------------------------------------
vector<glm::vec2> ofxMotive::get2DPoints(int serial) { // should be locking
	vector<glm::vec2> out;
	if (cameras.camExists(serial)) {
		out = cameras.getCameraFromSerial(serial)->markers2DRaw;
	}
	return out;
}

// --------------------------------------------------------------
vector<MotiveOutputMarker> ofxMotive::get3DPoints() { // this is problematic
	vector<MotiveOutputMarker> output;
	lock();

	vector<MotiveMarker> tmp = reconstruction.markers;

	unlock();
	for (int i = 0; i < tmp.size(); i++) {
		MotiveOutputMarker o;
		o.position = tmp[i].position;
		o.cuid = tmp[i].ID;
		output.push_back(o);
	}
	return output;
}

// --------------------------------------------------------------
bool ofxMotive::maybeNeedsCalibration() {
	return status == NULL ? false : status->maybeNeedsCalibration();
}

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
