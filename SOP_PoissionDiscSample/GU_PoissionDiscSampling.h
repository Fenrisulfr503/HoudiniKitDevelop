#pragma once

#include <UT/UT_IntArray.h>
#include <GEO/GEO_PrimVolume.h>

using GridArray = UT_Array<UT_Array<UT_IntArray>>;


void PoissionDiscSampl(UT_Array<UT_Vector2>& sampleList, 
	UT_Array<float>& pscaleAttribList,
	GEO_PrimVolume* maskVolume,
	float Width, float Height, float minSampleDist, float maxSampleDist );