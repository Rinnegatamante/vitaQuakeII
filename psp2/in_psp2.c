/*
Copyright (C) 2017 Felipe Izzo
Copyright (C) 1997-2001 Id Software, Inc.
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "../client/client.h"
#include <vitasdk.h>

SceCtrlData pad;
SceMotionState motionstate;

cvar_t *in_joystick;
cvar_t *leftanalog_sensitivity;
cvar_t *rightanalog_sensitivity;
cvar_t *vert_motioncam_sensitivity;
cvar_t *hor_motioncam_sensitivity;
cvar_t *use_gyro;
extern cvar_t *pstv_rumble;
extern cvar_t *gl_xflip;

uint64_t rumble_tick;

void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "1",	CVAR_ARCHIVE);
	leftanalog_sensitivity = Cvar_Get ("leftanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	rightanalog_sensitivity = Cvar_Get ("rightanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	vert_motioncam_sensitivity = Cvar_Get ("vert_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	hor_motioncam_sensitivity = Cvar_Get ("hor_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	use_gyro = Cvar_Get ("use_gyro", "0", CVAR_ARCHIVE);
	pstv_rumble	= Cvar_Get ("pstv_rumble", "1",	CVAR_ARCHIVE);

	sceMotionReset();
  sceMotionStartSampling();
}

void IN_RescaleAnalog(int *x, int *y, int dead) {

	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 128.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_Frame (void)
{
}

void IN_StartRumble (void)
{
	if (!pstv_rumble->value) return;
	SceCtrlActuator handle;
	handle.small = 100;
	handle.large = 100;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = sceKernelGetProcessTimeWide();
}

void IN_StopRumble (void)
{
	SceCtrlActuator handle;
	handle.small = 0;
	handle.large = 0;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = 0;
}

void IN_Move (usercmd_t *cmd)
{

	sceCtrlPeekBufferPositive(0, &pad, 1);
	int left_x = pad.lx - 127;
	int left_y = pad.ly - 127;
	int right_x = pad.rx - 127;
	int right_y = pad.ry - 127;

	/*if(hidKeysDown() & KEY_TOUCH)
		hidTouchRead(&old_touch);

	if((hidKeysHeld() & KEY_TOUCH) && !keyboard_toggled)
	{
		hidTouchRead(&touch);
		if(touch.px < 268)
		{
			cl.viewangles[YAW]   -= (touch.px - old_touch.px) * sensitivity->value/2;
			cl.viewangles[PITCH] += (touch.py - old_touch.py) * sensitivity->value/2;
		}
		old_touch = touch;
	}*/

	// Left analog support for player movement
	int x_mov = abs(left_x) < 30 ? 0 : (left_x * cl_sidespeed->value) * 0.01;
	int y_mov = abs(left_y) < 30 ? 0 : (left_y * cl_forwardspeed->value) * 0.01;
	cmd->forwardmove -= y_mov;
	if (gl_xflip->value) cmd->sidemove -= x_mov;
	else cmd->sidemove += x_mov;

	// Right analog support for camera movement
	IN_RescaleAnalog(&right_x, &right_y, 30);
	float x_cam = (right_x * rightanalog_sensitivity->value) * 0.008;
	float y_cam = (right_y * rightanalog_sensitivity->value) * 0.008;
	if (gl_xflip->value) cl.viewangles[YAW] += x_cam;
	else cl.viewangles[YAW] -= x_cam;

	if (m_pitch->value > 0)
		cl.viewangles[PITCH] += y_cam;
	else
		cl.viewangles[PITCH] -= y_cam;

  // gyro analog support for camera movement


  if (use_gyro->value){
    sceMotionGetState(&motionstate);

    // not sure why YAW or the horizontal x axis is the controlled by angularVelocity.y
    // and the PITCH or the vertical y axis is controlled by angularVelocity.x but its what seems to work
    float x_gyro_cam = motionstate.angularVelocity.y *  hor_motioncam_sensitivity->value; //motion_sensitivity.value;
    float y_gyro_cam = motionstate.angularVelocity.x * vert_motioncam_sensitivity->value; //motion_sensitivity.value;

    cl.viewangles[YAW] -= x_gyro_cam;

    if (g_pitch->value > 0)
      cl.viewangles[PITCH] += y_gyro_cam;
    else
      cl.viewangles[PITCH] -= y_gyro_cam;
  }

}

void IN_Activate (qboolean active)
{
}

void IN_ActivateMouse (void)
{
}

void IN_DeactivateMouse (void)
{
}

void IN_MouseEvent (int mstate)
{
}
