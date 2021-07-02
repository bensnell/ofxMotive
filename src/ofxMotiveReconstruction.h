#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "NPTrackingTools.h"
#include "ofxMotiveMarker.h"

class MotiveReconstruction {
public:

	MotiveReconstruction();
	~MotiveReconstruction();

	void setupParams();

	void update();

	vector<MotiveMarker> markers;

private:


};

