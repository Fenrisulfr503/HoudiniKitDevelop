#include "GU_PoissionDiscSampling.h"

#include "UT/UT_Math.h"
#include "GEO/GEO_PrimVolume.h"


void PoissionDiscSample(UT_Array<SampleData>& sampleList, 
	GEO_PrimVolume* maskVolume,
	float Width, float Height, float minSampleDist, float maxSampleDist ,
	float randSeed, UT_Vector3& offset)
{
    float cellSize  = maxSampleDist / sqrt(2) ;

    exint gridwidth  {int(Width / cellSize) + 1} ;
    exint gridheight {int(Height / cellSize) + 1} ;

    if(gridheight < 3 || gridwidth < 3)
    {
        return;
    }

	exint sampleCounts {36};
 
    GridArray grid;
    grid.setSize(gridheight);
    for(auto &gridx : grid)
    {
        gridx.setSize(gridwidth);
    }

	UT_IntArray processList{};
    processList.setCapacity(100);
	uint initSeed1 = 1984 + randSeed;
    uint initSeed2 = 1995 + randSeed;
    UT_Vector2 firstPoint{ UTrandom(initSeed1) * Width, UTrandom(initSeed2) * Height };
    UT_Vector3 samplePoint3(firstPoint[0], 0, firstPoint[1]);

	float grey = maskVolume->getValue( samplePoint3 + offset);	
	float minDistance;

	minDistance = SYSfit(grey, 0, 1, maxSampleDist, minSampleDist );
	SampleData data { firstPoint,  minDistance};

    exint SamplelistIndex = sampleList.append(data);
    processList.append(SamplelistIndex);

    grid[int(firstPoint[1] / cellSize)][int(firstPoint[0] / cellSize)].append(SamplelistIndex) ;

	auto GeneratPoint = [](UT_Vector2& point, float mindist, int id) -> UT_Vector2
    {
        uint i1 = point[0] * 1984 + 115 + id ;
        uint i2 = point[1] * 2885 + 775 + id ;
        float r1 { UTfastRandom(i1) };
        float r2 { UTfastRandom(i2) };

        float radius {mindist * (r1 + 1) * 0.65f} ;
        float angle  = 2 * 3.1415 * r2 ;

        float newX {point[0] + radius * cos(angle)};
        float newY {point[1] + radius * sin(angle)};

        return {newX, newY};

    };

    auto IsVaild = [ &Width, &Height, &gridwidth, &gridheight]( UT_Array<SampleData>& sampleList, GridArray& grid, UT_Vector2& point, float mindist, float cellSize) -> bool
    {
        if(point[0] > 0 && point[0] < Width && point[1] > 0 && point[1] < Height)
        {
            exint indexWidth  = int(point[0] / cellSize );
            exint indexHeight = int(point[1] / cellSize );

            indexWidth  = UTclamp(int(indexWidth), 1, int(gridwidth - 2)) ;
            indexHeight  = UTclamp(int(indexHeight), 1, int(gridheight - 2)) ;
            int index = -1;
            for (int h = -1; h < 2; h++)
            {
                for (int w = -1; w < 2; w++)
                {
                    auto& indexArr = grid[indexHeight + h][indexWidth+w] ;
                    
                    for(auto ptindex : indexArr)
                    {
						if( point.distance( sampleList[ptindex].position ) < (0.75 * (mindist * 0.5 + sampleList[ptindex].scale * 0.5 )) )
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

        int precessIndex { processList[processList.size() - 1] } ;
        processList.removeLast();
		UT_Vector2 newPoint;
        for (size_t i = 0; i < sampleCounts; i++)
        {

            newPoint = GeneratPoint(sampleList[precessIndex].position, sampleList[precessIndex].scale, i);
            UT_Vector3 samplePoint3(newPoint[0], 0, newPoint[1]);
            float grey = maskVolume->getValue( samplePoint3 + offset);
			
			minDistance = SYSfit(grey, 0, 1, maxSampleDist, minSampleDist);

            if(IsVaild(sampleList, grid, newPoint, minDistance, cellSize))
            {
				data.position = newPoint;
				data.scale = minDistance;

                SamplelistIndex = sampleList.append(data);
                processList.append(SamplelistIndex);
                grid[int(newPoint[1] / cellSize)][int(newPoint[0] / cellSize)].append(SamplelistIndex) ;
            }
        }
    }

}





    



    
    
    

    
    


