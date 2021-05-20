// Copyright Oleg Maximenko 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://github.com/svgpp/svgpp for library home page.

#pragma once

namespace svgpp { 
  namespace detail {

  enum attribute_id {
    attribute_id_alignment_baseline,
    attribute_id_baseline_shift,
    attribute_id_clip,
    attribute_id_clip_path,
    attribute_id_clip_rule,
    attribute_id_color,
    attribute_id_color_interpolation,
    attribute_id_color_interpolation_filters,
    attribute_id_color_profile,
    attribute_id_color_rendering,
    attribute_id_cursor,
    attribute_id_direction,
    attribute_id_display,
    attribute_id_dominant_baseline,
    attribute_id_enable_background,
    attribute_id_fill,
    attribute_id_fill_opacity,
    attribute_id_fill_rule,
    attribute_id_filter,
    attribute_id_flood_color,
    attribute_id_flood_opacity,
    attribute_id_font,
    attribute_id_font_family,
    attribute_id_font_size,
    attribute_id_font_size_adjust,
    attribute_id_font_stretch,
    attribute_id_font_style,
    attribute_id_font_variant,
    attribute_id_font_weight,
    attribute_id_glyph_orientation_horizontal,
    attribute_id_glyph_orientation_vertical,
    attribute_id_image_rendering,
    attribute_id_kerning,
    attribute_id_letter_spacing,
    attribute_id_lighting_color,
    attribute_id_marker,
    attribute_id_marker_end,
    attribute_id_marker_mid,
    attribute_id_marker_start,
    attribute_id_mask,
    attribute_id_opacity,
    attribute_id_overflow,
    attribute_id_pointer_events,
    attribute_id_shape_rendering,
    attribute_id_stop_color,
    attribute_id_stop_opacity,
    attribute_id_stroke,
    attribute_id_stroke_dasharray,
    attribute_id_stroke_dashoffset,
    attribute_id_stroke_linecap,
    attribute_id_stroke_linejoin,
    attribute_id_stroke_miterlimit,
    attribute_id_stroke_opacity,
    attribute_id_stroke_width,
    attribute_id_text_anchor,
    attribute_id_text_decoration,
    attribute_id_text_rendering,
    attribute_id_unicode_bidi,
    attribute_id_visibility,
    attribute_id_word_spacing,
    attribute_id_writing_mode,
    styling_attribute_count,
    last_styling_attribute = styling_attribute_count - 1,
    #define SVGPP_ON(attribute_name, attribute_string) attribute_id_## attribute_name,
    #define SVGPP_ON_NS(ns, attribute_name, attribute_string) SVGPP_ON(ns ## _ ## attribute_name, attribute_string) 
    #define SVGPP_ON_STYLE(attribute_name, attribute_string)
    SVGPP_ON      (accent_height                      , accent-height)
SVGPP_ON      (accumulate                         , accumulate)
SVGPP_ON      (additive                           , additive)
SVGPP_ON      (alphabetic                         , alphabetic)
SVGPP_ON      (amplitude                          , amplitude)
SVGPP_ON      (arabic_form                        , arabic-form)
SVGPP_ON      (ascent                             , ascent)
SVGPP_ON      (attributeName                      , attributeName)
SVGPP_ON      (attributeType                      , attributeType)
SVGPP_ON      (azimuth                            , azimuth)
SVGPP_ON      (baseFrequency                      , baseFrequency)
SVGPP_ON      (baseProfile                        , baseProfile)
SVGPP_ON      (bbox                               , bbox)
SVGPP_ON      (begin                              , begin)
SVGPP_ON      (bias                               , bias)
SVGPP_ON      (by                                 , by)
SVGPP_ON      (calcMode                           , calcMode)
SVGPP_ON      (cap_height                         , cap-height)
SVGPP_ON      (class_                             , class)
SVGPP_ON      (clipPathUnits                      , clipPathUnits)
SVGPP_ON      (contentScriptType                  , contentScriptType)
SVGPP_ON      (contentStyleType                   , contentStyleType)
SVGPP_ON      (cx                                 , cx)
SVGPP_ON      (cy                                 , cy)
SVGPP_ON      (d                                  , d)
SVGPP_ON      (descent                            , descent)
SVGPP_ON      (diffuseConstant                    , diffuseConstant)
SVGPP_ON      (divisor                            , divisor)
SVGPP_ON      (dur                                , dur)
SVGPP_ON      (dx                                 , dx)
SVGPP_ON      (dy                                 , dy)
SVGPP_ON      (edgeMode                           , edgeMode)
SVGPP_ON      (elevation                          , elevation)
SVGPP_ON      (end                                , end)
SVGPP_ON      (exponent                           , exponent)
SVGPP_ON      (externalResourcesRequired          , externalResourcesRequired)
SVGPP_ON      (filterRes                          , filterRes)
SVGPP_ON      (filterUnits                        , filterUnits)
SVGPP_ON      (format                             , format)
SVGPP_ON      (from                               , from)
SVGPP_ON      (fx                                 , fx)
SVGPP_ON      (fy                                 , fy)
SVGPP_ON      (g1                                 , g1)
SVGPP_ON      (g2                                 , g2)
SVGPP_ON      (glyph_name                         , glyph-name)
SVGPP_ON      (glyphRef                           , glyphRef)
SVGPP_ON      (gradientTransform                  , gradientTransform)
SVGPP_ON      (gradientUnits                      , gradientUnits)
SVGPP_ON      (hanging                            , hanging)
SVGPP_ON      (height                             , height)
SVGPP_ON      (horiz_adv_x                        , horiz-adv-x)
SVGPP_ON      (horiz_origin_x                     , horiz-origin-x)
SVGPP_ON      (horiz_origin_y                     , horiz-origin-y)
SVGPP_ON      (id                                 , id)
SVGPP_ON      (ideographic                        , ideographic)
SVGPP_ON      (in                                 , in)
SVGPP_ON      (in2                                , in2)
SVGPP_ON      (intercept                          , intercept)
SVGPP_ON      (k                                  , k)
SVGPP_ON      (k1                                 , k1)
SVGPP_ON      (k2                                 , k2)
SVGPP_ON      (k3                                 , k3)
SVGPP_ON      (k4                                 , k4)
SVGPP_ON      (kernelMatrix                       , kernelMatrix)
SVGPP_ON      (kernelUnitLength                   , kernelUnitLength)
SVGPP_ON      (keyPoints                          , keyPoints)
SVGPP_ON      (keySplines                         , keySplines)
SVGPP_ON      (keyTimes                           , keyTimes)
SVGPP_ON      (lang                               , lang)
SVGPP_ON      (lengthAdjust                       , lengthAdjust)
SVGPP_ON      (limitingConeAngle                  , limitingConeAngle)
SVGPP_ON      (local                              , local)
SVGPP_ON      (markerHeight                       , markerHeight)
SVGPP_ON      (markerUnits                        , markerUnits)
SVGPP_ON      (markerWidth                        , markerWidth)
SVGPP_ON      (maskContentUnits                   , maskContentUnits)
SVGPP_ON      (maskUnits                          , maskUnits)
SVGPP_ON      (mathematical                       , mathematical)
SVGPP_ON      (max                                , max)
SVGPP_ON      (media                              , media)
SVGPP_ON      (method                             , method)
SVGPP_ON      (min                                , min)
SVGPP_ON      (mode                               , mode)
SVGPP_ON      (name                               , name)
SVGPP_ON      (numOctaves                         , numOctaves)
SVGPP_ON      (offset                             , offset)
SVGPP_ON      (onabort                            , onabort)
SVGPP_ON      (onactivate                         , onactivate)
SVGPP_ON      (onbegin                            , onbegin)
SVGPP_ON      (onclick                            , onclick)
SVGPP_ON      (onend                              , onend)
SVGPP_ON      (onerror                            , onerror)
SVGPP_ON      (onfocusin                          , onfocusin)
SVGPP_ON      (onfocusout                         , onfocusout)
SVGPP_ON      (onload                             , onload)
SVGPP_ON      (onmousedown                        , onmousedown)
SVGPP_ON      (onmousemove                        , onmousemove)
SVGPP_ON      (onmouseout                         , onmouseout)
SVGPP_ON      (onmouseover                        , onmouseover)
SVGPP_ON      (onmouseup                          , onmouseup)
SVGPP_ON      (onrepeat                           , onrepeat)
SVGPP_ON      (onresize                           , onresize)
SVGPP_ON      (onscroll                           , onscroll)
SVGPP_ON      (onunload                           , onunload)
SVGPP_ON      (onzoom                             , onzoom)
SVGPP_ON      (operator_                          , operator)
SVGPP_ON      (order                              , order)
SVGPP_ON      (orient                             , orient)
SVGPP_ON      (orientation                        , orientation)
SVGPP_ON      (origin                             , origin)
SVGPP_ON      (overline_position                  , overline-position)
SVGPP_ON      (overline_thickness                 , overline-thickness)
SVGPP_ON      (panose_1                           , panose-1)
SVGPP_ON      (path                               , path)
SVGPP_ON      (pathLength                         , pathLength)
SVGPP_ON      (patternContentUnits                , patternContentUnits)
SVGPP_ON      (patternTransform                   , patternTransform)
SVGPP_ON      (patternUnits                       , patternUnits)
SVGPP_ON      (points                             , points)
SVGPP_ON      (pointsAtX                          , pointsAtX)
SVGPP_ON      (pointsAtY                          , pointsAtY)
SVGPP_ON      (pointsAtZ                          , pointsAtZ)
SVGPP_ON      (preserveAlpha                      , preserveAlpha)
SVGPP_ON      (preserveAspectRatio                , preserveAspectRatio)
SVGPP_ON      (primitiveUnits                     , primitiveUnits)
SVGPP_ON      (r                                  , r)
SVGPP_ON      (radius                             , radius)
SVGPP_ON      (refX                               , refX)
SVGPP_ON      (refY                               , refY)
SVGPP_ON      (rendering_intent                   , rendering-intent)
SVGPP_ON      (repeatCount                        , repeatCount)
SVGPP_ON      (repeatDur                          , repeatDur)
SVGPP_ON      (requiredExtensions                 , requiredExtensions)
SVGPP_ON      (requiredFeatures                   , requiredFeatures)
SVGPP_ON      (restart                            , restart)
SVGPP_ON      (result                             , result)
SVGPP_ON      (rotate                             , rotate)
SVGPP_ON      (rx                                 , rx)
SVGPP_ON      (ry                                 , ry)
SVGPP_ON      (scale                              , scale)
SVGPP_ON      (seed                               , seed)
SVGPP_ON      (slope                              , slope)
SVGPP_ON      (spacing                            , spacing)
SVGPP_ON      (specularConstant                   , specularConstant)
SVGPP_ON      (specularExponent                   , specularExponent)
SVGPP_ON      (spreadMethod                       , spreadMethod)
SVGPP_ON      (startOffset                        , startOffset)
SVGPP_ON      (stdDeviation                       , stdDeviation)
SVGPP_ON      (stemh                              , stemh)
SVGPP_ON      (stemv                              , stemv)
SVGPP_ON      (stitchTiles                        , stitchTiles)
SVGPP_ON      (strikethrough_position             , strikethrough-position)
SVGPP_ON      (strikethrough_thickness            , strikethrough-thickness)
SVGPP_ON      (string                             , string)
SVGPP_ON      (style                              , style)
SVGPP_ON      (surfaceScale                       , surfaceScale)
SVGPP_ON      (systemLanguage                     , systemLanguage)
SVGPP_ON      (tableValues                        , tableValues)
SVGPP_ON      (target                             , target)
SVGPP_ON      (targetX                            , targetX)
SVGPP_ON      (targetY                            , targetY)
SVGPP_ON      (textLength                         , textLength)
SVGPP_ON      (title                              , title)
SVGPP_ON      (to                                 , to)
SVGPP_ON      (transform                          , transform)
SVGPP_ON      (type                               , type)
SVGPP_ON      (u1                                 , u1)
SVGPP_ON      (u2                                 , u2)
SVGPP_ON      (underline_position                 , underline-position)
SVGPP_ON      (underline_thickness                , underline-thickness)
SVGPP_ON      (unicode                            , unicode)
SVGPP_ON      (unicode_range                      , unicode-range)
SVGPP_ON      (units_per_em                       , units-per-em)
SVGPP_ON      (v_alphabetic                       , v-alphabetic)
SVGPP_ON      (v_hanging                          , v-hanging)
SVGPP_ON      (v_ideographic                      , v-ideographic)
SVGPP_ON      (v_mathematical                     , v-mathematical)
SVGPP_ON      (values                             , values)
SVGPP_ON      (version                            , version)
SVGPP_ON      (vert_adv_y                         , vert-adv-y)
SVGPP_ON      (vert_origin_x                      , vert-origin-x)
SVGPP_ON      (vert_origin_y                      , vert-origin-y)
SVGPP_ON      (viewBox                            , viewBox)
SVGPP_ON      (viewTarget                         , viewTarget)
SVGPP_ON      (width                              , width)
SVGPP_ON      (widths                             , widths)
SVGPP_ON      (x                                  , x)
SVGPP_ON      (x_height                           , x-height)
SVGPP_ON      (x1                                 , x1)
SVGPP_ON      (x2                                 , x2)
SVGPP_ON      (xChannelSelector                   , xChannelSelector)
SVGPP_ON_NS   (xlink, actuate                     , actuate)
SVGPP_ON_NS   (xlink, arcrole                     , arcrole)
SVGPP_ON_NS   (xlink, href                        , href)
SVGPP_ON_NS   (xlink, role                        , role)
SVGPP_ON_NS   (xlink, show                        , show)
SVGPP_ON_NS   (xlink, title                       , title)
SVGPP_ON_NS   (xlink, type                        , type)
SVGPP_ON_NS   (xml,   base                        , base)
SVGPP_ON_NS   (xml,   lang                        , lang)
SVGPP_ON_NS   (xml,   space                       , space)
SVGPP_ON      (y                                  , y)
SVGPP_ON      (y1                                 , y1)
SVGPP_ON      (y2                                 , y2)
SVGPP_ON      (yChannelSelector                   , yChannelSelector)
SVGPP_ON      (z                                  , z)
SVGPP_ON      (zoomAndPan                         , zoomAndPan)
    #undef SVGPP_ON
    #undef SVGPP_ON_NS
    #undef SVGPP_ON_STYLE

    attribute_count,
    unknown_attribute_id = attribute_count
  };

}}
