#include "GU_PoussionDiscSampling.h"

void PoissionDiscSampl(UT_Array<UT_Vector2>& sampleList, UT_Array<float>& pscaleAttribList, GEO_PrimVolume * maskVolume, float Width, float Height, float minSampleDist, float maxSampleDist)
{
	float cellSize = maxSampleDist / sqrt(2);

	int gridWidth = int(Width / cellSize) + 1;
	int gridHeight = int(Height / cellSize) + 1;
	int sampleCounts = 30;

	if (gridWidth <= 3 || gridHeight <= 3)
	{
		return;
	}

	// initilizion GridArrays
	GridArray grid;
	grid.setSize(gridHeight);
	for (auto &gridx : grid)
	{
		gridx.setSize(gridWidth);
	}




}
