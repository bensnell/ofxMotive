#pragma once

#include "ofMain.h"

// If we're simulating motive, don't load 
//#ifndef SIMULATE_MOTIVE
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "NPTrackingTools.h"
//#endif

#include "ofxRemoteUIServer.h"
#include "ofxMotiveCameraSet.h"
#include "ofxMotiveReconstruction.h"
#include "ofxMotiveStatus.h"

enum MotiveState {
	MOTIVE_DISCONNECTED = 0,
	MOTIVE_TRY_CONNECT,
	MOTIVE_CONNECTED,
};

// The information output from this addon
struct MotiveOutputMarker {
	glm::vec3 position;
	Core::cUID cuid;
};

struct MotiveOutputCamera {
	glm::vec3 position;
	glm::quat orientation;
	int ID = -1;		// displayed on camera LED screen
	int serial = -1;	// hardware serial number (unique to camera)
	bool maybeNeedsCalibration = false;
};

// The event args output
class MotiveEventArgs : public ofEventArgs {
public:
	vector<MotiveOutputMarker> markers;
	vector<MotiveOutputCamera> cameras;
	bool maybeNeedsCalibration = false;
};

class ofxMotive : private ofThread {
public:

	/// \brief Create an object to connect with motive's cameras. There should only be one per program.
	ofxMotive();
	~ofxMotive();

	/// \brief Setup the RUI Params associated with this addon
	void setupParams();
	void setCalibrationPath(string _calibrationPath);
	void setProfilePath(string _profilePath);

	/// \brief Begin streaming and reconstructing information from the cameras
	void start();

	/// \brief Check if we're connected to the cameras and streaming info
	bool isConnected();

	bool isState(MotiveState _state);

	/// \brief Stop streaming and reconstructing
	void stop();

	/// \brief Get the state of this addon
	MotiveState getState();

	/// \brief Get the number of connected cameras
	int getNumCameras();
	
	/// \brief Get a pointer to all camera objects
	MotiveCameraSet* getCameras();

	/// \brief Get the observed 2D points from a specific camera feed
	vector<glm::vec2> get2DPoints(int camID);

	/// \brief Get the reconstructed 3D points
	vector<MotiveOutputMarker> get3DPoints(); // this is the output

	/// \brief Draw any debug info
	//void drawDebugInfo();

	// Event that occurs when new data is received
	ofEvent< MotiveEventArgs > newDataReceived;

	// Is it possible that motive needs to be re-calibrated?
	bool maybeNeedsCalibration();

	// Get the maximum frame rate.
	// Note: This may change depending on which cameras are connected
	int getMaxFPS() { return cameras.getMaxFPS(); }

private:

	/// \brief Try to connect after some time of being disconnected
	void connect();

	/// \brief Disconnect the system and disable all cameras.
	void disconnect();

	bool isSuccess(NPRESULT result, string fnctName = "");
	//void logResult(NPRESULT result, string functionName = "");

	MotiveState state = MOTIVE_DISCONNECTED;
	void setState(MotiveState _state);

	void threadedFunction();
	bool bBlocking = true;

	void startTryConnect();
	long lastTryConnectTime = 0;
	int tryConnectTimestep = 3000; // every 3 seconds
	int tryConnectCounter = 0;
	int maxTryConnectAttempts = 5;
	bool bForceTryConnect = false;

	bool bForceDisconnect = false;

	string profilePath = ofToDataPath("profile.motive");; // should be a .motive file (actually xml)
	bool bLoadDefaultProfile = false; // default is located in C:/ProgramData/Optitrack
	bool bForceLoadProfile = false;
	bool loadProfile();
	bool bSaveProfile = false;
	bool saveProfile();
	string getDefaultProfilePath();

	// Should we override the setting concerning continuous calibration
	// stored in the profile?
	bool bOverrideContinuousCalibration = false;
	enum ContinuousCalibrationType {
		DISABLED = 0,
		CONTINUOUS,
		CONTINUOUS_AND_BUMPED_CAMERA,
		NUM_TYPES
	};
	vector<string> getContinuousCalibrationTypes() {
		return { "Disabled","Continuous","Continuous + Bumped Camera" };
	}
	ContinuousCalibrationType continuousCalibrationType = ContinuousCalibrationType::DISABLED;
	bool getOverrideProfile(string refProfilePath, string& outProfilePath);
	string getContinuousCalibrationTypeDescription(ContinuousCalibrationType type) {
		auto types = getContinuousCalibrationTypes();
		return types[int(type) % types.size()];
	}

	string calibrationPath = ofToDataPath("calibration.cal");; // should be a .cal file
	bool bLoadDefaultCalibration = false; // default is located in C:/ProgramData/Optitrack/Motive
	bool bForceLoadCalibration = false;
	bool loadCalibration();
	string getDefaultCalibrationPath();

	bool initialize();
	bool update(bool bSingleFrame = false);
	bool shutdown();

	void processNewData();
	bool bProcessAllFrames = true;

	MotiveCameraSet cameras;

	bool bFlushCameraQueues = false;

	MotiveReconstruction reconstruction;

	MotiveStatus* status;

};

