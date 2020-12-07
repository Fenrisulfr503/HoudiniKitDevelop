#include "GU_PoissionDiscSampling.h"

#include "UT/UT_Math.h"
#include "GEO/GEO_PrimVolume.h"

struct GridTable
{
	UT_Array<UT_IntArray> data;
	exint myWidth;
	exint myHeight;

	GridTable(exint w, exint h) : data(w*h, w*h), myWidth(w), myHeight(h)
	{
		
	}

	UT_IntArray& getByIndex(exint w, exint h)
	{
		exint idx = myWidth * h + w - 1 ;
		return data[idx];
	}

};


void PoissionDiscSample(UT_Array<SampleData>& sampleList, 
	GEO_PrimVolume* maskVolume,
	float Width, float Height, float minSampleDist, float maxSampleDist ,float densityMultiplu,
	float randSeed, UT_Vector3& offset)
{
    float cellSize  = maxSampleDist * densityMultiplu / sqrt(2) ;

    exint gridwidth  {exint(Width / cellSize) + 1} ;
    exint gridheight {exint(Height / cellSize) + 1} ;

    if(gridheight < 3 || gridwidth < 3)
    {
        return;
    }

	exint sampleCounts {36};
 
    GridTable grid(gridwidth, gridheight);


	UT_IntArray processList{};
    processList.setCapacity(1000);
	uint initSeed1 = 2165161 + randSeed;
    uint initSeed2 = 161461615 + randSeed;
    UT_Vector2 firstPoint{ SYSfastRandom(initSeed1) * Width, SYSfastRandom(initSeed2) * Height };
    UT_Vector3 samplePoint3(firstPoint[0], 0, firstPoint[1]);

	float grey = maskVolume->getValue( samplePoint3 + offset);	
	float minDistance;
	
	minDistance = SYSfit(grey, 0, 1, maxSampleDist, minSampleDist ) * densityMultiplu;
	SampleData data { firstPoint,  minDistance};

    exint SamplelistIndex = sampleList.append(data);
    processList.append(SamplelistIndex);

	grid.getByIndex(exint(firstPoint[0] / cellSize), exint(firstPoint[1] / cellSize)).append(SamplelistIndex) ;

	auto GeneratPoint = [](UT_Vector2& point, float mindist, int id) -> UT_Vector2
    {
		UT_Vector2 randomPoint2 = point;
		randomPoint2.x() += 153.5 + id;
		randomPoint2.y() += 514648.87478 + id;

		uint seed1 = SYSvector_hash(randomPoint2.data(), 2);

		randomPoint2.x() += 216161.471 + id;
		randomPoint2.y() += 16894148.87484 + id;
		uint seed2 = SYSvector_hash(randomPoint2.data(), 2);
        float r1 { SYSfastRandom(seed1) };
        float r2 { SYSfastRandom(seed2) };

        float radius {mindist * (r1 + 1) } ;
		constexpr fpreal32 pi = 2 * 3.1415;
        float angle  = pi * r2 ;

        float newX {point[0] + radius * cos(angle)};
        float newY {point[1] + radius * sin(angle)};

        return {newX, newY};

    };

    auto IsVaild = [ &Width, &Height, &gridwidth, &gridheight]( UT_Array<SampleData>& sampleList, GridTable& grid, UT_Vector2& point, float mindist, float cellSize) -> bool
    {
        if(point[0] > 0 && point[0] < Width && point[1] > 0 && point[1] < Height)
        {
            exint indexWidth  = exint(point[0] / cellSize );
            exint indexHeight = exint(point[1] / cellSize );

            indexWidth  = UTclamp(int(indexWidth), 1, int(gridwidth - 2)) ;
            indexHeight  = UTclamp(int(indexHeight), 1, int(gridheight - 2)) ;
            exint index = -1;
            for (exint h = -1; h < 2; h++)
            {
                for (exint w = -1; w < 2; w++)
                {                    
                    for(auto ptindex : grid.getByIndex(indexWidth+w, indexHeight + h))
                    {
						if( point.distance( sampleList[ptindex].position ) <  (mindist * 0.5 + sampleList[ptindex].scale * 0.5 ) )
						{
							return false;
						}
                    }
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    };

	while ( !processList.isEmpty())
    {

        exint precessIndex { processList[processList.size() - 1] } ;
        processList.removeLast();
		UT_Vector2 newPoint;
        for (size_t i = 0; i < sampleCounts; i++)
        {

            newPoint = GeneratPoint(sampleList[precessIndex].position, sampleList[precessIndex].scale, i);
            UT_Vector3 samplePoint3(newPoint[0], 0, newPoint[1]);
            float grey = maskVolume->getValue( samplePoint3 + offset);
			
			minDistance = SYSfit(grey, 0, 1, maxSampleDist, minSampleDist) * densityMultiplu;
            if(IsVaild(sampleList, grid, newPoint, minDistance, cellSize))
            {
				data.position = newPoint;
				data.scale = minDistance;

                SamplelistIndex = sampleList.append(data);
                processList.append(SamplelistIndex);
				grid.getByIndex(exint(newPoint[0] / cellSize), exint(newPoint[1] / cellSize)).append(SamplelistIndex);
            }
        }
    }

}





    



    
    
    

    
    


