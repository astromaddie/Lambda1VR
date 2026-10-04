#include <math.h>

#include "VrEyeMath.h"

void VrEye_ToEngineOffset(const float position[3], float worldScale, float out[3])
{
	out[0] = -position[2] * worldScale;	// forward
	out[1] = -position[0] * worldScale;	// left
	out[2] = position[1] * worldScale;	// up
}

bool VrEye_CantAxisAngle(const float quat[4], float axis[3], float* degrees)
{
	float x = quat[0], y = quat[1], z = quat[2], w = quat[3];
	const float length = sqrtf(x * x + y * y + z * z + w * w);
	if (length < 1e-6f)
	{
		return false;
	}
	x /= length; y /= length; z /= length; w /= length;

	// the same rotation by the shorter way round
	if (w < 0.0f)
	{
		x = -x; y = -y; z = -z; w = -w;
	}

	const float s = sqrtf(x * x + y * y + z * z);
	const float angle = 2.0f * atan2f(s, w);
	if (angle < 1e-4f || s < 1e-6f)
	{
		return false;
	}

	// the axis changes basis like a vector does (the axes differ by a proper rotation)
	axis[0] = -z / s;
	axis[1] = -x / s;
	axis[2] = y / s;
	*degrees = angle * (180.0f / (float)M_PI);
	return true;
}

float VrEye_AxisPixel(float angleLeft, float angleRight, int widthPx)
{
	const float tanL = tanf(angleLeft);
	const float tanR = tanf(angleRight);
	if (tanR - tanL <= 0.0f)
	{
		return widthPx * 0.5f;
	}
	return -tanL / (tanR - tanL) * (float)widthPx;
}

float VrEye_FocalPixels(float angleLeft, float angleRight, int widthPx)
{
	const float span = tanf(angleRight) - tanf(angleLeft);
	return span > 0.0f ? (float)widthPx / span : 0.0f;
}

float VrEye_HudShiftPixels(float eyeX, float angleLeft, float angleRight, int widthPx, float distanceM)
{
	if (distanceM <= 0.0f)
	{
		return 0.0f;
	}
	// a point straight ahead of the head lies at -eyeX from this eye's own axis
	return -eyeX / distanceM * VrEye_FocalPixels(angleLeft, angleRight, widthPx);
}

float VrEye_ProjectX(float eyeX, float pointX, float pointZ, float angleLeft, float angleRight, int widthPx)
{
	const float tanL = tanf(angleLeft);
	const float tanR = tanf(angleRight);
	const float slope = (pointX - eyeX) / -pointZ;
	return (slope - tanL) / (tanR - tanL) * (float)widthPx;
}

void VrEye_ComposePose(const float a[7], const float b[7], float out[7])
{
	const float ax = a[3], ay = a[4], az = a[5], aw = a[6];
	const float bx = b[3], by = b[4], bz = b[5], bw = b[6];

	// position: a's position plus b's rotated by a's orientation
	// v' = v + 2w(q x v) + 2(q x (q x v))
	const float vx = b[0], vy = b[1], vz = b[2];
	const float cx = ay * vz - az * vy;
	const float cy = az * vx - ax * vz;
	const float cz = ax * vy - ay * vx;
	const float dx = ay * cz - az * cy;
	const float dy = az * cx - ax * cz;
	const float dz = ax * cy - ay * cx;
	out[0] = a[0] + vx + 2.0f * (aw * cx + dx);
	out[1] = a[1] + vy + 2.0f * (aw * cy + dy);
	out[2] = a[2] + vz + 2.0f * (aw * cz + dz);

	// orientation: a * b
	out[3] = aw * bx + ax * bw + ay * bz - az * by;
	out[4] = aw * by - ax * bz + ay * bw + az * bx;
	out[5] = aw * bz + ax * by - ay * bx + az * bw;
	out[6] = aw * bw - ax * bx - ay * by - az * bz;
}
