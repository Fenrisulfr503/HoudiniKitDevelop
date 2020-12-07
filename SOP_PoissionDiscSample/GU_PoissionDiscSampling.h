#pragma once

#include <UT/UT_IntArray.h>
#include <UT/UT_Vector3.h>


class GEO_PrimVolume;
//using GridArray = UT_Array<UT_IntArray>;
class GridTable;



struct SampleData
{
	UT_Vector2 position;
	float scale;
};


void PoissionDiscSample(UT_Array<SampleData>& sampleList, 
	GEO_PrimVolume* maskVolume,
	float Width, float Height, float minSampleDist, float maxSampleDist , float densityMultiplu,
	float randSeed, UT_Vector3& offset);