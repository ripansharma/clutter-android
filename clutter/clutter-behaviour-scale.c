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
 * SECTION:clutter-behaviour-scale
 * @short_description: A behaviour controlling scale
 *
 * A #ClutterBehaviourScale interpolates actors size between two values.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-actor.h"
#include "clutter-behaviour.h"
#include "clutter-enum-types.h"
#include "clutter-main.h"
#include "clutter-fixed.h"
#include "clutter-behaviour-scale.h"
#include "clutter-private.h"
#include "clutter-debug.h"

#include <math.h>

G_DEFINE_TYPE (ClutterBehaviourScale,
               clutter_behaviour_scale,
	       CLUTTER_TYPE_BEHAVIOUR);

struct _ClutterBehaviourScalePrivate
{
  ClutterFixed scale_start;
  ClutterFixed scale_end;

  ClutterGravity gravity;
};

#define CLUTTER_BEHAVIOUR_SCALE_GET_PRIVATE(obj)        \
              (G_TYPE_INSTANCE_GET_PRIVATE ((obj),      \
               CLUTTER_TYPE_BEHAVIOUR_SCALE,            \
               ClutterBehaviourScalePrivate))

enum
{
  PROP_0,

  PROP_SCALE_START,
  PROP_SCALE_END,
  PROP_SCALE_GRAVITY
};

static void
scale_frame_foreach (ClutterBehaviour *behaviour,
                     ClutterActor     *actor,
		     gpointer          data)
{
  ClutterBehaviourScalePrivate *priv =
      CLUTTER_BEHAVIOUR_SCALE (behaviour)->priv;
  ClutterFixed scale = GPOINTER_TO_UINT (data);
  ClutterGravity gravity = priv->gravity;

  /* Don't mess with the actor anchor point of gravity is set to
   * none
   */
  if (gravity != CLUTTER_GRAVITY_NONE)
    clutter_actor_set_anchor_point_from_gravity (actor, gravity);

  clutter_actor_set_scalex (actor, scale, scale);
}

static void
clutter_behaviour_scale_alpha_notify (ClutterBehaviour *behave,
                                      guint32           alpha_value)
{
  ClutterFixed scale, factor;
  ClutterBehaviourScalePrivate *priv;

  priv = CLUTTER_BEHAVIOUR_SCALE (behave)->priv;

  factor = CLUTTER_INT_TO_FIXED (alpha_value) / CLUTTER_ALPHA_MAX_ALPHA;
  scale = CLUTTER_FIXED_MUL (factor, (priv->scale_end - priv->scale_start));
  scale += priv->scale_start;

  clutter_behaviour_actors_foreach (behave,
                                    scale_frame_foreach,
                                    GUINT_TO_POINTER (scale));
}

static void
clutter_behaviour_scale_set_property (GObject      *gobject,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ClutterBehaviourScalePrivate *priv;

  priv = CLUTTER_BEHAVIOUR_SCALE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SCALE_START:
      priv->scale_start = CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value));
      break;
    case PROP_SCALE_END:
      priv->scale_end = CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value));
      break;
    case PROP_SCALE_GRAVITY:
      priv->gravity = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_behaviour_scale_get_property (GObject    *gobject,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ClutterBehaviourScalePrivate *priv;

  priv = CLUTTER_BEHAVIOUR_SCALE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SCALE_START:
      g_value_set_double (value, CLUTTER_FIXED_TO_FLOAT (priv->scale_start));
      break;
    case PROP_SCALE_END:
      g_value_set_double (value, CLUTTER_FIXED_TO_FLOAT (priv->scale_end));
      break;
    case PROP_SCALE_GRAVITY:
      g_value_set_enum (value, priv->gravity);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
clutter_behaviour_scale_class_init (ClutterBehaviourScaleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterBehaviourClass *behave_class = CLUTTER_BEHAVIOUR_CLASS (klass);

  gobject_class->set_property = clutter_behaviour_scale_set_property;
  gobject_class->get_property = clutter_behaviour_scale_get_property;

  /**
   * ClutterBehaviourScale:scale-start:
   *
   * The initial scaling factor for the actors.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SCALE_START,
                                   g_param_spec_double ("scale-start",
                                                        "Start Scale",
                                                        "Initial scale",
                                                        0.0, G_MAXDOUBLE,
                                                        1.0,
                                                        CLUTTER_PARAM_READWRITE));
  /**
   * ClutterBehaviourScale:scale-end:
   *
   * The final scaling factor for the actors.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SCALE_END,
                                   g_param_spec_double ("scale-end",
                                                        "End Scale",
                                                        "Final scale",
                                                        0.0, G_MAXDOUBLE,
                                                        1.0,
                                                        CLUTTER_PARAM_READWRITE));
  /**
   * ClutterBehaviourScale:gravity:
   *
   * The gravity of the scaling.
   *
   * Since: 0.2
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SCALE_GRAVITY,
                                   g_param_spec_enum ("scale-gravity",
                                                      "Scale Gravity",
                                                      "The gravity of the scaling",
                                                      CLUTTER_TYPE_GRAVITY,
                                                      CLUTTER_GRAVITY_CENTER,
                                                      CLUTTER_PARAM_READWRITE));


  behave_class->alpha_notify = clutter_behaviour_scale_alpha_notify;

  g_type_class_add_private (klass, sizeof (ClutterBehaviourScalePrivate));
}

static void
clutter_behaviour_scale_init (ClutterBehaviourScale *self)
{
  ClutterBehaviourScalePrivate *priv;

  self->priv = priv = CLUTTER_BEHAVIOUR_SCALE_GET_PRIVATE (self);
}

/**
 * clutter_behaviour_scale_new:
 * @alpha: a #ClutterAlpha
 * @scale_start: initial scale factor
 * @scale_end: final scale factor
 * @gravity: a #ClutterGravity for the scale.
 *
 * Creates a new  #ClutterBehaviourScale instance.
 *
 * Return value: the newly created #ClutterBehaviourScale
 *
 * Since: 0.2
 */
ClutterBehaviour *
clutter_behaviour_scale_new (ClutterAlpha   *alpha,
			     gdouble         scale_start,
			     gdouble         scale_end,
			     ClutterGravity  gravity)
{
  g_return_val_if_fail (alpha == NULL || CLUTTER_IS_ALPHA (alpha), NULL);

  return clutter_behaviour_scale_newx (alpha,
				       CLUTTER_FLOAT_TO_FIXED (scale_start),
				       CLUTTER_FLOAT_TO_FIXED (scale_end),
				       gravity);
}

/**
 * clutter_behaviour_scale_newx:
 * @alpha: a #ClutterAlpha
 * @scale_start: initial scale factor
 * @scale_end: final scale factor
 * @gravity: a #ClutterGravity for the scale.
 *
 * A fixed point implementation of clutter_behaviour_scale_new()
 *
 * Return value: the newly created #ClutterBehaviourScale
 *
 * Since: 0.2
 */
ClutterBehaviour *
clutter_behaviour_scale_newx (ClutterAlpha   *alpha,
			      ClutterFixed    scale_start,
			      ClutterFixed    scale_end,
			      ClutterGravity  gravity)
{
  ClutterBehaviourScale *behave;

  g_return_val_if_fail (alpha == NULL || CLUTTER_IS_ALPHA (alpha), NULL);

  behave = g_object_new (CLUTTER_TYPE_BEHAVIOUR_SCALE,
                         "alpha", alpha,
			 NULL);

  behave->priv->scale_start = scale_start;
  behave->priv->scale_end   = scale_end;
  behave->priv->gravity     = gravity;

  return CLUTTER_BEHAVIOUR (behave);
}

/**
 * clutter_behaviour_scale_get_bounds:
 * @scale: a #ClutterBehaviourScale
 * @scale_start: return location for the initial scale factor
 * @scale_end: return location for the final scale factor
 *
 * Retrieves the bounds used by scale behaviour.
 *
 * Since: 0.4
 */
void
clutter_behaviour_scale_get_bounds (ClutterBehaviourScale *scale,
                                    gdouble               *scale_start,
                                    gdouble               *scale_end)
{
  ClutterBehaviourScalePrivate *priv;

  g_return_if_fail (CLUTTER_IS_BEHAVIOUR_SCALE (scale));

  priv = scale->priv;

  if (scale_start)
    *scale_start = CLUTTER_FIXED_TO_DOUBLE (priv->scale_start);

  if (scale_end)
    *scale_end = CLUTTER_FIXED_TO_DOUBLE (priv->scale_end);
}

/**
 * clutter_behaviour_scale_get_boundsx:
 * @scale: a #ClutterBehaviourScale
 * @scale_start: return location for the initial scale factor
 * @scale_end: return location for the final scale factor
 *
 * Retrieves the bounds used by scale behaviour.
 *
 * Since: 0.4
 */
void
clutter_behaviour_scale_get_boundsx (ClutterBehaviourScale *scale,
                                     ClutterFixed          *scale_start,
                                     ClutterFixed          *scale_end)
{
  ClutterBehaviourScalePrivate *priv;

  g_return_if_fail (CLUTTER_IS_BEHAVIOUR_SCALE (scale));

  priv = scale->priv;

  if (scale_start)
    *scale_start = priv->scale_start;

  if (scale_end)
    *scale_end = priv->scale_end;
}

/**
 * clutter_behaviour_scale_get_gravity:
 * @scale: a #ClutterBehaviourScale
 *
 * Retrieves the #ClutterGravity applied by the scale behaviour.
 *
 * Return value: the gravity used by the behaviour
 *
 * Since: 0.4
 */
ClutterGravity
clutter_behaviour_scale_get_gravity (ClutterBehaviourScale *scale)
{
  g_return_val_if_fail (CLUTTER_IS_BEHAVIOUR_SCALE (scale), CLUTTER_GRAVITY_NONE);

  return scale->priv->gravity;
}
