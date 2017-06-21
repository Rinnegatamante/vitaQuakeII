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

void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "1",	CVAR_ARCHIVE);
	leftanalog_sensitivity = Cvar_Get ("leftanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	rightanalog_sensitivity = Cvar_Get ("rightanalog_sensitivity", "2.0", CVAR_ARCHIVE);
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

	if(abs(left_y) > 30)
	{
		float y_value = left_y - 15;
		cmd->forwardmove -= (y_value * leftanalog_sensitivity->value) * m_forward->value;
	}

	if(abs(left_x) > 30)
	{
		float x_value = left_x - 15;
		if((in_strafe.state & 1) || (lookstrafe->value))
			cmd->sidemove += (x_value * leftanalog_sensitivity->value) * m_forward->value;
		else
			cl.viewangles[YAW] -= m_side->value * x_value * 0.025f;
	}

	if(m_pitch->value < 0)
		right_y = -right_y;

	right_x = abs(right_x) < 10 ? 0 : right_x * 0.01 * rightanalog_sensitivity->value;
	right_y = abs(right_y) < 10 ? 0 : (-right_y) * 0.01 * rightanalog_sensitivity->value;

	cl.viewangles[YAW] -= right_x;
	cl.viewangles[PITCH] -= right_y;
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