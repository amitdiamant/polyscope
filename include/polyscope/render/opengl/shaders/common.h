// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// This file defines common POLYSCOPE_GLSL constants and routines used by
// multiple shaders; it is combined at link time with all fragment
// shaders compiled via the methods in the GLProgram class.

#include "polyscope/render/opengl/gl_engine.h"

namespace polyscope {
namespace render {

// clang-format off

const char* shaderCommonSource = R"(
#version 410

const vec3 RGB_TEAL     = vec3(0., 178./255., 178./255.);
const vec3 RGB_BLUE     = vec3(150./255., 154./255., 255./255.);
const vec3 RGB_SKYBLUE  = vec3(152./255., 158./255., 200./255.);
const vec3 RGB_ORANGE   = vec3(1., 0.45, 0.);
const vec3 RGB_BLACK    = vec3(0., 0., 0.);
const vec3 RGB_WHITE    = vec3(1., 1., 1.);
const vec3 RGB_RED      = vec3(0.8, 0., 0.);
const vec3 RGB_DARKGRAY = vec3( .2, .2, .2 );
const vec3 RGB_DARKRED  = vec3( .2, .0, .0 );

float orenNayarDiffuse(
  vec3 lightDirection,
  vec3 viewDirection,
  vec3 surfaceNormal,
  float roughness,
  float albedo) {
  
  float LdotV = dot(lightDirection, viewDirection);
  float NdotL = dot(lightDirection, surfaceNormal);
  float NdotV = dot(surfaceNormal, viewDirection);

  float s = LdotV - NdotL * NdotV;
  float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

  float sigma2 = roughness * roughness;
  float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
  float B = 0.45 * sigma2 / (sigma2 + 0.09);

  return albedo * max(0.0, NdotL) * (A + B * s / t) / 3.14159;
}


float specular( vec3 N, vec3 L, vec3 E, float shininess ) {
   vec3 R = 2.*dot(L,N)*N - L;
   return pow( max( 0., dot( R, E )), shininess );
}

float fresnel( vec3 N, vec3 E ) {
   const float sharpness = 10.;
   float NE = max( 0., dot( N, E ));
   return pow( sqrt( 1. - NE*NE ), sharpness );
}

float luminance(vec3 v) {
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}


vec3 gammaCorrect( vec3 colorLinear )
{
   const float screenGamma = 2.2;
   return pow(colorLinear, vec3(1.0/screenGamma));
}

vec3 undoGammaCorrect( vec3 colorLinear )
{
   const float screenGamma = 2.2;
   return pow(colorLinear, vec3(screenGamma));
}
      

vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k) {
  normal = normalize(normal);
  normal.y = -normal.y;
	normal *= 0.98; // pull slightly inward, to reduce sampling artifacts near edges
  vec2 matUV = normal.xy/2.0 + vec2(.5, .5);
  
  vec3 mat_r = texture(t_mat_r, matUV).rgb;
  vec3 mat_g = texture(t_mat_g, matUV).rgb;
  vec3 mat_b = texture(t_mat_b, matUV).rgb;
  vec3 mat_k = texture(t_mat_k, matUV).rgb;
	vec3 colorCombined = color.r * mat_r + color.g * mat_g + color.b * mat_b + 
											 (1. - color.r - color.g - color.b) * mat_k;

  return colorCombined;
}

vec2 sphericalTexCoords(vec3 v) {
  const vec2 invMap = vec2(0.1591, 0.3183);
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invMap;
  uv += 0.5;
  return uv;
}

// Two useful references:
//   - https://stackoverflow.com/questions/38938498/how-do-i-convert-gl-fragcoord-to-a-world-space-point-in-a-fragment-shader
//   - https://stackoverflow.com/questions/11277501/how-to-recover-view-space-position-given-view-space-depth-value-and-ndc-xy

vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord) {
	vec4 ndcPos;
	ndcPos.xy = ((2.0 * fragCoord.xy) - (2.0 * viewport.xy)) / (viewport.zw) - 1;
	ndcPos.z = (2.0 * fragCoord.z - depthRange.x - depthRange.y) / (depthRange.y - depthRange.x);
	ndcPos.w = 1.0;

	vec4 clipPos = ndcPos / fragCoord.w;
	vec4 eyePos = invProjMat * clipPos;
	return eyePos.xyz / eyePos.w;
}

float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint) {
	vec4 clipPos = projMat * vec4(viewPoint, 1.); // only actually need one element of this result, could save work
	float z_ndc = clipPos.z / clipPos.w;
	float depth = (((depthRange.y-depthRange.x) * z_ndc) + depthRange.x + depthRange.y) / 2.0;
	return depth;
}

void raySphereIntersection(vec3 rayStart, vec3 rayDir, vec3 sphereCenter, float sphereRad, out float tHit, out vec3 pHit, out vec3 nHit) {
		rayDir = normalize(rayDir);
		vec3 o = rayStart - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(o, rayDir);
    float c = dot(o,o) - sphereRad*sphereRad;
    float disc = b*b - 4*a*c;
    if(disc < 0){
				tHit = -1.;
				pHit = vec3(777, 777, 777);
				nHit = vec3(777, 777, 777);
    } else {
			tHit = (-b - sqrt(disc)) / (2.0*a);
			pHit = rayStart + tHit * rayDir;
			nHit = normalize(pHit - sphereCenter);
		}
}


)";

// clang-format on

} // namespace render
} // namespace polyscope
