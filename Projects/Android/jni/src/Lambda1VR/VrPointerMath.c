#include <math.h>

#include "VrPointerMath.h"

static void rotate(const float q[4], const float v[3], float out[3])
{
	const float x = q[0], y = q[1], z = q[2], w = q[3];
	const float cx = y * v[2] - z * v[1];
	const float cy = z * v[0] - x * v[2];
	const float cz = x * v[1] - y * v[0];
	const float dx = y * cz - z * cy;
	const float dy = z * cx - x * cz;
	const float dz = x * cy - y * cx;
	out[0] = v[0] + 2.0f * (w * cx + dx);
	out[1] = v[1] + 2.0f * (w * cy + dy);
	out[2] = v[2] + 2.0f * (w * cz + dz);
}

static float dot(const float a[3], const float b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void cross(const float a[3], const float b[3], float out[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

static bool normalise(float v[3])
{
	const float length = sqrtf(dot(v, v));
	if (length < 1e-6f)
		return false;
	v[0] /= length; v[1] /= length; v[2] /= length;
	return true;
}

void VrPointer_Forward(const float pose[7], float out[3])
{
	const float forward[3] = {0.0f, 0.0f, -1.0f};
	rotate(pose + 3, forward, out);
}

bool VrPointer_RayQuad(const float origin[3], const float direction[3], const float quadPose[7],
					   float width, float height, float* u, float* v, float* t)
{
	const float xAxis[3] = {1.0f, 0.0f, 0.0f}, yAxis[3] = {0.0f, 1.0f, 0.0f}, zAxis[3] = {0.0f, 0.0f, 1.0f};
	float right[3], up[3], normal[3];
	rotate(quadPose + 3, xAxis, right);
	rotate(quadPose + 3, yAxis, up);
	rotate(quadPose + 3, zAxis, normal);

	float d[3] = {direction[0], direction[1], direction[2]};
	if (!normalise(d))
		return false;

	const float facing = dot(d, normal);
	if (facing > -1e-4f)	// away from the quad's face, or along it
		return false;

	const float toQuad[3] = {quadPose[0] - origin[0], quadPose[1] - origin[1], quadPose[2] - origin[2]};
	const float distance = dot(toQuad, normal) / facing;
	if (distance <= 0.0f)
		return false;

	const float hit[3] = {origin[0] + d[0] * distance - quadPose[0], origin[1] + d[1] * distance - quadPose[1],
						  origin[2] + d[2] * distance - quadPose[2]};
	*u = dot(hit, right) / width + 0.5f;
	*v = 0.5f - dot(hit, up) / height;
	*t = distance;
	return true;
}

static void quaternionFromAxes(const float x[3], const float y[3], const float z[3], float q[4])
{
	// the rotation whose columns are x, y, z
	const float trace = x[0] + y[1] + z[2];
	if (trace > 0.0f)
	{
		const float s = sqrtf(trace + 1.0f) * 2.0f;
		q[3] = 0.25f * s;
		q[0] = (y[2] - z[1]) / s;
		q[1] = (z[0] - x[2]) / s;
		q[2] = (x[1] - y[0]) / s;
	}
	else if (x[0] > y[1] && x[0] > z[2])
	{
		const float s = sqrtf(1.0f + x[0] - y[1] - z[2]) * 2.0f;
		q[3] = (y[2] - z[1]) / s;
		q[0] = 0.25f * s;
		q[1] = (y[0] + x[1]) / s;
		q[2] = (z[0] + x[2]) / s;
	}
	else if (y[1] > z[2])
	{
		const float s = sqrtf(1.0f + y[1] - x[0] - z[2]) * 2.0f;
		q[3] = (z[0] - x[2]) / s;
		q[0] = (y[0] + x[1]) / s;
		q[1] = 0.25f * s;
		q[2] = (z[1] + y[2]) / s;
	}
	else
	{
		const float s = sqrtf(1.0f + z[2] - x[0] - y[1]) * 2.0f;
		q[3] = (x[1] - y[0]) / s;
		q[0] = (z[0] + x[2]) / s;
		q[1] = (z[1] + y[2]) / s;
		q[2] = 0.25f * s;
	}
}

float VrPointer_BeamPose(const float a[3], const float b[3], const float eye[3], float out[7])
{
	float y[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
	const float length = sqrtf(dot(y, y));
	if (!normalise(y))
	{
		out[0] = a[0]; out[1] = a[1]; out[2] = a[2];
		out[3] = out[4] = out[5] = 0.0f; out[6] = 1.0f;
		return 0.0f;
	}

	const float mid[3] = {(a[0] + b[0]) * 0.5f, (a[1] + b[1]) * 0.5f, (a[2] + b[2]) * 0.5f};
	float z[3] = {eye[0] - mid[0], eye[1] - mid[1], eye[2] - mid[2]};
	const float along = dot(z, y);
	z[0] -= y[0] * along; z[1] -= y[1] * along; z[2] -= y[2] * along;
	if (!normalise(z))
	{
		// looking straight down the beam: any way round will do
		const float other[3] = {fabsf(y[0]) < 0.9f ? 1.0f : 0.0f, fabsf(y[0]) < 0.9f ? 0.0f : 1.0f, 0.0f};
		cross(y, other, z);
		normalise(z);
	}
	float x[3];
	cross(y, z, x);

	out[0] = mid[0]; out[1] = mid[1]; out[2] = mid[2];
	quaternionFromAxes(x, y, z, out + 3);
	return length;
}

void VrPointer_DotPose(const float p[3], const float quadPose[7], float lift, float out[7])
{
	const float zAxis[3] = {0.0f, 0.0f, 1.0f};
	float normal[3];
	rotate(quadPose + 3, zAxis, normal);
	out[0] = p[0] + normal[0] * lift;
	out[1] = p[1] + normal[1] * lift;
	out[2] = p[2] + normal[2] * lift;
	out[3] = quadPose[3]; out[4] = quadPose[4]; out[5] = quadPose[5]; out[6] = quadPose[6];
}
