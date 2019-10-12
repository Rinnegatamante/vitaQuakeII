/*
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
/* r_main.c */
typedef struct
{
	unsigned		width, height;			/* coordinates from main game */
} viddef_t;

#include "gl_local.h"

void R_Clear (void);

viddef_t	vid;

refimport_t	ri;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_notexture;		   /* use for bad textures */
image_t		*r_particletexture;	/* little dot for particles */

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	/* bumped when going to a new PVS */
int			r_framecount;		/* used for dlight push checking */

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			/* final blending color */

/* forward declarations */
static void R_DrawBeam( entity_t *e );
void GL_Strings_f( void );

/*
 * view origin
 */
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

/*
 * screen size info
 */
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	/* FIXME: This is a HACK to get the client's light level */

cvar_t	*gl_nosubimage;
cvar_t	*gl_allow_software;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_palettedtexture;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t  *gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t  *gl_monolightmap;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_ztrick;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*gl_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

cvar_t  *gl_xflip;

extern int scr_width;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
    qglRotatef (-e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	/* don't even bother culling, because it's just a single
	 * polygon without a surface cache */

	psprite = (dsprite_t *)currentmodel->extradata;

	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

	/* normal sprite */
	up = vup;
	right = vright;

	if ( e->flags & RF_TRANSLUCENT )
		alpha = e->alpha;

	if ( alpha != 1.0f )
		qglEnable( GL_BLEND );

	qglColor4f( 1, 1, 1, alpha );

    GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv( GL_MODULATE );

	if ( alpha == 1.0 )
		qglEnable(GL_ALPHA_TEST);
	else
		qglDisable(GL_ALPHA_TEST);

	float *pPoint = gVertexBuffer;
	float texcoord[] = {0, 1, 0, 0, 1, 0, 1, 1};

	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, gVertexBuffer);
	gVertexBuffer += 3;

	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, gVertexBuffer);
	gVertexBuffer += 3;

	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, gVertexBuffer);
	gVertexBuffer += 3;

	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, gVertexBuffer);
	gVertexBuffer += 3;
	
	glVertexAttribPointerMapped(0, pPoint);
	glVertexAttribPointerMapped(1, texcoord);
	GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
	
	qglDisable(GL_ALPHA_TEST);
	GL_TexEnv( GL_REPLACE );

	if ( alpha != 1.0f )
		qglDisable( GL_BLEND );

	qglColor4f( 1, 1, 1, 1 );
}

/*================================================================================== */

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight);

    qglPushMatrix ();
	R_RotateForEntity (currententity);
	
	float* pPos = gVertexBuffer;
	
	*gVertexBuffer++ = 0;
	*gVertexBuffer++ = 0;
	*gVertexBuffer++ = -16;
	for (i=0 ; i<=4 ; i++){
		*gVertexBuffer++ = 16*cos(i*M_PI/2);
		*gVertexBuffer++ = 16*sin(i*M_PI/2);
		*gVertexBuffer++ = 0;
	}
	
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglColor4f(shadelight[0], shadelight[1], shadelight[2], 1);
	glVertexAttribPointerMapped(0, pPos);
	GL_DrawPolygon(GL_TRIANGLE_FAN, 6);
	
	pPos[2] = 16;
	
	glVertexAttribPointerMapped(0, pPos);
	qglColor4f(shadelight[0], shadelight[1], shadelight[2], 1);
	GL_DrawPolygon(GL_TRIANGLE_FAN, 6);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	qglColor4f (1,1,1,1);
	qglPopMatrix ();
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	/* draw non-transparent first */
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	/* solid */

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	/* draw transparent entities
	 * we could sort these if it ever becomes a problem... */
	qglDepthMask (GL_FALSE);		/* no z writes */
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	/* solid */

		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel (currententity);
				break;
			default:
				ri.Sys_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	qglDepthMask (GL_TRUE);		/* back to writing */

}

/*
** GL_DrawParticles
**
*/
void GL_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	byte			color[4];

    GL_Bind(r_particletexture->texnum);
	qglDepthMask( GL_FALSE );		/* no z buffering */
	qglEnable( GL_BLEND );
	GL_TexEnv( GL_MODULATE );

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	float* pPos = gVertexBuffer;
	float* pColor = gColorBuffer;
	float* pTex = gTexCoordBuffer;
	int num_vertices = num_particles*3;
	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		/* hack a scale up to keep particles from disapearing */
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		*(int *)color = colortable[p->color];
		
		*gColorBuffer++ = color[0]/255.0f;
		*gColorBuffer++ = color[1]/255.0f;
		*gColorBuffer++ = color[2]/255.0f;
		*gColorBuffer++ = p->alpha;
		*gColorBuffer++ = color[0]/255.0f;
		*gColorBuffer++ = color[1]/255.0f;
		*gColorBuffer++ = color[2]/255.0f;
		*gColorBuffer++ = p->alpha;
		*gColorBuffer++ = color[0]/255.0f;
		*gColorBuffer++ = color[1]/255.0f;
		*gColorBuffer++ = color[2]/255.0f;
		*gColorBuffer++ = p->alpha;
		
		*gTexCoordBuffer++ = 0.0625;
		*gTexCoordBuffer++ = 0.0625;
		*gTexCoordBuffer++ = 1.0625;
		*gTexCoordBuffer++ = 0.0625;
		*gTexCoordBuffer++ = 0.0625;
		*gTexCoordBuffer++ = 1.0625;

		*gVertexBuffer++ = p->origin[0];
		*gVertexBuffer++ = p->origin[1];
		*gVertexBuffer++ = p->origin[2];
		*gVertexBuffer++ = p->origin[0] + up[0]*scale;
		*gVertexBuffer++ = p->origin[1] + up[1]*scale;
		*gVertexBuffer++ = p->origin[2] + up[2]*scale;
		*gVertexBuffer++ = p->origin[0] + right[0]*scale;
		*gVertexBuffer++ = p->origin[1] + right[1]*scale;
		*gVertexBuffer++ = p->origin[2] + right[2]*scale;

	}

	qglEnableClientState(GL_COLOR_ARRAY);
	glVertexAttribPointerMapped(0, pPos);
	glVertexAttribPointerMapped(1, pTex);
	glVertexAttribPointerMapped(2, pColor);
	GL_DrawPolygon(GL_TRIANGLES, num_vertices);
	qglDisableClientState(GL_COLOR_ARRAY);
	
	qglDisable( GL_BLEND );
	qglColor4f( 1,1,1,1 );
	qglDepthMask( GL_TRUE );		/* back to normal Z buffering */
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table );
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
   if (!gl_polyblend->value)
      return;
   if (!v_blend[3])
      return;

   qglDisable(GL_ALPHA_TEST);
   qglEnable (GL_BLEND);
   qglDisable (GL_DEPTH_TEST);

   qglLoadIdentity ();

   /* FIXME: get rid of these */
   qglRotatef (-90,  1, 0, 0);	    /* put Z going up */
   qglRotatef (90,  0, 0, 1);	       /* put Z going up */

   float vertices[] = {
      10, 100, 100,
      10,-100, 100,
      10,-100,-100,
      10, 100,-100
   };

   qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
   qglColor4f(v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
   glVertexAttribPointerMapped(0, vertices);
   GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
   qglEnableClientState(GL_TEXTURE_COORD_ARRAY);

   qglDisable (GL_BLEND);
   qglEnable(GL_ALPHA_TEST);

   qglColor4f(1,1,1,1);
}

/*======================================================================= */

int SignbitsForPlane (cplane_t *out)
{
	int j;
	/* for fast box on planeside test */
	int bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	/* rotate VPN right by FOV_X/2 degrees */
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	/* rotate VPN left by FOV_X/2 degrees */
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	/* rotate VPN up by FOV_X/2 degrees */
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	/* rotate VPN down by FOV_X/2 degrees */
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

/*======================================================================= */

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

   /* build the transformation matrix for the given view angles */
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

   /* current viewcluster */
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		/* check above and below so crossing solid water doesn't draw wrong */
		if (!leaf->contents)
      {
         /* look down a bit */
         vec3_t	temp;

         VectorCopy (r_origin, temp);
         temp[2] -= 16;
         leaf = Mod_PointInLeaf (temp, r_worldmodel);
         if ( !(leaf->contents & CONTENTS_SOLID) &&
               (leaf->cluster != r_viewcluster2) )
            r_viewcluster2 = leaf->cluster;
      }
		else
      {
         /* look up a bit */
         vec3_t	temp;

         VectorCopy (r_origin, temp);
         temp[2] += 16;
         leaf = Mod_PointInLeaf (temp, r_worldmodel);
         if ( !(leaf->contents & CONTENTS_SOLID) &&
               (leaf->cluster != r_viewcluster2) )
            r_viewcluster2 = leaf->cluster;
      }
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	/* clear out the portion of the screen that the NOWORLDMODEL defines */
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
	{
		qglEnable( GL_SCISSOR_TEST );
		qglClearColor( 0.3, 0.3, 0.3, 1 );
		qglScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		qglClearColor( 1, 0, 0.5, 0.5 );
		qglDisable( GL_SCISSOR_TEST );
	}
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -( 2 * gl_state.camera_separation ) / zNear;
   xmax += -( 2 * gl_state.camera_separation ) / zNear;

   qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
#if 0
   float	yfov;
#endif
	int		x, x2, y2, y, w, h;

	/*
	 * set up viewport
	 */
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	qglViewport (x, y2, w, h);

	/*
	 * set up projection matrix
	 */
	screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
#if 0
	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
#endif
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  4096);

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    /* put Z going up */
    qglRotatef (90,  0, 0, 1);	    /* put Z going up */
	if (gl_xflip->value){
		qglScalef (1, -1, 1);
		qglCullFace(GL_BACK);
	}
	qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
	qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
	qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
	qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

#if 0
	if ( gl_state.camera_separation != 0 && gl_state.stereo_enabled )
		qglTranslatef ( gl_state.camera_separation, 0, 0 );
#endif

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	/*
	 * set drawing parms
	 */
	if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{	
	if (gl_clear->value)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		qglClear (GL_DEPTH_BUFFER_BIT);
	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);

	qglDepthRange (gldepthmin, gldepthmax);
	
	if (gl_shadows->value) {
		qglClearStencil(1);
		qglClear(GL_STENCIL_BUFFER_BIT);
	}
}

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Sys_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	/* done here so we know if we're in water */

	R_DrawWorld ();

	R_DrawEntitiesOnList ();

	R_RenderDlights ();

	R_DrawParticles ();

	R_DrawAlphaSurfaces ();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys, 
			c_alias_polys, 
			c_visible_textures, 
			c_visible_lightmaps); 
	}
}


void	R_SetGL2D (void)
{
	/* set 2D virtual screen size */
	qglViewport (0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable(GL_ALPHA_TEST);
	qglColor4f (1,1,1,1);
}

#if 0
static void GL_DrawColoredStereoLinePair( float r, float g, float b, float y )
{
	//->glColor3f( r, g, b );
	//->glVertex2f( 0, y );
	//->glVertex2f( vid.width, y );
	//->glColor3f( 0, 0, 0 );
	//->glVertex2f( 0, y + 1 );
	//->glVertex2f( vid.width, y + 1 );
}

static void GL_DrawStereoPattern( void )
{
	//->int i;

	//->if ( !( gl_config.renderer & GL_RENDERER_INTERGRAPH ) )
	//->	return;

	//->if ( !gl_state.stereo_enabled )
	//->	return;

	//->R_SetGL2D();

	//->qglDrawBuffer( GL_BACK_LEFT );

	//->for ( i = 0; i < 20; i++ )
	//->{
	//->	qglBegin( GL_LINES );
	//->		GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
	//->		GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
	//->		GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
	//->		GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
	//->		GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
	//->		GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
	//->		GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
	//->		GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
	//->	qglEnd();
		
	//->	GLimp_EndFrame();
	//->}
}
#endif

/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	/* save off light value for server to look at (BIG HACK!) */

	R_LightPoint (r_newrefdef.vieworg, shadelight);

	/* pick the greatest component, which should be the same
	 * as the mono value returned by software */
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
static void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetGL2D ();
}


void R_Register( void )
{
	r_lefthand = ri.Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get ("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get ("r_lightlevel", "0", 0);

	gl_nosubimage = ri.Cvar_Get( "gl_nosubimage", "0", 0 );
	gl_allow_software = ri.Cvar_Get( "gl_allow_software", "0", 0 );

	gl_particle_min_size = ri.Cvar_Get( "gl_particle_min_size", "2", CVAR_ARCHIVE );
	gl_particle_max_size = ri.Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = ri.Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = ri.Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = ri.Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = ri.Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = ri.Cvar_Get ("gl_modulate", "1", CVAR_ARCHIVE );
	gl_log = ri.Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth = ri.Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = ri.Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	
	switch (scr_width) {
		case 960:
			ri.Cvar_SetValue( "gl_mode", 3 );
			break;
		case 720:
			ri.Cvar_SetValue( "gl_mode", 2 );
			break;
		case 640:
			ri.Cvar_SetValue( "gl_mode", 1 );
			break;
		default:
			ri.Cvar_SetValue( "gl_mode", 0 );
			break;
	}
	
	gl_lightmap = ri.Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = ri.Cvar_Get ("gl_shadows", "0", CVAR_ARCHIVE );
	gl_dynamic = ri.Cvar_Get ("gl_dynamic", "1", 0);
	gl_nobind = ri.Cvar_Get ("gl_nobind", "0", 0);
	gl_round_down = ri.Cvar_Get ("gl_round_down", "0", 0);
	gl_picmip = ri.Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = ri.Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = ri.Cvar_Get ("gl_showtris", "0", 0);
	gl_ztrick = ri.Cvar_Get ("gl_ztrick", "0", 0);
	gl_finish = ri.Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = ri.Cvar_Get ("gl_clear", "0", 0);
	gl_cull = ri.Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = ri.Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = ri.Cvar_Get ("gl_flashblend", "0", 0);
	gl_playermip = ri.Cvar_Get ("gl_playermip", "0", 0);
	gl_monolightmap = ri.Cvar_Get( "gl_monolightmap", "0", 0 );
	gl_driver = ri.Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );
	gl_texturemode = ri.Cvar_Get( "gl_texturemode", "GL_LINEAR", CVAR_ARCHIVE );
	gl_texturealphamode = ri.Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = ri.Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = ri.Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_vertex_arrays = ri.Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = ri.Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_palettedtexture = ri.Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	gl_ext_multitexture = ri.Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters = ri.Cvar_Get( "gl_ext_pointparameters", "1", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = ri.Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_drawbuffer = ri.Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = ri.Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	gl_saturatelighting = ri.Cvar_Get( "gl_saturatelighting", "0", 0 );

	gl_3dlabs_broken = ri.Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );

	vid_fullscreen = ri.Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = ri.Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_ref = ri.Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );
	
	gl_xflip = ri.Cvar_Get( "gl_xflip", "0", CVAR_ARCHIVE);

	ri.Cmd_AddCommand( "imagelist", GL_ImageList_f );
	ri.Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	ri.Cmd_AddCommand( "modellist", Mod_Modellist_f );
	ri.Cmd_AddCommand( "gl_strings", GL_Strings_f );
}

/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	int err;
	qboolean fullscreen;
	
	if ( vid_fullscreen->modified && !gl_config.allow_cds )
	{
		ri.Con_Printf( PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n" );
		ri.Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ( ( err = GLimp_SetMode((int*)&vid.width, (int*)&vid.height, gl_mode->value, fullscreen ) ) == rserr_ok )
	{
		gl_state.prev_mode = gl_mode->value;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			ri.Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode((int*)&vid.width, (int*)&vid.height, gl_mode->value, false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			ri.Cvar_SetValue( "gl_mode", gl_state.prev_mode );
			gl_mode->modified = false;
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n" );
		}

		/* try setting it back to something safe */
		if ( ( err = GLimp_SetMode((int*)&vid.width, (int*)&vid.height, gl_state.prev_mode, false ) ) != rserr_ok )
		{
			ri.Con_Printf( PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}

/*
===============
R_Init
===============
*/
qboolean R_Init( void *hinstance, void *hWnd )
{	
	int		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
		r_turbsin[j] *= 0.5;

	ri.Con_Printf (PRINT_ALL, "ref_gl version: "REF_VERSION"\n");

	Draw_GetPalette ();

	ri.Con_Printf (PRINT_ALL, "Registering functions\n");
	
	R_Register();

	ri.Con_Printf (PRINT_ALL, "Initializing GLimp\n");
	
	/* initialize OS-specific parts of OpenGL */
	GLimp_Init( hinstance, hWnd );

	/* set our "safe" modes */
	gl_state.prev_mode = 3;

	/* create the window and set up the context */
	if ( !R_SetMode () )
	{
        ri.Con_Printf (PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	ri.Con_Printf (PRINT_ALL, "Vid_MenuInit\n");
	
	ri.Vid_MenuInit();

	gl_config.renderer = GL_RENDERER_OTHER;

	ri.Cvar_Set( "scr_drawall", "0" );

	gl_config.allow_cds = true;

	ri.Con_Printf (PRINT_ALL, "GL_SetDefaultState\n");
	
	GL_SetDefaultState();
	
	ri.Con_Printf (PRINT_ALL, "GL_InitImages\n");
	
	GL_InitImages ();
	
	ri.Con_Printf (PRINT_ALL, "Mod_Init\n");
	
	Mod_Init ();
	
	ri.Con_Printf (PRINT_ALL, "R_InitParticleTexture\n");
	
	R_InitParticleTexture ();
	
	ri.Con_Printf (PRINT_ALL, "Draw_InitLocal\n");
	
	Draw_InitLocal ();

   return true;
}

/*
===============
R_Shutdown
===============
*/
static void R_Shutdown (void)
{	
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();

	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();
	
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
static void R_BeginFrame( float camera_separation )
{

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified )
	{
      /* FIXME: only restart if CDS is required */
		cvar_t *ref = ri.Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	/*
	** update 3Dfx gamma -- it is expected that a user will do a vid_restart
	** after tweaking this value
	*/
	if ( vid_gamma->modified )
	{
		vid_gamma->modified = false;

		if ( gl_config.renderer & ( GL_RENDERER_VOODOO ) )
		{
			char envbuffer[1024];
			float g;

			g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
			Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
			putenv( envbuffer );
			Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
			putenv( envbuffer );
		}
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	qglViewport (0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable(GL_ALPHA_TEST);
	qglColor4f(1,1,1,1);

	/*
	** draw buffer stuff
	*/
#if 0
   if ( gl_drawbuffer->modified )
   {
      gl_drawbuffer->modified = false;

      if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
      {
         if ( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
            qglDrawBuffer( GL_FRONT );
         else
            qglDrawBuffer( GL_BACK );
      }
   }
#endif

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	/*
	 * clear screen if desired
	 */
	R_Clear ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}
	GL_SetTexturePalette( r_rawpalette );

	qglClearColor (0,0,0,0);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglClearColor (1,0, 0.5 , 0.5);
}

/*
** R_DrawBeam
*/
static void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qglEnable( GL_BLEND );
	qglDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	int num_vertices = NUM_BEAM_SEGS * 4;
	float* pPos = gVertexBuffer;
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		memcpy(gVertexBuffer, start_points[i], sizeof(vec3_t));
		gVertexBuffer+=3;
		memcpy(gVertexBuffer, end_points[i], sizeof(vec3_t));
		gVertexBuffer+=3;
		memcpy(gVertexBuffer, start_points[(i+1)%NUM_BEAM_SEGS], sizeof(vec3_t));
		gVertexBuffer+=3;
		memcpy(gVertexBuffer, end_points[(i+1)%NUM_BEAM_SEGS], sizeof(vec3_t));
		gVertexBuffer+=3;
	}

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglColor4f(r, g, b, e->alpha);
	glVertexAttribPointerMapped(0, pPos);
	GL_DrawPolygon(GL_TRIANGLE_STRIP, num_vertices);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	qglDisable( GL_BLEND );
	qglDepthMask( GL_TRUE );
}

/*=================================================================== */


void	R_BeginRegistration (char *map);
struct model_s	*R_RegisterModel (char *name);
struct image_s	*R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

struct image_s	*Draw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name, float factor);
void	Draw_Char (int x, int y, int c, float factor);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate;

	Swap_Init ();

	return re;
}


#ifndef REF_HARD_LINKED
/* this is only here so the functions in q_shared.c and q_shwin.c can link */
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ri.Sys_Error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	ri.Con_Printf (PRINT_ALL, "%s", text);
}

#endif
