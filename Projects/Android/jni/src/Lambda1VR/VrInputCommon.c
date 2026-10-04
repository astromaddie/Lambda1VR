/************************************************************************************

Filename	:	VrInputRight.c 
Content		:	Handles common controller input functionality
Created		:	September 2019
Authors		:	Simon Brown

*************************************************************************************/

#include <common/common.h>
#include <common/library.h>
#include <common/cvardef.h>
#include <common/xash3d_types.h>
#include <engine/keydefs.h>
#include <client/touch.h>
#include <client/client.h>

#include "VrInput.h"

bool xash_initialised;

float playerHeight;
float playerYaw;

bool showingScreenLayer;
int vrFOV;

vec3_t worldPosition;

vec3_t hmdPosition;
vec3_t hmdorientation;
vec3_t positionDeltaThisFrame;

vec3_t weaponangles[3];
vec3_t weaponoffset;
vec3_t weaponvelocity;

vec3_t offhandangles;

vec3_t flashlightangles;
vec3_t flashlightoffset;
int ducked;

bool player_moving;
int use_gesture_state;

bool stabiliseScope;
char * g_pszBackpackWeapon;

ovrInputStateTrackedRemote leftTrackedRemoteState_old;
ovrInputStateTrackedRemote leftTrackedRemoteState_new;
ovrTrackedController leftRemoteTracking_new;

ovrInputStateTrackedRemote rightTrackedRemoteState_old;
ovrInputStateTrackedRemote rightTrackedRemoteState_new;
ovrTrackedController rightRemoteTracking_new;

float remote_movementSideways;
float remote_movementForward;
float positional_movementSideways;
float positional_movementForward;
float snapTurn;


void handleTrackedControllerButton(ovrInputStateTrackedRemote * trackedRemoteState, ovrInputStateTrackedRemote * prevTrackedRemoteState, uint32_t button, int key)
{
    if ((trackedRemoteState->Buttons & button) != (prevTrackedRemoteState->Buttons & button))
    {
        Key_Event(key, (trackedRemoteState->Buttons & button) != 0);
    }
}

static void Matrix4x4_Transform (const matrix4x4 *in, const float v[3], float out[3])
{
    out[0] = v[0] * (*in)[0][0] + v[1] * (*in)[0][1] + v[2] * (*in)[0][2] + (*in)[0][3];
    out[1] = v[0] * (*in)[1][0] + v[1] * (*in)[1][1] + v[2] * (*in)[1][2] + (*in)[1][3];
    out[2] = v[0] * (*in)[2][0] + v[1] * (*in)[2][1] + v[2] * (*in)[2][2] + (*in)[2][3];
}

float length(float x, float y)
{
    return sqrtf(powf(x, 2.0f) + powf(y, 2.0f));
}

#define NLF_DEADZONE 0.2
#define NLF_POWER 2.2

float nonLinearFilter(float in)
{
    float val = 0.0f;
    if (in > NLF_DEADZONE)
    {
        val = in;
        val -= NLF_DEADZONE;
        val /= (1.0f - NLF_DEADZONE);
        val = powf(val, NLF_POWER);
    }
    else if (in < -NLF_DEADZONE)
    {
        val = in;
        val += NLF_DEADZONE;
        val /= (1.0f - NLF_DEADZONE);
        val = -powf(fabsf(val), NLF_POWER);
    }

    return val;
}

void sendButtonActionSimple(const char* action)
{
    char command[256];
    Q_snprintf( command, sizeof( command ), "%s\n", action );
    Cbuf_AddText( command );
}

bool between(float min, float val, float max)
{
    return (min < val) && (val < max);
}

void sendButtonAction(const char* action, long buttonDown)
{
    char command[256];
    Q_snprintf( command, sizeof( command ), "%s\n", action );
    if (!buttonDown)
    {
        command[0] = '-';
    }
    Cbuf_AddText( command );

}

int IN_TouchEvent( touchEventType type, int fingerID, float x, float y, float dx, float dy );

extern float initialTouchX, initialTouchY;

void interactWithTouchScreen(ovrTrackedController *tracking, ovrInputStateTrackedRemote *newState, ovrInputStateTrackedRemote *oldState) {
    float remoteAngles[3];
    vec3_t rotation = {0, 0, 0};
    QuatToYawPitchRoll(tracking->Pose.orientation, rotation, remoteAngles);

    int newRemoteTrigState = (newState->Buttons & xrButton_Trigger) != 0;
    int prevRemoteTrigState = (oldState->Buttons & xrButton_Trigger) != 0;

    float cursorX = -sinf(DEG2RAD(remoteAngles[YAW] - playerYaw)) + 0.5f;
    float cursorY = (float)(remoteAngles[PITCH] / 90.0) + 0.5f;
#ifdef L1VR_STEAM_FRAME
    // the cursor is where the hand's ray meets the screen, the same place the laser ends
    TBXR_ScreenPointer(&cursorX, &cursorY);
#endif
    touchEventType t = event_motion;
    if (newRemoteTrigState != prevRemoteTrigState)
    {
        t = newRemoteTrigState ? event_down : event_up;
        if (newRemoteTrigState)
        {
            initialTouchX = cursorX;
            initialTouchY = cursorY;
        }
    }
    IN_TouchEvent(t, 0, cursorX, cursorY, initialTouchX - cursorX, initialTouchY - cursorY);
}


//YAW:  Left increase, Right decrease
void updateScopeAngles(float forwardYaw)
{
    //Bit of a hack, but use weapon orientation / position for view when scope is engaged
    static vec3_t currentScopeAngles;
    if (isScopeEngaged())
    {
        //Set Position
        VectorSet(hmdPosition, hmdPosition[0] + weaponoffset[0], hmdPosition[1] + weaponoffset[1], hmdPosition[2] + weaponoffset[2]);
        VectorSet(weaponoffset, 0, 0, 0);

        //Lerp the weapon angles to smooth out shaky hands a bit
        vec3_t angles;
        VectorSet(angles, -weaponangles[ADJUSTED][PITCH], weaponangles[ADJUSTED][YAW] - (forwardYaw+snapTurn), hmdorientation[ROLL]);

        if (stabiliseScope) {
            VectorLerp(currentScopeAngles, 0.125, angles, currentScopeAngles);
        } else {
            VectorCopy(angles, currentScopeAngles);
        }

        //Set "view" Angles
        VectorCopy(currentScopeAngles, hmdorientation);
    } else {
        //Scope always engages with stabilisation turned on
        stabiliseScope = true;
        VectorSet(currentScopeAngles, -weaponangles[ADJUSTED][PITCH], weaponangles[ADJUSTED][YAW] - (forwardYaw+snapTurn), hmdorientation[ROLL]);
    }
}


// Check if pTracking is within 0.4 of the HMD and in a 120 degree arc "behind" it
bool isBackpack(ovrTrackedController* pTracking)
{
    const XrVector3f position = pTracking->Pose.position;
    double distanceFromHmd = sqrtf(
            ((hmdPosition[0] - position.x) * (hmdPosition[0] - position.x)) +
            ((hmdPosition[1] - position.y) * (hmdPosition[1] - position.y)) +
            ((hmdPosition[2] - position.z) * (hmdPosition[2] - position.z))
    );

    if(distanceFromHmd > 0.4){
        return false;
    }

    double trackingAngle = atan2(hmdPosition[0] - position.x, hmdPosition[2] - position.z) * (180 / M_PI);
    double angleFromHmd = trackingAngle - playerYaw;

    if(angleFromHmd > 180){
        angleFromHmd -= 360.0f;
    }
    else if(angleFromHmd < -180){
        angleFromHmd += 360.0f;
    }

    return abs(angleFromHmd) > 120;
}