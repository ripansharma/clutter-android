/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:clutter-actor
 * @short_description: Base abstract class for all visual stage actors.
 *
 * #ClutterActor is a base abstract class for all visual elements on the
 * stage. Every object that must appear on the main #ClutterStage must also
 * be a #ClutterActor, either by using one of the classes provided by
 * Clutter, or by implementing a new #ClutterActor subclass.
 *
 * * Notes on actor transformation matrix
 *
 * The OpenGL modelview matrix for the actor is constructed from the actor
 * settings by the following order of operations:
 * <orderedlist>
 *   <listitem><para>Translation by actor x, y coords,</para></listitem>
 *   <listitem><para>Scaling by scale_x, scale_y,</para></listitem>
 *   <listitem><para>Negative translation by anchor point x, y,</para>
 *   </listitem>
 *   <listitem><para>Rotation around z axis,</para></listitem>
 *   <listitem><para>Rotation around y axis,</para></listitem>
 *   <listitem><para>Rotation around x axis,</para></listitem>
 *   <listitem><para>Translation by actor depth (z),</para></listitem>
 *   <listitem><para>Clip stencil is applied (not an operation on the matrix as
 *   such, but done as part of the transform set up).</para>
 *   </listitem>
 * </orderedlist>
 *
 * Notes on clutter actor events:
 * <orderedlist>
 *   <listitem><para>Actors emit pointer events if set reactive, see
 *   clutter_actor_set_reactive()</para></listitem>
 *   <listitem><para>The stage is always reactive</para></listitem>
 *   <listitem><para>Events are handled by connecting signal handlers to
 *   the numerous event signal types.</para></listitem>
 *   <listitem><para>Event handlers must return %TRUE if they handled
 *   the event and wish to block the event emission chain, or %FALSE
 *   if the emission chain must continue</para></listitem>
 *   <listitem><para>Keyboard events are emitted if actor has focus, see
 *   clutter_stage_set_key_focus()</para></listitem>
 *   <listitem><para>Motion events (motion, enter, leave) are not emitted
 *   if clutter_set_motion_events_enabled() is called with %FALSE.
 *   See clutter_set_motion_events_enabled() documentation for more
 *   information.</para></listitem>
 *   <listitem><para>Once emitted, an event emission chain has two
 *   phases: capture and bubble. A emitted event starts in the capture
 *   phase beginning at the stage and traversing every child actor until
 *   the event source actor is reached. The emission then enters the bubble
 *   phase, traversing back up the chain via parents until it reaches the
 *   stage. Any event handler can abort this chain by returning
 *   %TRUE (meaning "event handled").</para></listitem>
 *   <listitem><para>Pointer events will 'pass through' non reactive actors.
 *   </para></listitem>
 * </orderedlist>
 */

/**
 * CLUTTER_ACTOR_IS_MAPPED:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_MAPPED flag is set.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_REALIZED:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_REALIZED flag is set.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_VISIBLE:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the actor is both realized and mapped.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_REACTIVE:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_REACTIVE flag is set.
 *
 * Since: 0.6
 */

/**
 * CLUTTER_ACTOR_SET_FLAGS:
 * @e: a #ClutterActor
 * @f: the flags to set
 *
 * Sets flags on the given #ClutterActor
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_UNSET_FLAGS:
 * @e: a #ClutterActor
 * @f: the flags to unset
 *
 * Unsets flags on the given #ClutterActor
 *
 * Since: 0.2
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-actor.h"
#include "clutter-container.h"
#include "clutter-main.h"
#include "clutter-enum-types.h"
#include "clutter-scriptable.h"
#include "clutter-script.h"
#include "clutter-marshal.h"
#include "clutter-private.h"
#include "clutter-debug.h"
#include "clutter-units.h"
#include "cogl.h"

G_DEFINE_ABSTRACT_TYPE (ClutterActor,
                        clutter_actor,
                        G_TYPE_INITIALLY_UNOWNED);

static guint32 __id = 0;


#define CLUTTER_ACTOR_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLUTTER_TYPE_ACTOR, ClutterActorPrivate))

struct _ClutterActorPrivate
{
  ClutterActorBox coords;

  ClutterGeometry clip;                          /* FIXME: Should be Units */
  guint           has_clip : 1;
  ClutterFixed    rxang, ryang, rzang;           /* Rotation*/
  gint            rzx, rzy, rxy, rxz, ryx, ryz;	 /* FIXME: Should be Units */
  gint            z;
  guint8          opacity;
  ClutterActor   *parent_actor;
  gchar          *name;
  ClutterFixed    scale_x, scale_y;
  guint32         id; /* Unique ID */
  ClutterUnit     anchor_x, anchor_y;
};

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_DEPTH,
  PROP_CLIP,
  PROP_HAS_CLIP,
  PROP_OPACITY,
  PROP_NAME,
  PROP_VISIBLE,
  PROP_SCALE_X,
  PROP_SCALE_Y,
  PROP_REACTIVE
};

enum
{
  SHOW,
  HIDE,
  DESTROY,
  PARENT_SET,
  FOCUS_IN,
  FOCUS_OUT,

  EVENT,
  CAPTURED_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SCROLL_EVENT,
  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  MOTION_EVENT,
  ENTER_EVENT,
  LEAVE_EVENT,

  LAST_SIGNAL
};

static guint actor_signals[LAST_SIGNAL] = { 0, };

static void
_clutter_actor_apply_modelview_transform (ClutterActor * self);

static void
_clutter_actor_apply_modelview_transform_recursive (ClutterActor * self);

static gboolean
redraw_update_idle (gpointer data)
{
  ClutterMainContext *ctx = CLUTTER_CONTEXT();

  if (ctx->update_idle)
    {
      g_source_remove (ctx->update_idle);
      ctx->update_idle = 0;
    }

  clutter_redraw ();

  return FALSE;
}

static void
clutter_actor_real_show (ClutterActor *self)
{
  if (!CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      if (!CLUTTER_ACTOR_IS_REALIZED (self))
        clutter_actor_realize (self);

      /* the mapped flag on the top-level actors is set by the
       * per-backend implementation because it might be asynchronous
       */
      if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL))
        CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_MAPPED);

      if (CLUTTER_ACTOR_IS_VISIBLE (self))
        clutter_actor_queue_redraw (self);
    }
}

/**
 * clutter_actor_show
 * @self: A #ClutterActor
 *
 * Flags a clutter actor to be displayed. An actor not shown will not
 * appear on the display.
 **/
void
clutter_actor_show (ClutterActor *self)
{
  if (!CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      g_object_ref (self);

      g_signal_emit (self, actor_signals[SHOW], 0);
      g_object_notify (G_OBJECT (self), "visible");

      g_object_unref (self);
    }
}

/**
 * clutter_actor_show_all:
 * @self: a #ClutterActor
 *
 * Call show() on all children of a actor (if any).
 *
 * Since: 0.2
 */
void
clutter_actor_show_all (ClutterActor *self)
{
  ClutterActorClass *klass;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  if (klass->show_all)
    klass->show_all (self);
}

void
clutter_actor_real_hide (ClutterActor *self)
{
  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      /* see comment in clutter_actor_real_show() on why we don't set
       * the mapped flag on the top-level actors
       */
      if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL))
        CLUTTER_ACTOR_UNSET_FLAGS (self, CLUTTER_ACTOR_MAPPED);

      clutter_actor_queue_redraw (self);
    }
}

/**
 * clutter_actor_hide
 * @self: A #ClutterActor
 *
 * Flags a clutter actor to be hidden. An actor not shown will not
 * appear on the display.
 **/
void
clutter_actor_hide (ClutterActor *self)
{
  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      g_object_ref (self);

      if (CLUTTER_ACTOR_IS_REACTIVE(self))
	; 			/* FIXME: decrease global reactive count */

      g_signal_emit (self, actor_signals[HIDE], 0);
      g_object_notify (G_OBJECT (self), "visible");

      g_object_unref (self);
    }
}

/**
 * clutter_actor_hide_all:
 * @self: a #ClutterActor
 *
 * Call hide() on all child actors (if any).
 *
 * Since: 0.2
 */
void
clutter_actor_hide_all (ClutterActor *self)
{
  ClutterActorClass *klass;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  if (klass->hide_all)
    klass->hide_all (self);
}

/**
 * clutter_actor_realize
 * @self: A #ClutterActor
 *
 * Creates any underlying graphics resources needed by the actor to be
 * displayed.
 **/
void
clutter_actor_realize (ClutterActor *self)
{
  ClutterActorClass *klass;

  if (CLUTTER_ACTOR_IS_REALIZED (self))
    return;

  CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_REALIZED);

  klass = CLUTTER_ACTOR_GET_CLASS (self);

  if (klass->realize)
    (klass->realize) (self);
}

/**
 * clutter_actor_unrealize
 * @self: A #ClutterActor
 *
 * Frees up any underlying graphics resources needed by the actor to be
 * displayed.
 **/
void
clutter_actor_unrealize (ClutterActor *self)
{
  ClutterActorClass *klass;

  if (!CLUTTER_ACTOR_IS_REALIZED (self))
    return;

  CLUTTER_ACTOR_UNSET_FLAGS (self, CLUTTER_ACTOR_REALIZED);

  klass = CLUTTER_ACTOR_GET_CLASS (self);

  if (klass->unrealize)
    (klass->unrealize) (self);
}

static void
clutter_actor_real_pick (ClutterActor       *self,
			 const ClutterColor *color)
{
  if (clutter_actor_should_pick_paint (self))
    {
      cogl_color (color);
      cogl_rectangle (0,
		      0,
		      clutter_actor_get_width(self),
		      clutter_actor_get_height(self));
    }
}

/**
 * clutter_actor_pick:
 * @self: A #ClutterActor
 * @color: A #ClutterColor
 *
 * Renders a silhouette of the actor in supplied color. Used internally for
 * mapping pointer events to actors.
 *
 * This function should not never be called directly by applications.
 *
 * Subclasses overiding this method should call
 * #clutter_actor_should_pick_paint to decide if to render there
 * silhouette but in any case should still recursively call pick for
 * any children.
 *
 * Since 0.4
 **/
void
clutter_actor_pick (ClutterActor       *self,
		    const ClutterColor *color)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (color != NULL);

  CLUTTER_ACTOR_GET_CLASS (self)->pick(self, color);
}

/**
 * clutter_actor_should_pick_paint:
 * @self: A #ClutterActor
 *
 * Utility call for subclasses overiding the pick method.
 *
 * This function should not never be called directly by applications.
 *
 * Return value: TRUE if the actor should paint its silhouette, FALSE otherwise
 */
gboolean
clutter_actor_should_pick_paint (ClutterActor *self)
{
  ClutterMainContext *context;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  context = clutter_context_get_default ();

  if (CLUTTER_ACTOR_IS_MAPPED (self)
      && (G_UNLIKELY(context->pick_mode == CLUTTER_PICK_ALL)
	  || CLUTTER_ACTOR_IS_REACTIVE (self)))
    return TRUE;

  return FALSE;
}

/*
 * Utility functions for manipulating transformation matrix
 *
 * Matrix: 4x4 of ClutterFixed
 */
#define M(m,row,col)  (m)[col*4+row]

/* Transform point (x,y,z) by matrix */
static void
mtx_transform (ClutterFixed m[16],
	       ClutterFixed *x, ClutterFixed *y, ClutterFixed *z,
	       ClutterFixed *w)
{
    ClutterFixed _x, _y, _z, _w;
    _x = *x;
    _y = *y;
    _z = *z;
    _w = *w;

    /* We care lot about precission here, so have to use QMUL */
    *x = CFX_QMUL (M (m,0,0), _x) + CFX_QMUL (M (m,0,1), _y) +
	 CFX_QMUL (M (m,0,2), _z) + CFX_QMUL (M (m,0,3), _w);

    *y = CFX_QMUL (M (m,1,0), _x) + CFX_QMUL (M (m,1,1), _y) +
	 CFX_QMUL (M (m,1,2), _z) + CFX_QMUL (M (m,1,3), _w);

    *z = CFX_QMUL (M (m,2,0), _x) + CFX_QMUL (M (m,2,1), _y) +
	 CFX_QMUL (M (m,2,2), _z) + CFX_QMUL (M (m,2,3), _w);

    *w = CFX_QMUL (M (m,3,0), _x) + CFX_QMUL (M (m,3,1), _y) +
	 CFX_QMUL (M (m,3,2), _z) + CFX_QMUL (M (m,3,3), _w);

    /* Specially for Matthew: was going to put a comment here, but could not
     * think of anything at all to say ;)
     */
}

/* Applies the transforms associated with this actor and its ancestors,
 * retrieves the resulting OpenGL modelview matrix, and uses the matrix
 * to transform the supplied point
 */
static void
clutter_actor_transform_point (ClutterActor *actor,
			       ClutterUnit  *x,
			       ClutterUnit  *y,
			       ClutterUnit  *z,
			       ClutterUnit  *w)
{
  ClutterFixed           mtx[16];
  ClutterActorPrivate   *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  priv = actor->priv;

  cogl_push_matrix();
  _clutter_actor_apply_modelview_transform_recursive (actor);

  cogl_get_modelview_matrix (mtx);

  mtx_transform (mtx, x, y, z, w);

  cogl_pop_matrix();
}

/* Help macros to scale from OpenGL <-1,1> coordinates system to our
 * X-window based <0,window-size> coordinates
 */
#define MTX_GL_SCALE_X(x,w,v1,v2) (CFX_MUL( \
                                      ((CFX_DIV (x,w) + CFX_ONE) >> 1), v1) \
				         + v2)

#define MTX_GL_SCALE_Y(y,w,v1,v2) (v1 - CFX_MUL( \
                                      ((CFX_DIV (y,w) + CFX_ONE) >> 1), v1) \
				         + v2)

#define MTX_GL_SCALE_Z(z,w,v1,v2) MTX_GL_SCALE_X(z,w,v1,v2)

/**
 * clutter_actor_apply_transform_to_point:
 * @self: A #ClutterActor
 * @point: A point as #ClutterVertex
 * @vertex: The translated #ClutterVertex
 *
 * Transforms point in coordinates relative to the actor
 * into screen coordiances with the current actor tranform
 * (i.e. scale, rotation etc)
 *
 * Since: 0.4
 **/
void
clutter_actor_apply_transform_to_point (ClutterActor  *self,
					ClutterVertex *point,
					ClutterVertex *vertex)
{
  ClutterFixed  mtx_p[16];
  ClutterFixed  v[4];
  ClutterFixed  w = CFX_ONE;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  /* First we tranform the point using the OpenGL modelview matrix */
  clutter_actor_transform_point (self, &point->x, &point->y, &point->z, &w);

  cogl_get_projection_matrix (mtx_p);
  cogl_get_viewport (v);

  /* Now, transform it again with the projection matrix */
  mtx_transform (mtx_p, &point->x, &point->y, &point->z, &w);

  /* Finaly translate from OpenGL coords to window coords */
  vertex->x = MTX_GL_SCALE_X(point->x,w,v[2],v[0]);
  vertex->y = MTX_GL_SCALE_Y(point->y,w,v[3],v[1]);
  vertex->z = MTX_GL_SCALE_Z(point->z,w,v[2],v[0]);
}

/* Recursively tranform supplied vertices with the tranform for the current
 * actor and all its ancestors (like clutter_actor_transform_point() but
 * for all the vertices in one go).
 */
static void
clutter_actor_transform_vertices (ClutterActor    * self,
				  ClutterVertex     verts[4],
				  ClutterFixed      w[4])
{
  ClutterFixed           mtx[16];
  ClutterFixed           _x, _y, _z, _w;
  ClutterActorPrivate   *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  cogl_push_matrix();
  _clutter_actor_apply_modelview_transform_recursive (self);

  cogl_get_modelview_matrix (mtx);

  _x = 0;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[0].x = _x;
  verts[0].y = _y;
  verts[0].z = _z;
  w[0] = _w;

  _x = priv->coords.x2 - priv->coords.x1;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[1].x = _x;
  verts[1].y = _y;
  verts[1].z = _z;
  w[1] = _w;

  _x = 0;
  _y = priv->coords.y2 - priv->coords.y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[2].x = _x;
  verts[2].y = _y;
  verts[2].z = _z;
  w[2] = _w;

  _x = priv->coords.x2 - priv->coords.x1;
  _y = priv->coords.y2 - priv->coords.y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[3].x = _x;
  verts[3].y = _y;
  verts[3].z = _z;
  w[3] = _w;

  cogl_pop_matrix();
}

/**
 * clutter_actor_get_vertices:
 * @self: A #ClutterActor
 * @verts: Pointer to a location of an array of 4 #ClutterVertex where to
 * store the result.
 *
 * Calculates the tranformed screen coordinates of the four corners of
 * the actor; the returned vertices relate to the ClutterActorBox
 * coordinates  as follows:
 *
 *  v[0] contains (x1, y1)
 *  v[1] contains (x2, y1)
 *  v[2] contains (x1, y2)
 *  v[3] contains (x2, y2)
 *
 * Since: 0.4
 **/
void
clutter_actor_get_vertices (ClutterActor    *self,
                            ClutterVertex    verts[4])
{
  ClutterFixed           mtx_p[16];
  ClutterFixed           v[4];
  ClutterFixed           w[4];
  ClutterActorPrivate   *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  clutter_actor_transform_vertices (self, verts, w);

  cogl_get_projection_matrix (mtx_p);
  cogl_get_viewport (v);

  mtx_transform (mtx_p,
		 &verts[0].x,
		 &verts[0].y,
		 &verts[0].z,
		 &w[0]);

  verts[0].x = MTX_GL_SCALE_X (verts[0].x, w[0], v[2], v[0]);
  verts[0].y = MTX_GL_SCALE_Y (verts[0].y, w[0], v[3], v[1]);
  verts[0].z = MTX_GL_SCALE_Z (verts[0].z, w[0], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[1].x,
		 &verts[1].y,
		 &verts[1].z,
		 &w[1]);

  verts[1].x = MTX_GL_SCALE_X (verts[1].x, w[1], v[2], v[0]);
  verts[1].y = MTX_GL_SCALE_Y (verts[1].y, w[1], v[3], v[1]);
  verts[1].z = MTX_GL_SCALE_Z (verts[1].z, w[1], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[2].x,
		 &verts[2].y,
		 &verts[2].z,
		 &w[2]);

  verts[2].x = MTX_GL_SCALE_X (verts[2].x, w[2], v[2], v[0]);
  verts[2].y = MTX_GL_SCALE_Y (verts[2].y, w[2], v[3], v[1]);
  verts[2].z = MTX_GL_SCALE_Z (verts[2].z, w[2], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[3].x,
		 &verts[3].y,
		 &verts[3].z,
		 &w[3]);

  verts[3].x = MTX_GL_SCALE_X (verts[3].x, w[3], v[2], v[0]);
  verts[3].y = MTX_GL_SCALE_Y (verts[3].y, w[3], v[3], v[1]);
  verts[3].z = MTX_GL_SCALE_Z (verts[3].z, w[3], v[2], v[0]);
}

/* Applies the transforms associated with this actor to the
 * OpenGL modelview matrix.
 *
 * This function does not push/pop matrix; it is the responsibility
 * of the caller to do so as appropriate
 */
static void
_clutter_actor_apply_modelview_transform (ClutterActor * self)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActor        *parent;

  parent = clutter_actor_get_parent (self);

  if (parent != NULL)
    {
      cogl_translate (CLUTTER_UNITS_TO_INT (priv->coords.x1),
		      CLUTTER_UNITS_TO_INT (priv->coords.y1),
		      0);
    }

  /*
   * because the rotation involves translations, we must scale before
   * applying the rotations (if we apply the scale after the rotations,
   * the translations included in the rotation are not scaled and so the
   * entire object will move on the screen as a result of rotating it).
   */
  if (priv->scale_x != CFX_ONE ||
      priv->scale_y != CFX_ONE)
    {
      cogl_scale (priv->scale_x, priv->scale_y);
    }

  if (parent && (priv->anchor_x || priv->anchor_y))
    {
      cogl_translate (CLUTTER_UNITS_TO_INT (-priv->anchor_x),
		      CLUTTER_UNITS_TO_INT (-priv->anchor_y),
		      0);
    }

   if (priv->rzang)
    {
      cogl_translate (priv->rzx, priv->rzy, 0);
      cogl_rotatex (priv->rzang, 0, 0, CFX_ONE);
      cogl_translate (-priv->rzx, -priv->rzy, 0);
    }

  if (priv->ryang)
    {
      cogl_translate (priv->ryx, 0, priv->z + priv->ryz);
      cogl_rotatex (priv->ryang, 0, CFX_ONE, 0);
      cogl_translate (-priv->ryx, 0, -(priv->z + priv->ryz));
    }

  if (priv->rxang)
    {
      cogl_translate (0, priv->rxy, priv->z + priv->rxz);
      cogl_rotatex (priv->rxang, CFX_ONE, 0, 0);
      cogl_translate (0, -priv->rxy, -(priv->z + priv->rxz));
    }

  if (priv->z)
    cogl_translate (0, 0, priv->z);

  if (priv->has_clip)
    cogl_clip_set (&(priv->clip));
}

/* Recursively applies the transforms associated with this actor and
 * its ancestors to the OpenGL modelview matrix.
 *
 * This function does not push/pop matrix; it is the responsibility
 * of the caller to do so as appropriate
 */
static void
_clutter_actor_apply_modelview_transform_recursive (ClutterActor * self)
{
  ClutterActor * parent;

  parent = clutter_actor_get_parent (self);

  if (parent)
    _clutter_actor_apply_modelview_transform_recursive (parent);

  _clutter_actor_apply_modelview_transform (self);
}

/**
 * clutter_actor_paint:
 * @self: A #ClutterActor
 *
 * Renders the actor to display.
 *
 * This function should not be called directly by applications instead
 * #clutter_actor_queue_redraw should be used to queue paints.
 **/
void
clutter_actor_paint (ClutterActor *self)
{
  ClutterActorPrivate *priv;
  ClutterActorClass *klass;
  ClutterMainContext *context;
  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  priv = self->priv;

  if (!CLUTTER_ACTOR_IS_REALIZED (self))
    {
      CLUTTER_NOTE (PAINT, "Attempting realize via paint()");
      clutter_actor_realize(self);

      if (!CLUTTER_ACTOR_IS_REALIZED (self))
	{
	  CLUTTER_NOTE (PAINT, "Attempt failed, aborting paint");
	  return;
	}
    }

  context = clutter_context_get_default ();
  klass   = CLUTTER_ACTOR_GET_CLASS (self);

  cogl_push_matrix();

  _clutter_actor_apply_modelview_transform (self);

  if (G_UNLIKELY(context->pick_mode != CLUTTER_PICK_NONE))
    {
      gint         r, g, b;
      ClutterColor col;
      guint32      id;

      id = clutter_actor_get_gid (self);

      cogl_get_bitmasks (&r, &g, &b, NULL);

      /* Encode the actor id into a color, taking into account bpp */
      col.red = ((id >> (g+b)) & (0xff>>(8-r)))<<(8-r);
      col.green = ((id >> b)  & (0xff>>(8-g))) << (8-g);
      col.blue = (id & (0xff>>(8-b)))<<(8-b);
      col.alpha = 0xff;

      /* Actor will then paint silhouette of itself in supplied
       * color.  See clutter_stage_get_actor_at_pos() for where
       * picking is enabled.
       */
      clutter_actor_pick (self, &col);
    }
  else
    {
      if (G_LIKELY(klass->paint))
	(klass->paint) (self);
    }

  if (priv->has_clip)
    cogl_clip_unset();

  cogl_pop_matrix();
}

#undef M

static void
clutter_actor_real_request_coords (ClutterActor    *self,
                                   ClutterActorBox *box)
{
  self->priv->coords = *box;
}

/**
 * clutter_actor_request_coords:
 * @self: A #ClutterActor
 * @box: A #ClutterActorBox with the new coordinates, in ClutterUnits
 *
 * Requests new untransformed coordinates for the bounding box of
 * a #ClutterActor. The coordinates must be relative to the current
 * parent of the actor.
 *
 * This function should not be called directly by applications;
 * instead, the various position/geometry methods should be used.
 *
 * Note: Actors overriding the ClutterActor::request_coords() virtual
 * function should always chain up to the parent class request_coords()
 * method. Actors should override this function only if they need to
 * recompute some internal state or need to reposition their evental
 * children.
 */
void
clutter_actor_request_coords (ClutterActor    *self,
			      ClutterActorBox *box)
{
  ClutterActorPrivate *priv;
  gboolean x_change, y_change, width_change, height_change;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (box != NULL);

  priv = self->priv;

  /* avoid calling request coords if the coordinates did not change */
  x_change      = (priv->coords.x1 != box->x1);
  y_change      = (priv->coords.y1 != box->y1);
  width_change  = ((priv->coords.x2 - priv->coords.x1) != (box->x2 - box->x1));
  height_change = ((priv->coords.y2 - priv->coords.y1) != (box->y2 - box->y1));

  if (x_change || y_change || width_change || height_change)
    {
      g_object_ref (self);
      g_object_freeze_notify (G_OBJECT (self));

      CLUTTER_ACTOR_GET_CLASS (self)->request_coords (self, box);

      if (CLUTTER_ACTOR_IS_VISIBLE (self))
	clutter_actor_queue_redraw (self);

      if (x_change)
	g_object_notify (G_OBJECT (self), "x");

      if (y_change)
	g_object_notify (G_OBJECT (self), "y");

      if (width_change)
	g_object_notify (G_OBJECT (self), "width");

      if (height_change)
	g_object_notify (G_OBJECT (self), "height");

      g_object_thaw_notify (G_OBJECT (self));
      g_object_unref (self);
    }
}

/**
 * clutter_actor_query_coords:
 * @self: A #ClutterActor
 * @box: A location to store the actors #ClutterActorBox co-ordinates
 *
 * Requests the untransformed co-ordinates (in ClutterUnits) for the
 * #ClutterActor relative to any parent.
 *
 * This function should not be called directly by applications instead
 * the various position/geometry methods should be used.
 **/
void
clutter_actor_query_coords (ClutterActor    *self,
			    ClutterActorBox *box)
{
  ClutterActorClass *klass;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (box != NULL);

  klass = CLUTTER_ACTOR_GET_CLASS (self);

  box->x1 = self->priv->coords.x1;
  box->y1 = self->priv->coords.y1;
  box->x2 = self->priv->coords.x2;
  box->y2 = self->priv->coords.y2;

  if (klass->query_coords)
    {
      /* FIXME: This is kind of a cludge - we pass out *private*
       *        co-ords down to any subclasses so they can modify
       *        we then resync any changes. Needed for group class.
       *        Need to figure out nicer way.
      */
      klass->query_coords(self, box);

      self->priv->coords.x1 = box->x1;
      self->priv->coords.y1 = box->y1;
      self->priv->coords.x2 = box->x2;
      self->priv->coords.y2 = box->y2;
    }
}

static void
clutter_actor_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{

  ClutterActor        *actor;
  ClutterActorPrivate *priv;

  actor = CLUTTER_ACTOR(object);
  priv = actor->priv;

  switch (prop_id)
    {
    case PROP_X:
      clutter_actor_set_x (actor, g_value_get_int (value));
      break;
    case PROP_Y:
      clutter_actor_set_y (actor, g_value_get_int (value));
      break;
    case PROP_WIDTH:
      clutter_actor_set_width (actor, g_value_get_int (value));
      break;
    case PROP_HEIGHT:
      clutter_actor_set_height (actor, g_value_get_int (value));
      break;
    case PROP_DEPTH:
      clutter_actor_set_depth (actor, g_value_get_int (value));
      break;
    case PROP_OPACITY:
      clutter_actor_set_opacity (actor, g_value_get_uchar (value));
      break;
    case PROP_NAME:
      clutter_actor_set_name (actor, g_value_get_string (value));
      break;
    case PROP_VISIBLE:
      if (g_value_get_boolean (value) == TRUE)
	clutter_actor_show (actor);
      else
	clutter_actor_hide (actor);
      break;
    case PROP_SCALE_X:
      clutter_actor_set_scalex
                         (actor,
			  CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value)),
			  priv->scale_y);
      break;
    case PROP_SCALE_Y:
      clutter_actor_set_scalex
                         (actor,
			  priv->scale_x,
			  CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value)));
      break;
    case PROP_CLIP:
      {
        ClutterGeometry *geom = g_value_get_boxed (value);

	clutter_actor_set_clip (actor,
				geom->x, geom->y,
				geom->width, geom->height);
      }
      break;
    case PROP_REACTIVE:
      clutter_actor_set_reactive (actor, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  ClutterActor        *actor;
  ClutterActorPrivate *priv;

  actor = CLUTTER_ACTOR(object);
  priv = actor->priv;

  switch (prop_id)
    {
    case PROP_X:
      g_value_set_int (value, clutter_actor_get_x (actor));
      break;
    case PROP_Y:
      g_value_set_int (value, clutter_actor_get_y (actor));
      break;
    case PROP_WIDTH:
      g_value_set_int (value, clutter_actor_get_width (actor));
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, clutter_actor_get_height (actor));
      break;
    case PROP_DEPTH:
      g_value_set_int (value, clutter_actor_get_depth (actor));
      break;
    case PROP_OPACITY:
      g_value_set_uchar (value, priv->opacity);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value,
		           (CLUTTER_ACTOR_IS_VISIBLE (actor) != FALSE));
      break;
    case PROP_HAS_CLIP:
      g_value_set_boolean (value, priv->has_clip);
      break;
    case PROP_CLIP:
      g_value_set_boxed (value, &(priv->clip));
      break;
    case PROP_SCALE_X:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->scale_x));
      break;
    case PROP_SCALE_Y:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->scale_y));
      break;
    case PROP_REACTIVE:
      g_value_set_boolean (value,
                           (CLUTTER_ACTOR_IS_REACTIVE (actor) != FALSE));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_dispose (GObject *object)
{
  ClutterActor *self = CLUTTER_ACTOR (object);

  CLUTTER_NOTE (MISC, "Disposing of object (id=%d) of type `%s' (ref_count:%d)",
		self->priv->id,
		g_type_name (G_OBJECT_TYPE (self)),
                object->ref_count);

 if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_DESTRUCTION))
    {
      CLUTTER_SET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_DESTRUCTION);

      g_signal_emit (self, actor_signals[DESTROY], 0);

      CLUTTER_UNSET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_DESTRUCTION);
    }

  G_OBJECT_CLASS (clutter_actor_parent_class)->dispose (object);
}

static void
clutter_actor_finalize (GObject *object)
{
  ClutterActor *actor = CLUTTER_ACTOR (object);

  CLUTTER_NOTE (MISC, "Finalize object (id=%d) of type `%s'",
		actor->priv->id,
		g_type_name (G_OBJECT_TYPE (actor)));

  g_free (actor->priv->name);

  G_OBJECT_CLASS (clutter_actor_parent_class)->finalize (object);
}

static void
clutter_actor_class_init (ClutterActorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clutter_actor_set_property;
  object_class->get_property = clutter_actor_get_property;
  object_class->dispose      = clutter_actor_dispose;
  object_class->finalize     = clutter_actor_finalize;

  g_type_class_add_private (klass, sizeof (ClutterActorPrivate));

  /**
   * ClutterActor:x:
   *
   * X coordinate of the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_X,
                                   g_param_spec_int ("x",
                                                     "X co-ord",
                                                     "X co-ord of actor",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:y:
   *
   * Y coordinate of the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y co-ord",
                                                     "Y co-ord of actor",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:width:
   *
   * Width of the actor (in pixels).
   */
  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                     "Width",
                                                     "Width of actor in pixels",
                                                     0, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:height:
   *
   * Height of the actor (in pixels).
   */
  g_object_class_install_property (object_class,
                                   PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                     "Height",
                                                     "Height of actor in pixels",
                                                     0, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:depth:
   *
   * Depth of the actor.
   *
   * Since: 0.6
   */
  g_object_class_install_property (object_class,
                                   PROP_DEPTH,
                                   g_param_spec_int ("depth",
                                                     "Depth",
                                                     "Depth of actor",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:opacity:
   *
   * Opacity of the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_OPACITY,
                                   g_param_spec_uchar ("opacity",
                                                       "Opacity",
                                                       "Opacity of actor",
                                                       0, 0xff,
                                                       0xff,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:visible:
   *
   * Whether the actor is visible or not.
   */
  g_object_class_install_property (object_class,
                                   PROP_VISIBLE,
                                   g_param_spec_boolean ("visible",
                                                         "Visible",
                                                         "Whether the actor is visible or not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:reactive:
   *
   * Whether the actor is reactive to events or not.
   *
   * Since: 0.6
   */
  g_object_class_install_property (object_class,
                                   PROP_REACTIVE,
                                   g_param_spec_boolean ("reactive",
                                                         "Reactive",
                                                         "Whether the actor is reactive to events or not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:has-clip:
   *
   * Whether the actor has the clip property set or not.
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_CLIP,
                                   g_param_spec_boolean ("has-clip",
                                                         "Has Clip",
                                                         "Whether the actor has a clip set or not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READABLE));
  /**
   * ClutterActor:clip:
   *
   * The clip region for the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_CLIP,
                                   g_param_spec_boxed ("clip",
                                                       "Clip",
                                                       "The clip region for the actor",
                                                       CLUTTER_TYPE_GEOMETRY,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:name:
   *
   * The name of the actor.
   *
   * Since: 0.2
   */
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name of the actor",
                                                        NULL,
                                                        CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor::scale-x:
   *
   * The horizontal scale of the actor
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_SCALE_X,
			g_param_spec_double ("scale-x",
					     "Scale-X",
					     "Scale X",
					     0.0,
					     G_MAXDOUBLE,
					     1.0,
					     CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor::scale-y:
   *
   * The vertical scale of the actor
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_SCALE_Y,
			g_param_spec_double ("scale-y",
					     "Scale-Y",
					     "Scale Y",
					     0.0,
					     G_MAXDOUBLE,
					     1.0,
					     CLUTTER_PARAM_READWRITE));


  /**
   * ClutterActor::destroy:
   * @actor: the object which received the signal
   *
   * The ::destroy signal is emitted when an actor is destroyed,
   * either by direct invocation of clutter_actor_destroy() or
   * when the #ClutterGroup that contains the actor is destroyed.
   *
   * Since: 0.2
   */
  actor_signals[DESTROY] =
    g_signal_new ("destroy",
		  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (ClutterActorClass, destroy),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::show:
   * @actor: the object which received the signal
   *
   * The ::show signal is emitted when an actor becomes visible.
   *
   * Since: 0.2
   */
  actor_signals[SHOW] =
    g_signal_new ("show",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (ClutterActorClass, show),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::hide:
   * @actor: the object which received the signal
   *
   * The ::hide signal is emitted when an actor is no longer visible.
   *
   * Since: 0.2
   */
  actor_signals[HIDE] =
    g_signal_new ("hide",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (ClutterActorClass, hide),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::parent-set:
   * @actor: the object which received the signal
   * @old_parent: the previous parent of the actor, or %NULL
   *
   * This signal is emitted when the parent of the actor changes.
   *
   * Since: 0.2
   */
  actor_signals[PARENT_SET] =
    g_signal_new ("parent-set",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterActorClass, parent_set),
                  NULL, NULL,
                  clutter_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  CLUTTER_TYPE_ACTOR);

  /**
   * ClutterActor::event:
   * @actor: the actor which received the event
   * @event: a #ClutterEvent
   *
   * The ::event signal is emitted each time and event is received
   * by the @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[EVENT] =
    g_signal_new ("event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::button-press-event:
   * @actor: the actor which received the event
   * @event: a #ClutterButtonEvent
   *
   * The ::button-press-event signal is emitted each time a mouse button
   * is pressed on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, button_press_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::button-release-event:
   * @actor: the actor which received the event
   * @event: a #ClutterButtonEvent
   *
   * The ::button-release-event signal is emitted each time a mouse button
   * is released on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, button_release_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::scroll-event:
   * @actor: the actor which received the event
   * @event: a #ClutterScrollEvent
   *
   * The ::scroll-event signal is emitted each time a the mouse is
   * scrolled on @actor
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[SCROLL_EVENT] =
    g_signal_new ("scroll-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, scroll_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::key-press-event:
   * @actor: the actor which received the event
   * @event: a #ClutterKeyEvent
   *
   * The ::key-press-event signal is emitted each time a keyboard button
   * is pressed on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[KEY_PRESS_EVENT] =
    g_signal_new ("key-press-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, key_press_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::key-release-event:
   * @actor: the actor which received the event
   * @event: a #ClutterKeyEvent
   *
   * The ::key-release-event signal is emitted each time a keyboard button
   * is released on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[KEY_RELEASE_EVENT] =
    g_signal_new ("key-release-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, key_release_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::motion-event:
   * @actor: the actor which received the event
   * @event: a #ClutterMotionEvent
   *
   * The ::motion-event signal is emitted each time the mouse pointer is
   * moved on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[MOTION_EVENT] =
    g_signal_new ("motion-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, motion_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::focus-in:
   * @actor: the actor which now has key focus
   *
   * The ::focus-in signal is emitted when @actor recieves key focus.
   *
   * Since: 0.6
   */
  actor_signals[FOCUS_IN] =
    g_signal_new ("focus-in",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, focus_in),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * ClutterActor::focus-out:
   * @actor: the actor which now has key focus
   *
   * The ::focus-out signal is emitted when @actor loses key focus.
   *
   * Since: 0.6
   */
  actor_signals[FOCUS_OUT] =
    g_signal_new ("focus-out",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, focus_out),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * ClutterActor::enter-event:
   * @actor: the actor which the pointer has entered.
   *
   * The ::enter-event signal is emitted when the pointer enters the @actor
   *
   * Since: 0.6
   */
  actor_signals[ENTER_EVENT] =
    g_signal_new ("enter-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, enter_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::leave-event:
   * @actor: the actor which the pointer has left
   *
   * The ::leave-event signal is emitted when the pointer leaves the @actor.
   *
   * Since: 0.6
   */
  actor_signals[LEAVE_EVENT] =
    g_signal_new ("leave-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, leave_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::captured-event:
   * @actor: the actor which the pointer has left
   *
   * The ::captured-event signal is emitted when an event is captured
   * by Clutter.
   *
   * Since: 0.6
   */
  actor_signals[CAPTURED_EVENT] =
    g_signal_new ("captured-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, captured_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  klass->show = clutter_actor_real_show;
  klass->show_all = clutter_actor_show;
  klass->hide = clutter_actor_real_hide;
  klass->hide_all = clutter_actor_hide;
  klass->pick = clutter_actor_real_pick;
  klass->request_coords = clutter_actor_real_request_coords;
}

static void
clutter_actor_init (ClutterActor *self)
{
  ClutterActorPrivate *priv;
  ClutterActorBox box = { 0, };

  self->priv = priv = CLUTTER_ACTOR_GET_PRIVATE (self);

  priv->parent_actor = NULL;
  priv->has_clip     = FALSE;
  priv->opacity      = 0xff;
  priv->id           = __id++;
  priv->scale_x      = CFX_ONE;
  priv->scale_y      = CFX_ONE;

  clutter_actor_request_coords (self, &box);
}

/**
 * clutter_actor_destroy:
 * @self: a #ClutterActor
 *
 * Destroys an actor.  When an actor is destroyed, it will break any
 * references it holds to other objects.  If the actor is inside a
 * container, the actor will be removed.
 *
 * When you destroy a container its children will be destroyed as well.
 *
 * Note: you cannot destroy the #ClutterStage returned by
 * clutter_stage_get_default().
 */
void
clutter_actor_destroy (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
    {
      g_warning ("Calling clutter_actor_destroy() on an actor of type `%s' "
                 "is not possible. This is usually an application bug.",
                 g_type_name (G_OBJECT_TYPE (self)));
      return;
    }

  priv = self->priv;

  if (priv->parent_actor)
    {
      ClutterActor *parent = priv->parent_actor;

      if (CLUTTER_IS_CONTAINER (parent))
        {
          g_object_ref (self);
          clutter_container_remove_actor (CLUTTER_CONTAINER (parent), self);
        }
      else
        priv->parent_actor = NULL;
    }

  if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_DESTRUCTION))
    g_object_run_dispose (G_OBJECT (self));

  g_object_unref (self);
}

/**
 * clutter_actor_queue_redraw:
 * @self: A #ClutterActor
 *
 * Queues up a redraw of an actor and any children. The redraw occurs
 * once the main loop becomes idle (after the current batch of events
 * has been processed, roughly).
 *
 * Applications rarely need to call this as redraws are handled automatically
 * by modification functions.
 */
void
clutter_actor_queue_redraw (ClutterActor *self)
{
  ClutterMainContext *ctx = CLUTTER_CONTEXT();

  if (!ctx->update_idle)
    {
      CLUTTER_TIMESTAMP (SCHEDULER, "Adding idle source for actor: %p", self);

      ctx->update_idle =
        clutter_threads_add_idle_full (G_PRIORITY_DEFAULT + 10,
                                       redraw_update_idle,
                                       NULL, NULL);
    }
}

/**
 * clutter_actor_set_geometry:
 * @self: A #ClutterActor
 * @geometry: A #ClutterGeometry
 *
 * Sets the actors untransformed geometry in pixels relative to any
 * parent actor.
 */
void
clutter_actor_set_geometry (ClutterActor          *self,
			    const ClutterGeometry *geometry)
{
  ClutterActorBox box;

  box.x1 = CLUTTER_UNITS_FROM_INT (geometry->x);
  box.y1 = CLUTTER_UNITS_FROM_INT (geometry->y);
  box.x2 = CLUTTER_UNITS_FROM_INT (geometry->x + geometry->width);
  box.y2 = CLUTTER_UNITS_FROM_INT (geometry->y + geometry->height);

  clutter_actor_request_coords (self, &box);
}

/**
 * clutter_actor_get_geometry:
 * @self: A #ClutterActor
 * @geometry: A location to store actors #ClutterGeometry
 *
 * Gets the actors untransformed geometry in pixels relative to any
 * parent actor.
 */
void
clutter_actor_get_geometry (ClutterActor    *self,
			    ClutterGeometry *geometry)
{
  ClutterActorBox box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  geometry->x      = CLUTTER_UNITS_TO_INT (box.x1);
  geometry->y      = CLUTTER_UNITS_TO_INT (box.y1);
  geometry->width  = CLUTTER_UNITS_TO_INT (box.x2 - box.x1);
  geometry->height = CLUTTER_UNITS_TO_INT (box.y2 - box.y1);
}

/**
 * clutter_actor_get_coords:
 * @self: A #ClutterActor
 * @x_1: A location to store actors left position, or %NULL.
 * @y_1: A location to store actors top position, or %NULL.
 * @x_2: A location to store actors right position, or %NULL.
 * @y_2: A location to store actors bottom position, or %NULL.
 *
 * Gets the actors untransformed bounding rectangle co-ordinates in pixels
 * relative to any parent actor.
 */
void
clutter_actor_get_coords (ClutterActor *self,
			  gint         *x_1,
			  gint         *y_1,
			  gint         *x_2,
			  gint         *y_2)
{
  ClutterActorBox box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  if (x_1)
    *x_1 = CLUTTER_UNITS_TO_INT (box.x1);

  if (y_1)
    *y_1 = CLUTTER_UNITS_TO_INT (box.y1);

  if (x_2)
    *x_2 = CLUTTER_UNITS_TO_INT (box.x2);

  if (y_2)
    *y_2 = CLUTTER_UNITS_TO_INT (box.y2);
}

/**
 * clutter_actor_set_position
 * @self: A #ClutterActor
 * @x: New left position of actor in pixels.
 * @y: New top position of actor in pixels.
 *
 * Sets the actors position in pixels relative to any
 * parent actor.
 */
void
clutter_actor_set_position (ClutterActor *self,
			    gint          x,
			    gint          y)
{
  ClutterActorBox box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  box.x2 += (CLUTTER_UNITS_FROM_INT (x) - box.x1);
  box.y2 += (CLUTTER_UNITS_FROM_INT (y) - box.y1);

  box.x1 = CLUTTER_UNITS_FROM_INT (x);
  box.y1 = CLUTTER_UNITS_FROM_INT (y);

  clutter_actor_request_coords (self, &box);
}

/**
 * clutter_actor_move_by
 * @self: A #ClutterActor
 * @dx: Distance to move Actor on X axis.
 * @dy: Distance to move Actor on Y axis.
 *
 * Moves an actor by specified distance relative to
 * current position in pixels.
 *
 * Since: 0.2
 */
void
clutter_actor_move_by (ClutterActor *self,
		       gint          dx,
		       gint          dy)
{
  ClutterActorBox box;
  gint32 dxu = CLUTTER_UNITS_FROM_INT (dx);
  gint32 dyu = CLUTTER_UNITS_FROM_INT (dy);

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  box.x2 += dxu;
  box.y2 += dyu;
  box.x1 += dxu;
  box.y1 += dyu;

  clutter_actor_request_coords (self, &box);
}

/* local inline version, without type checking to be used by
 * set_width() and set_height(). if one of the dimensions is < 0
 * it will not be changed
 */
static inline void
clutter_actor_set_size_internal (ClutterActor *self,
                                 gint          width,
                                 gint          height)
{
  ClutterActorBox box;

  clutter_actor_query_coords (self, &box);

  if (width > 0)
    box.x2 = box.x1 + CLUTTER_UNITS_FROM_INT (width);

  if (height > 0)
    box.y2 = box.y1 + CLUTTER_UNITS_FROM_INT (height);

  clutter_actor_request_coords (self, &box);
}

/**
 * clutter_actor_set_size
 * @self: A #ClutterActor
 * @width: New width of actor in pixels, or -1
 * @height: New height of actor in pixels, or -1
 *
 * Sets the actors size in pixels. If @width and/or @height are -1 the
 * actor will assume the same size of its bounding box.
 */
void
clutter_actor_set_size (ClutterActor *self,
			gint          width,
			gint          height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_size_internal (self, width, height);
}

/**
 * clutter_actor_get_size:
 * @self: A #ClutterActor
 * @width: Location to store width if non NULL.
 * @height: Location to store height if non NULL.
 *
 * Gets the size of an actor in pixels ignoring any scaling factors.
 *
 * Since: 0.2
 */
void
clutter_actor_get_size (ClutterActor *self,
			guint        *width,
			guint        *height)
{
  ClutterActorBox box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  if (width)
    *width = CLUTTER_UNITS_TO_INT (box.x2 - box.x1);

  if (height)
    *height = CLUTTER_UNITS_TO_INT (box.y2 - box.y1);
}

/**
 * clutter_actor_get_position:
 * @self: a #ClutterActor
 * @x: return location for the X coordinate, or %NULL
 * @y: return location for the Y coordinate, or %NULL
 *
 * Retrieves the position of an actor.
 *
 * Since: 0.6
 */
void
clutter_actor_get_position (ClutterActor *self,
                            gint         *x,
                            gint         *y)
{
  ClutterActorBox box = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_query_coords (self, &box);

  if (x)
    *x = CLUTTER_UNITS_TO_INT (box.x1);

  if (y)
    *y = CLUTTER_UNITS_TO_INT (box.y1);
}

/*
 * clutter_actor_get_abs_position_units
 * @self: A #ClutterActor
 * @x: Location to store x position if non NULL.
 * @y: Location to store y position if non NULL.
 *
 * Gets the absolute position of an actor in clutter units relative
 * to the stage.
 *
 * Since: 0.4
 */
static void
clutter_actor_get_abs_position_units (ClutterActor *self,
				      gint32       *x,
				      gint32       *y)
{
  ClutterVertex v1;
  ClutterVertex v2;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  v1.x = v1.y = v1.z = 0;
  clutter_actor_apply_transform_to_point (self, &v1, &v2);

  if (x)
    *x = v2.x;
  if (y)
    *y = v2.y;
}

/**
 * clutter_actor_get_abs_position
 * @self: A #ClutterActor
 * @x: Location to store x position if non NULL.
 * @y: Location to store y position if non NULL.
 *
 * Gets the absolute position of an actor in pixels relative
 * to the stage.
 */
void
clutter_actor_get_abs_position (ClutterActor *self,
				gint         *x,
				gint         *y)
{
  ClutterUnit xu, yu;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  xu = yu = 0;
  clutter_actor_get_abs_position_units (self, &xu, &yu);

  if (x)
    *x = CLUTTER_UNITS_TO_INT (xu);
  if (y)
    *y = CLUTTER_UNITS_TO_INT (yu);
}

/*
 * clutter_actor_get_abs_size_units:
 * @self: A #ClutterActor
 * @width: Location to store width if non NULL.
 * @height: Location to store height if non NULL.
 *
 * Gets the absolute size of an actor in clutter units taking into account
 * an scaling factors.
 *
 * Note: When the actor (or one of its ancestors) is rotated around the x or y
 * axis, it no longer appears as on the stage as a rectangle, but as a generic
 * quadrangle; in that case this function returns the size of the smallest
 * rectangle that encapsulates the entire quad. Please note that in this case
 * no assumptions can be made about the relative position of this envelope to
 * the absolute position of the actor, as returned by
 * clutter_actor_get_abs_position() - if you need this information, you need
 * to use clutter_actor_get_vertices() to get the coords of the actual
 * quadrangle.
 *
 * Since: 0.4
 */
static void
clutter_actor_get_abs_size_units (ClutterActor *self,
				  gint32       *width,
				  gint32       *height)
{
  ClutterVertex v[4];
  ClutterFixed  x_min, x_max, y_min, y_max;
  gint i;

  clutter_actor_get_vertices (self, v);

  x_min = x_max = v[0].x;
  y_min = y_max = v[0].y;

  for (i = 1; i < sizeof(v)/sizeof(v[0]); ++i)
    {
      if (v[i].x < x_min)
	x_min = v[i].x;

      if (v[i].x > x_max)
	x_max = v[i].x;

      if (v[i].y < y_min)
	y_min = v[i].y;

      if (v[i].y > y_max)
	y_max = v[i].y;
    }

  *width  = x_max - x_min;
  *height = y_max - y_min;
}

/**
 * clutter_actor_get_abs_size:
 * @self: A #ClutterActor
 * @width: Location to store width if non NULL.
 * @height: Location to store height if non NULL.
 *
 * Gets the absolute size of an actor taking into account
 * an scaling factors
 */
void
clutter_actor_get_abs_size (ClutterActor *self,
			    guint        *width,
			    guint        *height)
{
  gint32 wu, hu;
  clutter_actor_get_abs_size_units (self, &wu, &hu);

  *width  = CLUTTER_UNITS_TO_INT (wu);
  *height = CLUTTER_UNITS_TO_INT (hu);
}


/**
 * clutter_actor_get_width
 * @self: A #ClutterActor
 *
 * Retrieves the actors width ignoring any scaling factors.
 *
 * Return value: The actor width in pixels
 **/
guint
clutter_actor_get_width (ClutterActor *self)
{
  ClutterActorBox box;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  clutter_actor_query_coords (self, &box);

  return CLUTTER_UNITS_TO_INT (box.x2 - box.x1);
}

/**
 * clutter_actor_get_height
 * @self: A #ClutterActor
 *
 * Retrieves the actors height ignoring any scaling factors.
 *
 * Return value: The actor height in pixels
 **/
guint
clutter_actor_get_height (ClutterActor *self)
{
  ClutterActorBox box;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  clutter_actor_query_coords (self, &box);

  return CLUTTER_UNITS_TO_INT (box.y2 - box.y1);
}

/**
 * clutter_actor_set_width
 * @self: A #ClutterActor
 * @width: Requested new width for actor
 *
 * Requests a new width for actor
 *
 * since: 2.0
 **/
void
clutter_actor_set_width (ClutterActor *self,
                         guint         width)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_size_internal (self, width, -1);
}

/**
 * clutter_actor_set_height
 * @self: A #ClutterActor
 * @height: Requested new height for actor
 *
 * Requests a new height for actor
 *
 * since: 2.0
 **/
void
clutter_actor_set_height (ClutterActor *self,
                          guint         height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_size_internal (self, -1, height);
}

/**
 * clutter_actor_set_x:
 * @self: a #ClutterActor
 * @x: the actors position on the X axis
 *
 * Sets the actor's x position relative to its parent.
 *
 * Since: 0.6
 */
void
clutter_actor_set_x (ClutterActor *self,
                     gint          x)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_position (self,
                              x,
                              clutter_actor_get_y (self));
}

/**
 * clutter_actor_set_y:
 * @self: a #ClutterActor
 * @y: the actors position on the Y axis
 *
 * Sets the actor's y position relative to its parent.
 *
 * Since: 0.6
 */
void
clutter_actor_set_y (ClutterActor *self,
                     gint          y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_position (self,
                              clutter_actor_get_x (self),
                              y);
}

/**
 * clutter_actor_get_x
 * @self: A #ClutterActor
 *
 * Retrieves the actors x position relative to any parent.
 *
 * Return value: The actor x position in pixels ignoring any tranforms
 * (i.e scaling, rotation).
 **/
gint
clutter_actor_get_x (ClutterActor *self)
{
  ClutterActorBox box;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  clutter_actor_query_coords (self, &box);

  return CLUTTER_UNITS_TO_INT (box.x1);
}

/**
 * clutter_actor_get_y:
 * @self: A #ClutterActor
 *
 * Retrieves the actors y position relative to any parent.
 *
 * Return value: The actor y position in pixels ignoring any tranforms
 * (i.e scaling, rotation).
 **/
gint
clutter_actor_get_y (ClutterActor *self)
{
  ClutterActorBox box;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  clutter_actor_query_coords (self, &box);

  return CLUTTER_UNITS_TO_INT (box.y1);
}

/**
 * clutter_actor_set_scalex:
 * @self: A #ClutterActor
 * @scale_x: #ClutterFixed factor to scale actor by horizontally.
 * @scale_y: #ClutterFixed factor to scale actor by vertically.
 *
 * Scales an actor with fixed point parameters.
 */
void
clutter_actor_set_scalex (ClutterActor *self,
			  ClutterFixed  scale_x,
			  ClutterFixed  scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_ref (self);
  g_object_freeze_notify (G_OBJECT (self));

  self->priv->scale_x = scale_x;
  g_object_notify (G_OBJECT (self), "scale-x");

  self->priv->scale_y = scale_y;
  g_object_notify (G_OBJECT (self), "scale-y");

  g_object_thaw_notify (G_OBJECT (self));
  g_object_unref (self);

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_set_scale:
 * @self: A #ClutterActor
 * @scale_x: double factor to scale actor by horizontally.
 * @scale_y: double factor to scale actor by vertically.
 *
 * Scales an actor with floating point parameters.
 *
 * Since: 0.2
 */
void
clutter_actor_set_scale (ClutterActor *self,
			 double        scale_x,
			 double        scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_scalex (self,
			    CLUTTER_FLOAT_TO_FIXED (scale_x),
			    CLUTTER_FLOAT_TO_FIXED (scale_y));
}

/**
 * clutter_actor_get_scalex:
 * @self: A #ClutterActor
 * @scale_x: Location to store horizonal fixed scale factor if non NULL.
 * @scale_y: Location to store vertical fixed scale factor if non NULL.
 *
 * Retrieves an actors scale in fixed point.
 *
 * Since: 0.2
 */
void
clutter_actor_get_scalex (ClutterActor *self,
			  ClutterFixed *scale_x,
			  ClutterFixed *scale_y)
{
  if (scale_x)
    *scale_x = self->priv->scale_x;

  if (scale_y)
    *scale_y = self->priv->scale_y;
}

/**
 * clutter_actor_get_scale:
 * @self: A #ClutterActor
 * @scale_x: Location to store horizonal float scale factor if non NULL.
 * @scale_y: Location to store vertical float scale factor if non NULL.
 *
 * Retrieves an actors scale in floating point.
 *
 * Since: 0.2
 */
void
clutter_actor_get_scale (ClutterActor *self,
			 gdouble      *scale_x,
			 gdouble      *scale_y)
{
  if (scale_x)
    *scale_x = CLUTTER_FIXED_TO_FLOAT (self->priv->scale_x);

  if (scale_y)
    *scale_y = CLUTTER_FIXED_TO_FLOAT (self->priv->scale_y);
}

/**
 * clutter_actor_set_opacity:
 * @self: A #ClutterActor
 * @opacity: New opacity value for actor.
 *
 * Sets the actors opacity, with zero being completely transparent.
 */
void
clutter_actor_set_opacity (ClutterActor *self,
			   guint8        opacity)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  self->priv->opacity = opacity;

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_get_opacity:
 * @self: A #ClutterActor
 *
 * Retrieves the actors opacity.
 *
 * Return value: The actor opacity value.
 */
guint8
clutter_actor_get_opacity (ClutterActor *self)
{
  ClutterActor *parent;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  parent = self->priv->parent_actor;

  /* Factor in the actual actors opacity with parents */
  if (parent && clutter_actor_get_opacity (parent) != 0xff)
      return (clutter_actor_get_opacity(parent) * self->priv->opacity) / 0xff;

  return self->priv->opacity;
}

/**
 * clutter_actor_set_name:
 * @self: A #ClutterActor
 * @name: Textual tag to apply to actor
 *
 * Sets a textual tag to the actor.
 */
void
clutter_actor_set_name (ClutterActor *self,
			const gchar  *name)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_ref (self);

  g_free (self->priv->name);

  if (name && name[0] != '\0')
    self->priv->name = g_strdup(name);

  g_object_notify (G_OBJECT (self), "name");
  g_object_unref (self);
}

/**
 * clutter_actor_get_name:
 * @self: A #ClutterActor
 *
 * Retrieves the name of @self.
 *
 * Return value: pointer to textual tag for the actor.  The
 *   returned string is owned by the actor and should not
 *   be modified or freed.
 */
G_CONST_RETURN gchar *
clutter_actor_get_name (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), NULL);

  return self->priv->name;
}

/**
 * clutter_actor_get_gid:
 * @self: A #ClutterActor
 *
 * Retrieves the unique id for @self.
 *
 * Return value: Globally unique value for object instance.
 *
 * Since: 0.6
 */
guint32
clutter_actor_get_gid (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return self->priv->id;
}

/**
 * clutter_actor_set_depth:
 * @self: a #ClutterActor
 * @depth: Z co-ord
 *
 * Sets the Z co-ordinate of @self to @depth. The Units of which are dependant
 * on the perspective setup.
 */
void
clutter_actor_set_depth (ClutterActor *self,
                         gint          depth)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (priv->z != depth)
    {
      /* Sets Z value. - FIXME: should invert ?*/
      priv->z = depth;

      if (priv->parent_actor && CLUTTER_IS_CONTAINER (priv->parent_actor))
        {
          ClutterContainer *parent;

          /* We need to resort the container stacking order as to
           * correctly render alpha values.
           *
           * FIXME: This is sub optimal. maybe queue the the sort
           *        before stacking
           */
          parent = CLUTTER_CONTAINER (priv->parent_actor);
          clutter_container_sort_depth_order (parent);
        }

      if (CLUTTER_ACTOR_IS_VISIBLE (self))
        clutter_actor_queue_redraw (self);

      g_object_notify (G_OBJECT (self), "depth");
    }
}

/**
 * clutter_actor_get_depth:
 * @self: a #ClutterActor
 *
 * Retrieves the depth of @self.
 *
 * Return value: the depth of a #ClutterActor
 */
gint
clutter_actor_get_depth (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), -1);

  return self->priv->z;
}

/**
 * clutter_actor_set_rotationx:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @angle: the angle of rotation
 * @x: X coordinate of the rotation center
 * @y: Y coordinate of the rotation center
 * @z: Z coordinate of the rotation center
 *
 * Sets the rotation angle of @self around the given axis.
 *
 * This function is the fixed point variant of clutter_actor_set_rotation().
 *
 * Since: 0.6
 */
void
clutter_actor_set_rotationx (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             ClutterFixed       angle,
                             gint               x,
                             gint               y,
                             gint               z)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  switch (axis)
    {
    case CLUTTER_X_AXIS:
      priv->rxang = angle;
      priv->rxy = y;
      priv->rxz = z;
      break;

    case CLUTTER_Y_AXIS:
      priv->ryang = angle;
      priv->ryx = x;
      priv->ryz = z;
      break;

    case CLUTTER_Z_AXIS:
      priv->rzang = angle;
      priv->rzx = x;
      priv->rzy = y;
      break;
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_set_rotation:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @angle: the angle of rotation
 * @x: X coordinate of the rotation center
 * @y: Y coordinate of the rotation center
 * @z: Z coordinate of the rotation center
 *
 * Sets the rotation angle of @self around the given axis.
 *
 * The rotation center coordinates depend on the value of @axis:
 * <itemizedlist>
 *   <listitem><para>%CLUTTER_X_AXIS requires @y and @z</para></listitem>
 *   <listitem><para>%CLUTTER_Y_AXIS requires @x and @z</para></listitem>
 *   <listitem><para>%CLUTTER_Z_AXIS requires @x and @y</para></listitem>
 * </itemizedlist>
 *
 * Since: 0.6
 */
void
clutter_actor_set_rotation (ClutterActor      *self,
                            ClutterRotateAxis  axis,
                            gdouble            angle,
                            gint               x,
                            gint               y,
                            gint               z)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_rotationx (self, axis,
                               CLUTTER_FLOAT_TO_FIXED (angle),
                               x, y, z);
}

/**
 * clutter_actor_get_rotationx:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @x: return value for the X coordinate of the center of rotation
 * @y: return value for the Y coordinate of the center of rotation
 * @z: return value for the Z coordinate of the center of rotation
 *
 * Retrieves the angle and center of rotation on the given axis,
 * set using clutter_actor_set_rotation().
 *
 * This function is the fixed point variant of clutter_actor_get_rotation().
 *
 * Return value: the angle of rotation as a fixed point value.
 *
 * Since: 0.6
 */
ClutterFixed
clutter_actor_get_rotationx (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             gint              *x,
                             gint              *y,
                             gint              *z)
{
  ClutterActorPrivate *priv;
  ClutterFixed retval = 0;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  priv = self->priv;

  switch (axis)
    {
    case CLUTTER_X_AXIS:
      retval = priv->rxang;
      if (y)
        *y = priv->rxy;
      if (z)
        *z = priv->rxz;
      break;

    case CLUTTER_Y_AXIS:
      retval = priv->ryang;
      if (x)
        *x = priv->ryx;
      if (z)
        *z = priv->ryz;
      break;

    case CLUTTER_Z_AXIS:
      retval = priv->rzang;
      if (x)
        *x = priv->rzx;
      if (y)
        *y = priv->rzy;
      break;
    }

  return retval;
}

/**
 * clutter_actor_get_rotation:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @x: return value for the X coordinate of the center of rotation
 * @y: return value for the Y coordinate of the center of rotation
 * @z: return value for the Z coordinate of the center of rotation
 *
 * Retrieves the angle and center of rotation on the given axis,
 * set using clutter_actor_set_angle().
 *
 * The coordinates of the center depend on the axis used.
 *
 * Return value: the angle of rotation.
 *
 * Since: 0.6
 */
gdouble
clutter_actor_get_rotation (ClutterActor      *self,
                            ClutterRotateAxis  axis,
                            gint              *x,
                            gint              *y,
                            gint              *z)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0.0);

  return CLUTTER_FIXED_TO_FLOAT (clutter_actor_get_rotationx (self,
                                                              axis,
                                                              x, y, z));
}

/**
 * clutter_actor_set_clip:
 * @self: A #ClutterActor
 * @xoff: X offset of the clip rectangle
 * @yoff: Y offset of the clip rectangle
 * @width: Width of the clip rectangle
 * @height: Height of the clip rectangle
 *
 * Sets clip area in pixels for @self.
 */
void
clutter_actor_set_clip (ClutterActor *self,
			gint          xoff,
			gint          yoff,
			gint          width,
			gint          height)
{
  ClutterGeometry *clip;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clip = &(self->priv->clip);

  clip->x      = xoff;
  clip->y      = yoff;
  clip->width  = width;
  clip->height = height;

  self->priv->has_clip = TRUE;

  g_object_notify (G_OBJECT (self), "has-clip");
  g_object_notify (G_OBJECT (self), "clip");
}

/**
 * clutter_actor_remove_clip
 * @self: A #ClutterActor
 *
 * Removes clip area in pixels from @self.
 */
void
clutter_actor_remove_clip (ClutterActor *self)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  self->priv->has_clip = FALSE;

  g_object_notify (G_OBJECT (self), "has-clip");
}

/**
 * clutter_actor_has_clip:
 * @self: a #ClutterActor
 *
 * Gets whether the actor has a clip set or not.
 *
 * Return value: %TRUE if the actor has a clip set.
 *
 * Since: 0.1.1
 */
gboolean
clutter_actor_has_clip (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  return self->priv->has_clip;
}

/**
 * clutter_actor_get_clip:
 * @self: a #ClutterActor
 * @xoff: return location for the X offset of the clip rectangle, or %NULL
 * @yoff: return location for the Y offset of the clip rectangle, or %NULL
 * @width: return location for the width of the clip rectangle, or %NULL
 * @height: return location for the height of the clip rectangle, or %NULL
 *
 * Gets the clip area for @self, in pixels.
 *
 * Since: 0.6
 */
void
clutter_actor_get_clip (ClutterActor *self,
			gint         *xoff,
			gint         *yoff,
			gint         *width,
			gint         *height)
{
  ClutterActorPrivate *priv;
  ClutterGeometry clip = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (!priv->has_clip)
    return;

  clip = priv->clip;

  if (xoff)
    *xoff = clip.x;
  if (yoff)
    *yoff = clip.y;
  if (width)
    *width = clip.width;
  if (height)
    *height = clip.height;
}

/**
 * clutter_actor_set_parent:
 * @self: A #ClutterActor
 * @parent: A new #ClutterActor parent
 *
 * Sets the parent of @self to @parent.  The opposite function is
 * clutter_actor_unparent().
 *
 * This function should not be used by applications but by custom
 * 'composite' actor sub classes.
 */
void
clutter_actor_set_parent (ClutterActor *self,
		          ClutterActor *parent)
{
  ClutterMainContext *clutter_context;

  clutter_context = clutter_context_get_default ();

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (CLUTTER_IS_ACTOR (parent));
  g_return_if_fail (self != parent);
  g_return_if_fail (clutter_context != NULL);

  if (self->priv->parent_actor != NULL)
    {
      g_warning ("Cannot set a parent on an actor which has a parent.\n"
		 "You must use clutter_actor_unparent() first.\n");

      return;
    }

  if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
    {
      g_warning ("Cannot set a parent on a toplevel actor\n");

      return;
    }

  g_hash_table_insert (clutter_context->actor_hash,
		       GUINT_TO_POINTER (clutter_actor_get_gid (self)),
		       (gpointer)self);

  g_object_ref_sink (self);
  self->priv->parent_actor = parent;
  g_signal_emit (self, actor_signals[PARENT_SET], 0, NULL);

  if (CLUTTER_ACTOR_IS_REALIZED (self->priv->parent_actor))
    clutter_actor_realize (self);

  if (CLUTTER_ACTOR_IS_VISIBLE (self->priv->parent_actor) &&
      CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      clutter_actor_queue_redraw (self);
    }
}

/**
 * clutter_actor_get_parent:
 * @self: A #ClutterActor
 *
 * Retrieves the parent of @self.
 *
 * Return Value: The #ClutterActor parent or NULL
 */
ClutterActor *
clutter_actor_get_parent (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), NULL);

  return self->priv->parent_actor;
}

/**
 * clutter_actor_unparent:
 * @self: a #ClutterActor
 *
 * This function should not be used in applications.  It should be called by
 * implementations of container actors, to dissociate a child from the
 * container.
 *
 * Since: 0.1.1
 */
void
clutter_actor_unparent (ClutterActor *self)
{
  ClutterActor *old_parent;
  ClutterMainContext *clutter_context;

  clutter_context = clutter_context_get_default ();

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (clutter_context != NULL);

  if (self->priv->parent_actor == NULL)
    return;

  /* just hide the actor if we are reparenting it */
  if (CLUTTER_ACTOR_IS_REALIZED (self))
    {
      if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_REPARENT)
        clutter_actor_hide (self);
      else
        clutter_actor_unrealize (self);
    }

  old_parent = self->priv->parent_actor;
  self->priv->parent_actor = NULL;
  g_signal_emit (self, actor_signals[PARENT_SET], 0, old_parent);

  g_hash_table_remove (clutter_context->actor_hash,
		       GUINT_TO_POINTER (clutter_actor_get_gid (self)));

  g_object_unref (self);
}

/**
 * clutter_actor_reparent:
 * @self: a #ClutterActor
 * @new_parent: the new #ClutterActor parent
 *
 * This function resets the parent actor of @self.  It is
 * logically equivalent to calling clutter_actory_unparent()
 * and clutter_actor_set_parent().
 *
 * Since: 0.2
 */
void
clutter_actor_reparent (ClutterActor *self,
                        ClutterActor *new_parent)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (CLUTTER_IS_ACTOR (new_parent));
  g_return_if_fail (self != new_parent);

  if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
    {
      g_warning ("Cannot set a parent on a toplevel actor\n");
      return;
    }

  priv = self->priv;

  if (priv->parent_actor != new_parent)
    {
      ClutterActor *old_parent;

      /* if the actor and the parent have already been realized,
       * mark the actor as reparenting, so that clutter_actor_unparent()
       * just hides the actor instead of unrealize it.
       */
      if (CLUTTER_ACTOR_IS_REALIZED (self) &&
          CLUTTER_ACTOR_IS_REALIZED (new_parent))
        {
          CLUTTER_SET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_REPARENT);
        }

      old_parent = priv->parent_actor;

      g_object_ref (self);

      /* FIXME: below assumes only containers can reparent */
      if (CLUTTER_IS_CONTAINER (priv->parent_actor))
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->parent_actor),
                                        self);
      else
        priv->parent_actor = NULL;

      if (CLUTTER_IS_CONTAINER (new_parent))
        clutter_container_add_actor (CLUTTER_CONTAINER (new_parent), self);
      else
        priv->parent_actor = new_parent;

      g_object_unref (self);

      if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_REPARENT)
        {
          CLUTTER_UNSET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_REPARENT);

          clutter_actor_queue_redraw (self);
        }
   }
}
/**
 * clutter_actor_raise:
 * @self: A #ClutterActor
 * @below: A #ClutterActor to raise above.
 *
 * Puts @self above @below.
 * Both actors must have the same parent.
 */
void
clutter_actor_raise (ClutterActor *self,
                     ClutterActor *below)
{
  ClutterActor *parent;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  parent = clutter_actor_get_parent (self);
  if (!parent)
    {
      g_warning ("Actor of type %s is not inside a container",
                 g_type_name (G_OBJECT_TYPE (self)));
      return;
    }

  if (below)
    {
      if (parent != clutter_actor_get_parent (below))
        {
          g_warning ("Actor of type %s is not in the same "
                     "container of actor of type %s",
                     g_type_name (G_OBJECT_TYPE (self)),
                     g_type_name (G_OBJECT_TYPE (below)));
          return;
        }
    }

  clutter_container_raise_child (CLUTTER_CONTAINER (parent), self, below);
}

/**
 * clutter_actor_lower:
 * @self: A #ClutterActor
 * @above: A #ClutterActor to lower below
 *
 * Puts @self below @above.
 * Both actors must have the same parent.
 */
void
clutter_actor_lower (ClutterActor *self,
                     ClutterActor *above)
{
  ClutterActor *parent;

  g_return_if_fail (CLUTTER_IS_ACTOR(self));

  parent = clutter_actor_get_parent (self);
  if (!parent)
    {
      g_warning ("Actor of type %s is not inside a container",
                 g_type_name (G_OBJECT_TYPE (self)));
      return;
    }

  if (above)
    {
      if (parent != clutter_actor_get_parent (above))
        {
          g_warning ("Actor of type %s is not in the same "
                     "container of actor of type %s",
                     g_type_name (G_OBJECT_TYPE (self)),
                     g_type_name (G_OBJECT_TYPE (above)));
          return;
        }
    }

  clutter_container_lower_child (CLUTTER_CONTAINER (parent), self, above);
}

/**
 * clutter_actor_raise_top:
 * @self: A #ClutterActor
 *
 * Raises @self to the top.
 */
void
clutter_actor_raise_top (ClutterActor *self)
{
  clutter_actor_raise (self, NULL);
}

/**
 * clutter_actor_lower_bottom:
 * @self: A #ClutterActor
 *
 * Lowers @self to the bottom.
 */
void
clutter_actor_lower_bottom (ClutterActor *self)
{
  clutter_actor_lower (self, NULL);
}

/*
 * Event handling
 */

/**
 * clutter_actor_event:
 * @actor: a #ClutterActor
 * @event: a #ClutterEvent
 * @capture: TRUE if event in in capture phase, FALSE otherwise.
 *
 * This function is used to emit an event on the main stage.
 * You should rarely need to use this function, except for
 * synthetising events.
 *
 * Return value: the return value from the signal emission: %TRUE
 *   if the actor handled the event, or %FALSE if the event was
 *   not handled
 *
 * Since: 0.6
 */
gboolean
clutter_actor_event (ClutterActor *actor,
                     ClutterEvent *event,
		     gboolean      capture)
{
  gboolean retval = FALSE;
  gint signal_num = -1;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (actor), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  g_object_ref (actor);

  if (capture)
    {
      g_signal_emit (actor, actor_signals[CAPTURED_EVENT], 0,
		     event,
                     &retval);
      goto out;
    }

  g_signal_emit (actor, actor_signals[EVENT], 0, event, &retval);

  if (!retval)
    {
      switch (event->type)
	{
	case CLUTTER_NOTHING:
	  break;
	case CLUTTER_BUTTON_PRESS:
	  signal_num = BUTTON_PRESS_EVENT;
	  break;
	case CLUTTER_BUTTON_RELEASE:
	  signal_num = BUTTON_RELEASE_EVENT;
	  break;
	case CLUTTER_SCROLL:
	  signal_num = SCROLL_EVENT;
	  break;
	case CLUTTER_KEY_PRESS:
	  signal_num = KEY_PRESS_EVENT;
	  break;
	case CLUTTER_KEY_RELEASE:
	  signal_num = KEY_RELEASE_EVENT;
	  break;
	case CLUTTER_MOTION:
	  signal_num = MOTION_EVENT;
	  break;
	case CLUTTER_ENTER:
	  signal_num = ENTER_EVENT;
	  break;
	case CLUTTER_LEAVE:
	  signal_num = LEAVE_EVENT;
	  break;
	case CLUTTER_DELETE:
	case CLUTTER_DESTROY_NOTIFY:
	case CLUTTER_CLIENT_MESSAGE:
	default:
	  signal_num = -1;
	  break;
	}

      if (signal_num != -1)
	g_signal_emit (actor, actor_signals[signal_num], 0,
		       event, &retval);
    }

out:
  g_object_unref (actor);

  return retval;
}

/**
 * clutter_actor_set_reactive:
 * @actor: a #ClutterActor
 * @reactive: whether the actor should be reactive to events
 *
 * Sets @actor as reactive. Reactive actors will receive events.
 *
 * Since: 0.6
 */
void
clutter_actor_set_reactive (ClutterActor *actor,
                            gboolean      reactive)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  if (reactive == CLUTTER_ACTOR_IS_REACTIVE (actor))
    return;

  if (reactive)
    CLUTTER_ACTOR_SET_FLAGS (actor, CLUTTER_ACTOR_REACTIVE);
  else
    CLUTTER_ACTOR_UNSET_FLAGS (actor, CLUTTER_ACTOR_REACTIVE);
}

/**
 * clutter_actor_get_reactive:
 * @actor: a #ClutterActor
 *
 * Checks whether @actor is marked as reactive.
 *
 * Return value: %TRUE if the actor is reactive
 *
 * Since: 0.6
 */
gboolean
clutter_actor_get_reactive (ClutterActor *actor)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (actor), FALSE);

  return CLUTTER_ACTOR_IS_REACTIVE (actor);
}

/**
 * clutter_actor_set_anchor_point:
 * @actor: a #ClutterActor
 * @anchor_x: X coordinace of the anchor point.
 * @anchor_y: Y coordinace of the anchor point.
 *
 * Sets an anchor point for the @actor. The anchor point is a point in the
 * coordianate space of the actor to which the actor position within its
 * parent is relative; the default is 0,0, i.e., the top-left corner.
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_point (ClutterActor *self,
				gint anchor_x, gint anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  priv->anchor_x = CLUTTER_UNITS_FROM_DEVICE (anchor_x);
  priv->anchor_y = CLUTTER_UNITS_FROM_DEVICE (anchor_y);
}

/**
 * clutter_actor_get_anchor_point:
 * @actor: a #ClutterActor
 * @anchor_x: location for the X coordinace of the anchor point.
 * @anchor_y: location for the X coordinace of the anchor point.
 *
 * Gets the current anchor point of the @actor.
 *
 * Since: 0.6
 */
void
clutter_actor_get_anchor_point (ClutterActor *self,
				gint *anchor_x, gint *anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (anchor_x)
    *anchor_x = CLUTTER_UNITS_TO_DEVICE (priv->anchor_x);

  if (anchor_y)
    *anchor_y = CLUTTER_UNITS_TO_DEVICE (priv->anchor_y);
}

/**
 * clutter_actor_set_anchor_pointu:
 * @actor: a #ClutterActor
 * @anchor_x: X coordinace of the anchor point, in multiples of #ClutterUnit.
 * @anchor_y: Y coordinace of the anchor point, in multiples of #ClutterUnit.
 *
 * Sets an anchor point for the @actor. The anchor point is a point in the
 * coordinate space of the actor to which the actor position within its
 * parent is relative; the default is 0,0, i.e., the top-left corner, of the
 * actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_pointu (ClutterActor *self,
				 ClutterUnit anchor_x, ClutterUnit anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  priv->anchor_x = anchor_x;
  priv->anchor_y = anchor_y;
}

/**
 * clutter_actor_get_anchor_pointu:
 * @actor: a #ClutterActor
 * @anchor_x: location for the X coordinace of the anchor point, #ClutterUnit.
 * @anchor_y: location for the X coordinace of the anchor point, #ClutterUnit.
 *
 * Gets the current anchor point of the @actor.
 *
 * Since: 0.6
 */
void
clutter_actor_get_anchor_pointu (ClutterActor *self,
				 ClutterUnit *anchor_x, ClutterUnit *anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (anchor_x)
    *anchor_x = priv->anchor_x;

  if (anchor_y)
    *anchor_y = priv->anchor_y;
}

/**
 * clutter_actor_set_anchor_point_from_gravity:
 * @actor: a #ClutterActor
 * @gravity: #ClutterGravity.
 *
 * Sets an anchor point the actor based on the given gravity (this is a
 * convenience function wrapping #clutter_actor_set_anchor_point).
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_point_from_gravity (ClutterActor *self,
					     ClutterGravity gravity)
{
  ClutterActorPrivate *priv;
  ClutterActorBox box;
  ClutterUnit w, h, x, y;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  clutter_actor_query_coords (self, &box);

  x = 0;
  y = 0;
  w  = box.x2 - box.x1;
  h  = box.y2 - box.y1;

  switch (gravity)
    {
    case CLUTTER_GRAVITY_NORTH:
      x = w/2;
      break;
    case CLUTTER_GRAVITY_SOUTH:
      x = w/2;
      y = h;
      break;
    case CLUTTER_GRAVITY_EAST:
      x = w;
      y = h/2;
      break;
    case CLUTTER_GRAVITY_NORTH_EAST:
      x = w;
      break;
    case CLUTTER_GRAVITY_SOUTH_EAST:
      x = w;
      y = h;
      break;
    case CLUTTER_GRAVITY_SOUTH_WEST:
      y = h;
      break;
    case CLUTTER_GRAVITY_WEST:
      y = h/2;
      break;
    case CLUTTER_GRAVITY_CENTER:
      x = w/2;
      y = h/2;
      break;
    case CLUTTER_GRAVITY_NONE:
    case CLUTTER_GRAVITY_NORTH_WEST:
    default:
      break;
    }

  priv->anchor_x = x;
  priv->anchor_y = y;
}

/*
 * ClutterGeometry
 */

static ClutterGeometry*
clutter_geometry_copy (const ClutterGeometry *geometry)
{
  ClutterGeometry *result = g_slice_new (ClutterGeometry);

  *result = *geometry;

  return result;
}

static void
clutter_geometry_free (ClutterGeometry *geometry)
{
  if (G_LIKELY (geometry))
    g_slice_free (ClutterGeometry, geometry);
}

GType
clutter_geometry_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static
                   (g_intern_static_string ("ClutterGeometry"),
		    (GBoxedCopyFunc) clutter_geometry_copy,
		    (GBoxedFreeFunc) clutter_geometry_free);

  return our_type;
}

/*
 * ClutterVertices
 */

static ClutterVertex *
clutter_vertex_copy (const ClutterVertex *vertex)
{
  ClutterVertex *result = g_slice_new (ClutterVertex);

  *result = *vertex;

  return result;
}

static void
clutter_vertex_free (ClutterVertex *vertex)
{
  if (G_UNLIKELY (vertex))
    g_slice_free (ClutterVertex, vertex);
}

GType
clutter_vertex_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static
                   (g_intern_static_string ("ClutterVertex"),
		    (GBoxedCopyFunc) clutter_vertex_copy,
		    (GBoxedFreeFunc) clutter_vertex_free);

  return our_type;
}

/*
 * ClutterActorBox
 */
static ClutterActorBox *
clutter_actor_box_copy (const ClutterActorBox *box)
{
  ClutterActorBox *result = g_slice_new (ClutterActorBox);

  *result = *box;

  return result;
}

static void
clutter_actor_box_free (ClutterActorBox *box)
{
  if (G_LIKELY (box))
    g_slice_free (ClutterActorBox, box);
}

GType
clutter_actor_box_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static
                   (g_intern_static_string ("ClutterActorBox"),
		    (GBoxedCopyFunc) clutter_actor_box_copy,
		    (GBoxedFreeFunc) clutter_actor_box_free);
  return our_type;
}
