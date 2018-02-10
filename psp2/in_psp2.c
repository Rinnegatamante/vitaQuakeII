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
cvar_t *in_joystick;
cvar_t *leftanalog_sensitivity;
cvar_t *rightanalog_sensitivity;
extern cvar_t *pstv_rumble;

uint64_t rumble_tick;

void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "1",	CVAR_ARCHIVE);
	leftanalog_sensitivity = Cvar_Get ("leftanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	rightanalog_sensitivity = Cvar_Get ("rightanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	pstv_rumble	= Cvar_Get ("pstv_rumble", "1",	CVAR_ARCHIVE);
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
	float x_mov = (left_x * cl_sidespeed->value) * 0.01;
	float y_mov = (left_y * cl_forwardspeed->value) * 0.01;
	cmd->forwardmove -= y_mov;
	cmd->sidemove += x_mov;
	
	// Right analog support for camera movement
	IN_RescaleAnalog(&right_x, &right_y, 30);
	float x_cam = (right_x * sensitivity->value) * 0.008;
	float y_cam = (right_y * sensitivity->value) * 0.008;
	cl.viewangles[YAW] -= x_cam;
	if (m_pitch->value > 0) cl.viewangles[PITCH] += y_cam;
	else cl.viewangles[PITCH] -= y_cam;

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