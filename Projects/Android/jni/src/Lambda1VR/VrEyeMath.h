// Steam Frame: where each eye is, from what the runtime says, and the 2D shift that puts
// the HUD at a distance. Plain C with no OpenXR or engine types, so the host tests can use it.
// OpenXR axes: x right, y up, -z forward. Engine (Quake) axes: x forward, y left, z up.
#ifndef VREYEMATH_H
#define VREYEMATH_H

#include <stdbool.h>

// An eye's position in head space (OpenXR axes, metres) as an offset in the engine's
// head-aligned axes, scaled to world units.
void VrEye_ToEngineOffset(const float position[3], float worldScale, float out[3]);

// The eye's rotation relative to the head (OpenXR quaternion x y z w) as an axis in the
// engine's axes and an angle in degrees. Returns false when there's no rotation to speak of.
bool VrEye_CantAxisAngle(const float quat[4], float axis[3], float* degrees);

// Pixels from the left of an eye image to where straight ahead lands, for an asymmetric
// frustum (angles in radians as OpenXR gives them: angleLeft is negative).
float VrEye_AxisPixel(float angleLeft, float angleRight, int widthPx);

// Focal length in pixels along x.
float VrEye_FocalPixels(float angleLeft, float angleRight, int widthPx);

// How far to shift 2D content in an eye's image (positive: right) so that it sits at
// distanceM straight ahead of the head, given that eye's x position in head space.
float VrEye_HudShiftPixels(float eyeX, float angleLeft, float angleRight, int widthPx, float distanceM);

// Pixels from the left of an eye image where a point at (x, z) in head space lands
// (z negative is ahead), the eye being at eyeX. For tests and measuring.
float VrEye_ProjectX(float eyeX, float pointX, float pointZ, float angleLeft, float angleRight, int widthPx);

// pose = a pose b: b (an eye in head space) placed by a (the head in the stage). A pose is
// position x y z then quaternion x y z w.
void VrEye_ComposePose(const float a[7], const float b[7], float out[7]);

#endif
