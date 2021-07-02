#include "ofxMotiveCameraSet.h"

// --------------------------------------------------------
MotiveCameraSet::MotiveCameraSet() {

	// Set default camera settings
	// (these are arbitrary, but should not be invalid)
	defaultCameraSettings.camMode = MotiveCameraMode::MOTIVE_CAMERA_MODE_OBJECT;
	defaultCameraSettings.camState = eCameraStates::Camera_Enabled;
	defaultCameraSettings.exposure = 2000;
	defaultCameraSettings.threshold = 128;
	defaultCameraSettings.frameRate = 240;
	defaultCameraSettings.imagerGain = 3;
	defaultCameraSettings.intensity = 15;
	defaultCameraSettings.mjpegQuality = 50;
}

// --------------------------------------------------------
MotiveCameraSet::~MotiveCameraSet() {
	//delete all cameras
}

// --------------------------------------------------------
void MotiveCameraSet::setupParams() {

	// Setup template camera params
	RUI_NEW_GROUP("ofxMotive Camera Set");
	RUI_SHARE_PARAM_WCN("Init Cams With Defaults", bInitWithDefaults);
	RUI_SHARE_PARAM_WCN("Push Defaults to All Cams", bPushDefaults);
	defaultCameraSettings.shareParams("[DEFAULT]");

	// Setup individual camera params
	map<int, MotiveCamera*>::iterator it;
	for (it = cams.begin(); it != cams.end(); it++) {
		it->second->setupParams();
	}
}

// --------------------------------------------------------
bool MotiveCameraSet::camExists(int serial) {
	return (cams.find(serial) != cams.end());
}

// --------------------------------------------------------
MotiveCamera* MotiveCameraSet::getCamera(int serial) {
	if (!camExists(serial)) {
		cams[serial] = new MotiveCamera();
		cams[serial]->serial = serial;
	}
	return cams[serial];
}

// --------------------------------------------------------
void MotiveCameraSet::update() {

	// Update all cameras

	// First, get the set of current serial numbers
	nActiveCams = TT_CameraCount();
	thisSerials.clear();
	serial2Index.clear();
	for (int index = 0; index < nActiveCams; index++) {
		// Get this serial number
		int serial = TT_CameraSerial(index);
		thisSerials.push_back(serial);
		serial2Index[serial] = index;
	}
	sort(thisSerials.begin(), thisSerials.end());
	// Compare these new serial numbers with the old ones to find how cameras have changed state
	newCams.clear(); 
	newCams.resize(lastSerials.size() + thisSerials.size());
	lostCams.clear();
	lostCams.resize(lastSerials.size() + thisSerials.size());
	activeCams = thisSerials;
	// Get all new cameras
	vector<int>::iterator it = set_difference(
		thisSerials.begin(),
		thisSerials.end(),
		lastSerials.begin(),
		lastSerials.end(),
		newCams.begin());
	newCams.resize(it - newCams.begin());
	// Get all lost cameras
	it = set_difference(
		lastSerials.begin(),
		lastSerials.end(),
		thisSerials.begin(),
		thisSerials.end(),
		lostCams.begin());
	lostCams.resize(it - lostCams.begin());
	// Save these cams
	lastSerials = thisSerials;

	// Print information about cameras
	// ... if we've lost connection with any
	if (lostCams.size() > 0) {
		stringstream ss;
		ss << "Disconnected from cameras: ";
		for (int i = 0; i < lostCams.size(); i++) {
			if (i != 0) ss << ", ";
			ss << lostCams[i];
		}
		ofLogNotice("ofxMotive : Camera Set") << ss.str();
	}
	// ... if we've connected with any new ones
	if (newCams.size() > 0) {
		stringstream ss;
		ss << "Connected to cameras: ";
		for (int i = 0; i < newCams.size(); i++) {
			if (i != 0) ss << ", ";
			ss << newCams[i];
		}
		ofLogNotice("ofxMotive : Camera Set") << ss.str();
	}

	// Update the connection status of disconnected cameras
	for (int i = 0; i < lostCams.size(); i++) {
		getCamera(lostCams[i])->bConnected = false;

		// TODO: Ideally, we would also remove their parameters from the RUI,
		// but the RUI does not currently allow parameters to be re-added to prior
		// groups once removed.
		//getCamera(lostCams[i])->disableParams();
	}

	// Add any new cameras and set their status to connected
	for (int i = 0; i < newCams.size(); i++) {
		MotiveCamera* cam = getCamera(newCams[i]);
		cam->bConnected = true;
		cam->serial = newCams[i]; // redundant
		// Get all relevant information for this camera that isn't going to change every frame
		cam->index = serial2Index[cam->serial];
		cam->name = TT_CameraName(cam->index);
		cam->ID = TT_CameraID(cam->index);
		// camera state
		TT_CameraState(cam->index, cam->camState);
		// pixel resolution
		int width, height;
		if (TT_CameraPixelResolution(cam->index, width, height)) {
			cam->resolution.x = width;
			cam->resolution.y = height;
		}
		// frame rate
		cam->frameRate = TT_CameraFrameRate(cam->index);
		// video type
		cam->setTTVideoType(TT_CameraVideoType(cam->index));
		// exposure
		cam->exposure = TT_CameraExposure(cam->index);
		// threshold
		cam->threshold = TT_CameraThreshold(cam->index);
		// intensity
		cam->intensity = TT_CameraIntensity(cam->index);
		// imager gain
		cam->imagerGain = TT_CameraImagerGain(cam->index);
		cam->imagerGainLevels = TT_CameraImagerGainLevels(cam->index);
		// mask settings

		// Create new parameter groups
		cam->setupParams();

		// If we're initializing the cameras with defaults, then 
		// set the default settable params and push.
		if (bInitWithDefaults) {
			cam->copySettingsFrom(defaultCameraSettings);
			cam->pushSettings();
		}

		ofLogNotice("ofxMotive : Camera Set") << "Adding new camera " << cam->serial << 
			(bInitWithDefaults ? " with default settings" : "");
	}
	
	// Update settings for all cameras, if need be
	if (bPushDefaults) {
		bPushDefaults = false;
		RUI_PUSH_TO_CLIENT();
		for (int i = 0; i < activeCams.size(); i++) {
			MotiveCamera* cam = getCamera(activeCams[i]);
			cam->copySettingsFrom(defaultCameraSettings);
			cam->pushSettings();
		}
		ofLogNotice("ofxMotive : Camera Set") << "Pushed default settings to all cameras";
	}

	// Now, process all current cameras
	for (int i = 0; i < activeCams.size(); i++) {
		MotiveCamera* cam = getCamera(activeCams[i]);
		cam->bConnected = true; // redundant

		// Update their active index
		cam->index = serial2Index[cam->serial];
		cam->ID = TT_CameraID(cam->index);

		// Position and orientation might change if continuous calibration is enabled, 
		// so set these values every time.
		// Set position.
		cam->position.x = TT_CameraXLocation(cam->index);
		cam->position.y = TT_CameraYLocation(cam->index);
		cam->position.z = TT_CameraZLocation(cam->index);
		// Set orientation.
		glm::mat3 mat;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				mat[i][j] = TT_CameraOrientationMatrix(cam->index, i * 3 + j);
			}
		}
		cam->orientation = getQuaternion(mat);

		// Update all information that changes every frame
		cam->clearFrameData();
		// 2d markers
		int nMarkers2DRaw = TT_CameraMarkerCount(cam->index);
		for (int i = 0; i < nMarkers2DRaw; i++) {
			glm::vec2 tmp;
			if (TT_CameraMarker(cam->index, i, tmp.x, tmp.y)) {
				cam->markers2DRaw.push_back(tmp);
			}
		}
		//  3d markers / projected location


		// temperature
		cam->temperature = TT_CameraTemperature(cam->index);

		// Set all new information
		// push changes to this camera
		if (cam->bPushSettings) {
			cam->bPushSettings = false;
			cam->pushSettings();
		}

		// Set or clear a mask


	}
}

// --------------------------------------------------------
glm::quat MotiveCameraSet::getQuaternion(glm::mat3& rotMat) {

	// Implementation from:
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

	glm::quat q;
	float trace = rotMat[0][0] + rotMat[1][1] + rotMat[2][2];
	if (trace > 0) {
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (rotMat[2][1] - rotMat[1][2]) * s;
		q.y = (rotMat[0][2] - rotMat[2][0]) * s;
		q.z = (rotMat[1][0] - rotMat[0][1]) * s;
	}
	else {
		if (rotMat[0][0] > rotMat[1][1] && rotMat[0][0] > rotMat[2][2]) {
			float s = 2.0f * sqrtf(1.0f + rotMat[0][0] - rotMat[1][1] - rotMat[2][2]);
			q.w = (rotMat[2][1] - rotMat[1][2]) / s;
			q.x = 0.25f * s;
			q.y = (rotMat[0][1] + rotMat[1][0]) / s;
			q.z = (rotMat[0][2] + rotMat[2][0]) / s;
		}
		else if (rotMat[1][1] > rotMat[2][2]) {
			float s = 2.0f * sqrtf(1.0f + rotMat[1][1] - rotMat[0][0] - rotMat[2][2]);
			q.w = (rotMat[0][2] - rotMat[2][0]) / s;
			q.x = (rotMat[0][1] + rotMat[1][0]) / s;
			q.y = 0.25f * s;
			q.z = (rotMat[1][2] + rotMat[2][1]) / s;
		}
		else {
			float s = 2.0f * sqrtf(1.0f + rotMat[2][2] - rotMat[0][0] - rotMat[1][1]);
			q.w = (rotMat[1][0] - rotMat[0][1]) / s;
			q.x = (rotMat[0][2] + rotMat[2][0]) / s;
			q.y = (rotMat[1][2] + rotMat[2][1]) / s;
			q.z = 0.25f * s;
		}
	}
	return q;
}

// --------------------------------------------------------
void MotiveCameraSet::flushQueues() {
	TT_FlushCameraQueues();
}

// --------------------------------------------------------
vector<MotiveCamera*> MotiveCameraSet::getActiveCameras() {
	vector<MotiveCamera*> out;
	for (int i = 0; i < activeCams.size(); i++) {
		out.push_back(getCamera(activeCams[i]));
	}
	return out;
}

// --------------------------------------------------------
MotiveCamera* MotiveCameraSet::getCameraFromSerial(int serial) {
	if (!camExists(serial)) return NULL;
	return getCamera(serial);
}

// --------------------------------------------------------
int MotiveCameraSet::getMaxFPS() {
	int maxFPS = 0;
	for (int i = 0; i < activeCams.size(); i++) {
		maxFPS = max(maxFPS, getCamera(activeCams[i])->frameRate);
	}
	return maxFPS;
}

// --------------------------------------------------------

// --------------------------------------------------------

// --------------------------------------------------------

// --------------------------------------------------------

