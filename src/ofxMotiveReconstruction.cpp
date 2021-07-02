#include "ofxMotiveReconstruction.h"

// -----------------------------------------------------
MotiveReconstruction::MotiveReconstruction() {
}

// -----------------------------------------------------
MotiveReconstruction::~MotiveReconstruction() {
}

// -----------------------------------------------------
void MotiveReconstruction::setupParams() {

}

// -----------------------------------------------------
void MotiveReconstruction::update() {
	
	int nCameras = TT_CameraCount();

	markers.clear();
	int nMarkers = TT_FrameMarkerCount();
	//ofLogNotice("Recon") << "There are this many markers: " << nMarkers;
	for (int i = 0; i < nMarkers; i++) {
		
		// Get all attributes of this marker
		MotiveMarker m;
		m.position.x = TT_FrameMarkerX(i);
		m.position.y = TT_FrameMarkerY(i);
		m.position.z = TT_FrameMarkerZ(i);
		m.ID = TT_FrameMarkerLabel(i);
		m.residual = TT_FrameMarkerResidual(i);
		m.timestamp = TT_FrameTimeStamp();

		// Determine which cameras contributed to this marker
		for (int j = 0; j < nCameras; j++) {
			MarkerContribution ctr;
			if (TT_FrameCameraCentroid(i, j, ctr.centroid.x, ctr.centroid.y)) {
				ctr.cameraIndex = j;
				ctr.cameraSerial = TT_CameraSerial(j);
				m.contributions.push_back(ctr);
			}
		}
		
		// Add this marker
		markers.push_back(m);
	}
}
