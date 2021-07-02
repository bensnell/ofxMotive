#include "ofxMotiveStatus.h"

// --------------------------------------------------------------
MotiveStatus::MotiveStatus() {

}

// --------------------------------------------------------------
MotiveStatus::~MotiveStatus() {

}

// --------------------------------------------------------------
void MotiveStatus::attachListeners() {
	TT_AttachListener(this);
}

// --------------------------------------------------------------
void MotiveStatus::detachListeners() {
	TT_DetachListener(this);
}

// --------------------------------------------------------------
void MotiveStatus::setup() {

	RUI_NEW_GROUP("ofxMotive Status");
	RUI_SHARE_PARAM_WCN("Motive- Passthrough Auto Cal", bPassthroughAutoCal);
	RUI_SHARE_PARAM_WCN("Motive- Marker Count Easing", markerCountEasingParam, 0, 1);
	RUI_SHARE_ENUM_PARAM_WCN("Motive- Fusion Mode", fusionMode, FUSE_SYSTEM_AND_CAMERAS, FUSE_NONE, { "System And Cameras", "System Or Cameras", "None" });
	RUI_SHARE_PARAM_WCN("Motive- Fusion Timeout Sec", fusionTimeoutSec, 0, 86400);
	RUI_SHARE_PARAM_WCN("Motive- Fusion Camera Easing", cameraFlagEase, 0, 1);
	RUI_SHARE_PARAM_WCN("Motive- Fusion Camera Thresh", cameraFlagThreshold, 0, 1);
}

// --------------------------------------------------------------
bool MotiveStatus::ApplyContinuousCalibrationResult() {

	// Mark that calibration wants to be appliedflagApplyContinuousCalibration
	flagApplyContinuousCalibration = true;

	// Return true to allow it to be applied
	return bPassthroughAutoCal;
}

// --------------------------------------------------------------
void MotiveStatus::update(MotiveReconstruction& recon, MotiveCameraSet& cams) {

	// Update the misalignment flags for each camera
	updateCameraMisalignmentFlags(recon, cams);

	// Lastly, check if the calibration file is empty. 
	updateEmptyCalibrationFlags(cams);
}

// --------------------------------------------------------------
void MotiveStatus::updateCameraMisalignmentFlags(MotiveReconstruction& recon, MotiveCameraSet& cams) {

	uint64_t thisTime = ofGetElapsedTimeMillis();

	// Get all active cameras
	auto activeCams = cams.getActiveCameras();
	// TODO: even inactive ones?

	// Count contributions and missing for each camera to reconstruction
	vector<int> camContr; // cam index to # contr
	camContr.resize(activeCams.size(), 0);
	vector<int> camMissing; // cam index to # missing
	camMissing.resize(activeCams.size(), 0);
	vector<bool> contrCamIndices;
	contrCamIndices.resize(activeCams.size(), false);
	for (auto& m : recon.markers) {

		// Collect all contributing camera indices
		fill(contrCamIndices.begin(), contrCamIndices.end(), false);
		for (auto& c : m.contributions) {
			contrCamIndices[c.cameraIndex] = true;
		}

		// Iterate through all cameras, checking their status
		for (auto& c : activeCams) {
			if (contrCamIndices[c->index]) {
				// This camera is contributing to the reconstruction of this 3D marker.
				camContr[c->index]++;
			}
			else {
				// Should this camera be contributing? Is the 3D marker in view of this camera?
				float x = -1;
				float y = -1;
				TT_CameraBackproject(c->index, m.position.x, m.position.y, m.position.z, x, y);
				if (x >= 0 && x <= c->resolution.x && y >= 0 && y <= c->resolution.y) {
					// 3D marker is in view; camera should be contributing but isn't
					camMissing[c->index]++;
				}
			}
		}
	}

	// Tally the total counts for each camera and update each's camera's counts
	bool bAnyCamsPossiblyMisaligned = false;
	for (auto& c : activeCams) {

		// Add a counter for this camera if one doesn't yet exist
		if (counters.find(c->serial) == counters.end()) {
			counters[c->serial] = new MarkerCounter();
		}
		MarkerCounter* counter = counters[c->serial];

		// Calculate the new counts
		int total = c->markers2DRaw.size();
		int contr = camContr[c->index];
		//int notContr = total - contr;
		int missing = camMissing[c->index];

		// Check if these counts indicate that this camera might be out of alignment
		if (counter->bInit) {
			// Is the camera possibly out of alignment?
			bool tmp = (missing > 0) && (contr == 0) && (total >= counter->total);

			// This bool may be noisy, so ease it.
			c->avgFlagPossibleMisalignment = glm::mix(float(tmp),
				c->avgFlagPossibleMisalignment, convertEaseParam(cameraFlagEase, 60, c->frameRate));

			// Flag whether this is out of alignment, based on eased data
			c->flagPossibleMisalignment = c->avgFlagPossibleMisalignment >= cameraFlagThreshold;
		}

		// Mark this time of possible misalignment
		if (c->flagPossibleMisalignment) c->lastPossibleMisalignmentTimeMS = thisTime;

		// Update the counter for this camera with some easing parameter.
		counters[c->serial]->update(
			total,
			contr,
			missing,
			convertEaseParam(markerCountEasingParam, 60, c->frameRate)
		);
	}

	// Store the time of receiving the continuous calibration call
	if (flagApplyContinuousCalibration) {
		flagApplyContinuousCalibration = false;

		bMaybeNeedsCalibration = true;
		lastContinuousCalibrationCalledTimeMS = thisTime;
	}

	// If we're fusing, override all flags, using the timeout.
	if (fusionMode != FUSE_NONE) {

		// Reset all values
		bMaybeNeedsCalibration = false;
		for (auto& c : activeCams) c->flagPossibleMisalignment = false;

		// When is the last time, if at all, that a camera might have been misaligned?
		// TODO: account for inactive cams?
		uint64_t maxTime = 0;
		for (auto& c : activeCams) maxTime = max(maxTime, c->lastPossibleMisalignmentTimeMS);

		// Were any of the cameras possibly recently misaligned?
		bool bCameraPossiblyRecentlyMisaligned = (maxTime != 0 &&
			(float(thisTime - maxTime) / 1000.0 <= fusionTimeoutSec));
		// Was the system possibly recently in need of re-calibration?
		bool bSystemPossiblyRecentlyNeedsCalibration = (lastContinuousCalibrationCalledTimeMS != 0
			&& (float(thisTime - lastContinuousCalibrationCalledTimeMS) / 1000.0 <= fusionTimeoutSec));

		switch (fusionMode) {
		case FUSE_SYSTEM_AND_CAMERAS: {

			// If both are true...
			if (bSystemPossiblyRecentlyNeedsCalibration && bCameraPossiblyRecentlyMisaligned) {

				// ... The system needs recalibration.
				bMaybeNeedsCalibration = true;

				// ... Appropriate cameras need recalibration.
				for (auto& c : activeCams) {
					c->flagPossibleMisalignment = (c->lastPossibleMisalignmentTimeMS != 0)
						&& (float(thisTime - c->lastPossibleMisalignmentTimeMS) / 1000.0 <= fusionTimeoutSec);
				}
			}

		}; break;
		case FUSE_SYSTEM_OR_CAMERAS: {

			// If either is true...
			if (bSystemPossiblyRecentlyNeedsCalibration && bCameraPossiblyRecentlyMisaligned) {

				// ... The system needs recalibration.
				bMaybeNeedsCalibration = true;

				// ... Appropriate cameras need recalibration.
				if (bCameraPossiblyRecentlyMisaligned) {
					for (auto& c : activeCams) {
						c->flagPossibleMisalignment = (c->lastPossibleMisalignmentTimeMS != 0)
							&& (float(thisTime - c->lastPossibleMisalignmentTimeMS) / 1000.0 <= fusionTimeoutSec);
					}
				}
			}

		}; break;
		default: {}; break;
		}
	}
}

// --------------------------------------------------------------
void MotiveStatus::updateEmptyCalibrationFlags(MotiveCameraSet& _cams) {

	// Update flag
	bMaybeEmptyCalibration = isCalibrationEmpty(_cams);

	// If maybe empty, flag all cameras as misaligned
	if (bMaybeEmptyCalibration) {
		auto activeCams = _cams.getActiveCameras();
		for (auto& c : activeCams) c->flagPossibleMisalignment = true;
	}
}

// --------------------------------------------------------------
bool MotiveStatus::isCalibrationEmpty(MotiveCameraSet& _cams, float epsilon) {

	// Get all active cams
	auto cams = _cams.getActiveCameras();

	// There must be cams to proceed
	if (cams.size() == 0) return false;

	// Check all cameras' positions.
	// If they are in a line, then return true.
	glm::vec3 sumPositions = glm::vec3(0, 0, 0);
	for (auto& c : cams) sumPositions += c->position;
	int nEmptyAxes = 0;
	for (int i = 0; i < 3; i++) {
		if (abs(sumPositions[i]) <= epsilon) nEmptyAxes++;
	}
	// Calibration is empty if two (or more) axes are zero.
	return nEmptyAxes >= 2;
}

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
