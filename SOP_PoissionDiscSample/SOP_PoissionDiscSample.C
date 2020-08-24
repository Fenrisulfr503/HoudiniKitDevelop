
#include "SOP_PoissionDiscSample.h"

#include "SOP_PoissionDiscSample.proto.h"

#include <GA/GA_SplittableRange.h>
#include <GU/GU_Detail.h>
#include <GEO/GEO_PrimPoly.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_TemplateBuilder.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_StringHolder.h>
#include <UT/UT_ParallelUtil.h>
#include <SYS/SYS_Math.h>
#include <limits.h>

#include <UT/UT_Quaternion.h>
#include <UT/UT_Options.h>
#include <GA/GA_Attribute.h>
#include <UT/UT_FastRandom.h>
#include <UT/UT_Math.h>
#include <GU/GU_PrimVolume.h>
using namespace HDK_Sample;

const UT_StringHolder SOP_PoissionDiscSample::theSOPTypeName("hdk_poissondisc"_sh);

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_PoissionDiscSample::theSOPTypeName,   // Internal name
        "PoissonDisc",                     // UI name
        SOP_PoissionDiscSample::myConstructor,    // How to build the SOP
        SOP_PoissionDiscSample::buildTemplates(), // My parameters
        1,                          // Min # of sources
        1,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR));        // Flag it as generator
}

static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "scale_range"
        label   "Scale Range"
        type    vector
        size    2           // 2 components in a vector2
        default { "5" "10" } // Outside and inside radius defaults

    }
}
)THEDSFILE";

int func(void *data, int index, fpreal64 time,
				  const PRM_Template *tplate)
{
    std::cout << "test for button\n" ;
    std::cout << time << std::endl; 
    return 1;
}


PRM_Template*
SOP_PoissionDiscSample::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_PoissionDiscSample.C"_sh, theDsFile);
    // std::cout << templ.templateLength() << std::endl;

    auto prmPtr = templ.templates();
    
    // for(int i = 0; i < templ.templateLength(); ++i)
    // {
        
    //     // std::cout << prmPtr->getLabel() << std::endl;
    //     if(UT_StringHolder(prmPtr->getLabel()) == UT_StringHolder("My Button"))
    //     {
    //         std::cout << "I got my button ui. \n";

            
    //         prmPtr->setCallback(PRM_Callback(func));
    //     }
    //     prmPtr++;

    // }
    return templ.templates();
}

class SOP_PoissionDiscSampleVerb : public SOP_NodeVerb
{
public:
    SOP_PoissionDiscSampleVerb() {}
    virtual ~SOP_PoissionDiscSampleVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SOP_PoissionDiscSampleParms(); }
    virtual UT_StringHolder name() const { return SOP_PoissionDiscSample::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_GENERATOR; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<SOP_PoissionDiscSampleVerb> theVerb;
};

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<SOP_PoissionDiscSampleVerb> SOP_PoissionDiscSampleVerb::theVerb;

const SOP_NodeVerb *
SOP_PoissionDiscSample::cookVerb() const 
{ 
    return SOP_PoissionDiscSampleVerb::theVerb.get();
}

/// This is the function that does the actual work.
void
SOP_PoissionDiscSampleVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms {cookparms.parms<SOP_PoissionDiscSampleParms>()} ;
    GU_Detail *detail = cookparms.gdh().gdpNC() ;
	const GEO_Detail* firstInput  = cookparms.inputGeo(0)  ;
  
    UT_Vector3 volumePrimMinPosition;
	exint width, height;
    float volMinValue = sopparms.getScale_range().x();
    float volMaxValue = sopparms.getScale_range().y();
    GEO_PrimVolume *vol;
    GEO_PrimVolume *heightVol;
	const GEO_Primitive* maskPrim{ firstInput->findPrimitiveByName("mask") };
	const GEO_Primitive* heightPrim{ firstInput->findPrimitiveByName("height") };

	if (maskPrim == nullptr || heightPrim  == nullptr)
	{
		return;
	}
	
    if (heightPrim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
	{
        heightVol = ( GEO_PrimVolume *)heightPrim;
    }
	if (maskPrim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
	{

		vol = ( GEO_PrimVolume *)maskPrim;
		int resx, resy, resz;
		UT_Vector3 p1, p2;
		vol->getRes(resx, resy, resz);
		vol->indexToPos(0, 0, 0, p1);
		vol->indexToPos(1, 0, 0, p2);
		float voxelLength = (p1 - p2).length();

		width = (resx-1) * voxelLength; height = (resy-1) * voxelLength;
		volumePrimMinPosition = firstInput->getPos3(vol->getPointOffset(0));
		volumePrimMinPosition[0] -= width * 0.5;
		volumePrimMinPosition[2] -= height * 0.5;
	}

    float minDistance = volMaxValue;

    UT_AutoInterrupt boss("Start Cacl Poisson Dic Sampleing.");
    if (boss.wasInterrupted())
        return;
  
    float cellSize  = volMaxValue / sqrt(2) ;

    exint gridwidth  {int(width / cellSize) + 1} ;
    exint gridheight {int(height / cellSize) + 1} ;

    if(gridheight < 5 || gridwidth < 5)
    {
        return;
    }

    exint sampleCounts {36};
    
    using GridArray = UT_Array<UT_Array<UT_IntArray>>;
    GridArray grid;
    grid.setSize(gridheight);
    for(auto &gridx : grid)
    {
        gridx.setSize(gridwidth);
    }

    UT_IntArray processList{};
	UT_IntArray useIndexsList{};
    UT_Array<UT_Vector2> sampleList{};
    UT_Array<float> pscaleAttrib{};
	
    uint initSeed1 = 1984;
    uint initSeed2 = 1995;
    UT_Vector2 firstPoint{ UTrandom(initSeed1) * width, UTrandom(initSeed2) * height };
    UT_Vector3 samplePoint3(firstPoint[0], 0, firstPoint[1]);

    float grey = vol->getValue( samplePoint3 + volumePrimMinPosition);	
	minDistance = SYSfit(grey, 0, 1, volMinValue, volMaxValue);

    exint SamplelistIndex = sampleList.append(firstPoint);
    processList.append(SamplelistIndex);
    pscaleAttrib.append(minDistance);

    grid[int(firstPoint[1] / cellSize)][int(firstPoint[0] / cellSize)].append(SamplelistIndex) ;
    
    auto GeneratPoint = [](UT_Vector2& point, float mindist, int id) -> UT_Vector2
    {
        uint i1 = point[0] * 1984 + 115 + id ;
        uint i2 = point[1] * 1995 + 775 + id ;
        float r1 {UTrandom(i1)};
        float r2 {UTrandom(i2)};
        float radius {mindist * (r1 + 1)} ;
        float angle  = 2 * 3.1415 * r2 ;

        float newX {point[0] + radius * cos(angle)};
        float newY {point[1] + radius * sin(angle)};
        return {newX, newY};

    };

    auto IsVaild = [ &width, &height, &gridwidth, &gridheight]( UT_Array<UT_Vector2>& sampleList, GridArray& grid, UT_Vector2& point, float mindist, float cellSize) -> bool
    {
        if(point[0] >0.01 && point[0] < width-0.01 && point[1] > 0.01 && point[1]<height-0.01)
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
                        if(ptindex > -1)
                        {
                            if( point.distance( sampleList[ptindex] ) < mindist)
                            {
                                return false;
                            }
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
        for (size_t i = 0; i < sampleCounts; i++)
        {
            UT_Vector2 point {sampleList[precessIndex]} ;
            
            UT_Vector2 newPoint {GeneratPoint(point, minDistance, i)};
            UT_Vector3 samplePoint3(newPoint[0], 0, newPoint[1]);

            float grey = vol->getValue( samplePoint3 + volumePrimMinPosition);
        
			minDistance = SYSfit(grey, 0, 1, volMinValue, volMaxValue);

//            float tempDistance = minDistance;
//             if(minDistance < pscaleAttrib[precessIndex])
//                 tempDistance = pscaleAttrib[precessIndex];

            if(IsVaild(sampleList, grid, newPoint, minDistance, cellSize))
            {
                SamplelistIndex = sampleList.append(newPoint);
                processList.append(SamplelistIndex);
				pscaleAttrib.append(minDistance);

                grid[int(newPoint[1] / cellSize)][int(newPoint[0] / cellSize)].append(SamplelistIndex) ;
            }
        }
    }


    exint pointNumbers { sampleList.size()};
    GA_RWHandleF pscaleValueHandle;
    GA_Attribute* pointDistanceAttr; 
                
    if(detail->getNumPoints() != pointNumbers)
    {
        detail->clearAndDestroy();
        detail->appendPointBlock(pointNumbers);
        pointDistanceAttr = detail->addFloatTuple(GA_ATTRIB_POINT, "pscale", 1); 
        detail->bumpDataIdsForAddOrRemove(true, true, true);

    }else
    {
        detail->getP()->bumpDataId();
        pointDistanceAttr = detail->findPointAttribute("pscale");
    }

    pscaleValueHandle.bind(pointDistanceAttr);

    struct VolumeData
    {
        UT_StringHolder name;
        GEO_PrimVolume* primVol;
        GA_Attribute* volumeAttribute;
    };
    
    using VolumeDataArray = UT_Array<VolumeData> ;
    VolumeDataArray volDataArr;
    //Set for all volume value

    auto primRange = firstInput->getPrimitiveRange();
    GA_Offset primStart;
    GA_Offset primEnd;
    GA_ROHandleS attribName(firstInput->findPrimitiveAttribute("name"));
    for (GA_Iterator it(primRange); it.blockAdvance(primStart, primEnd);) 
    {
        for (GA_Offset primPtoff = primStart; primPtoff < primEnd; ++primPtoff)
        {
            GEO_Primitive* prim = (GEO_Primitive*)firstInput->getPrimitiveByIndex(primPtoff);

            if(prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
            {
                VolumeData volData;
                volData.primVol = ( GEO_PrimVolume *)prim;
                volData.name = attribName.get(prim->getMapOffset());
                volData.volumeAttribute = detail->addFloatTuple(GA_ATTRIB_POINT, volData.name, 1); 
                volDataArr.append(volData);

            }
        }
    }
	
	UT_Vector3 pointPosition = firstInput->getPos3(GA_Offset(0));

    UTparallelFor(GA_SplittableRange(detail->getPointRange()), [&detail, &pscaleValueHandle, &sampleList, &pscaleAttrib, heightVol, &volumePrimMinPosition, firstInput, &volDataArr, &pointPosition](const GA_SplittableRange &r)
    {
        GA_Offset start;
        GA_Offset end;
        for (GA_Iterator it(r); it.blockAdvance(start, end);) 
        {
            for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
            {
                // Set Height Value to P
                UT_Vector3 pos{sampleList[ptoff][0], 0, sampleList[ptoff][1]};
                pos += volumePrimMinPosition;
                pos[1] = heightVol->getValue(pos);
				pos[1] += pointPosition[1];
                detail->setPos3(ptoff, pos );
                pscaleValueHandle.set(ptoff, pscaleAttrib[ptoff]);

                GA_RWHandleF volValueHandle;
                for(auto &volData : volDataArr)
                {
                    volValueHandle.bind(volData.volumeAttribute);

                    float val = volData.primVol->getValue(pos);
                    volValueHandle.set(ptoff, val);
                }

            }
        }
    } 
    );

}
