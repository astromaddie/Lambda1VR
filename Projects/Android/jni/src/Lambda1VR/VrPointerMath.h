// Steam Frame: the menu laser. A ray from the weapon hand's aim pose against the flat screen
// quad, and the thin quads (the beam and its dot) that are drawn as layers of their own so nothing
// can cover them. Plain C with no OpenXR types, for the host tests. Poses are position x y z and
// quaternion x y z w (OpenXR axes: forward is -z, a quad faces +z).
#ifndef VRPOINTERMATH_H
#define VRPOINTERMATH_H

#include <stdbool.h>

// Where a ray from origin along direction (any length) meets the quad, from the quad's front. u and v
// are 0..1 across and down it (they can be outside that when the ray misses the quad itself), t is how
// far along the ray in metres of the (normalised) direction. False when the ray points away from the
// quad or along it.
bool VrPointer_RayQuad(const float origin[3], const float direction[3], const float quadPose[7],
					   float width, float height, float* u, float* v, float* t);

// The direction a pose's -z axis points in
void VrPointer_Forward(const float pose[7], float out[3]);

// The pose of a thin quad that lies along the line from a to b (its long side, the height, is
// the line's length) and turns about it to face the eye. Returns the length.
float VrPointer_BeamPose(const float a[3], const float b[3], const float eye[3], float out[7]);

// A pose at p turned like the quad (and pushed out of its face by lift, towards the viewer)
void VrPointer_DotPose(const float p[3], const float quadPose[7], float lift, float out[7]);

#endif
