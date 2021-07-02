#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "NPTrackingTools.h"

// Given a cUID, if a marker active?
bool isMarkerActive(Core::cUID ID);

// What is the marker's ID? If it isn't active, -1 is returned.
int getActiveMarkerID(Core::cUID ID);

struct MarkerContribution {
	int cameraIndex;
	int cameraSerial;
	glm::vec2 centroid; // 2d location of the contributing point (raw?)
};

class MotiveMarker {
public:

	MotiveMarker();
	~MotiveMarker();

	glm::vec3 position;
	Core::cUID ID;
	float residual;
	vector<MarkerContribution> contributions;
	double timestamp;
	

	string getIDString();
};

