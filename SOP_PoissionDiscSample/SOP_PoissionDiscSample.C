
#include "SOP_PoissionDiscSample.h"
#include "GU_PoissionDiscSampling.h"
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

const UT_StringHolder SOP_PoissionDiscSample::theSOPTypeName("sop_poissondisc"_sh);

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_PoissionDiscSample::theSOPTypeName,   // Internal name
        "SOP PoissonDisc",                     // UI name
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
        type    float_minmax
        size    2
        default { "1" "2" }
        range   { 0 10 }
    }
    parm {
        name    "rand_seed"
        label   "Random Seed"
        type    integer
        default { "5876" }
        range   { 0 100000 }
    }
}
)THEDSFILE";

PRM_Template*
SOP_PoissionDiscSample::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_PoissionDiscSample.C"_sh, theDsFile);

    auto prmPtr = templ.templates();
    
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
	const GEO_Detail* firstInput  = cookparms.inputGeo(0);
    
    UT_Vector3 volumePrimMinPosition;
    exint randSeed = sopparms.getRand_seed();
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

    // Keep volValue can,t to samll
    const float limitValue = 0.02;
    int ContainerSize = (width / volMinValue) * (height / volMinValue);

    if(volMinValue < limitValue)
        volMinValue = limitValue;
    if(volMaxValue < limitValue)
        volMaxValue = limitValue;

    UT_AutoInterrupt boss("Start Cacl Poisson Dic Sampleing.");
    if (boss.wasInterrupted())
        return;

    
    UT_Array<SampleData> sampleList;
    sampleList.setCapacity(ContainerSize);

    PoissionDiscSample(sampleList, 
	vol,
	width, height, volMinValue, volMaxValue ,
	randSeed, volumePrimMinPosition);


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
    detail->getP()->bumpDataId();

    UTparallelFor(GA_SplittableRange(detail->getPointRange()), [&detail, &pscaleValueHandle, &sampleList,  heightVol, &volumePrimMinPosition, firstInput, &volDataArr, &pointPosition](const GA_SplittableRange &r)
    {
        GA_Offset start;
        GA_Offset end;
        for (GA_Iterator it(r); it.blockAdvance(start, end);) 
        {
            for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
            {
                // Set Height Value to P
                UT_Vector3 pos{sampleList[ptoff].position[0], 0, sampleList[ptoff].position[1]};
				
                pos += volumePrimMinPosition;
                pos[1] = heightVol->getValue(pos);
				pos[1] += pointPosition[1];

				UT_Vector3 samplePosition{ pos };
				samplePosition[1] = pointPosition[1];

                detail->setPos3(ptoff, pos );
                pscaleValueHandle.set(ptoff, sampleList[ptoff].scale);

                GA_RWHandleF volValueHandle;
                for(auto &volData : volDataArr)
                {
                    volValueHandle.bind(volData.volumeAttribute);

                    float val = volData.primVol->getValue(samplePosition);
                    volValueHandle.set(ptoff, val);
                }

            }
        }
    } 
    );

}
