
#include "SopMultiPoissionDiscSample.h"

#include "GU_PoissionDiscSampling.h"
#include "SopMultiPoissionDiscSample.proto.h"

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

#include <UT/UT_Options.h>
#include <GA/GA_Attribute.h>
#include <GEO/GEO_PrimVolume.h>

using namespace Fenrisulfr;


const UT_StringHolder SopMultiPoissionDiscSample::theSOPTypeName("sop_multipoissiondisc"_sh);

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SopMultiPoissionDiscSample::theSOPTypeName,   // Internal name
        "SOP MultiPoissionDisc",                     // UI name
        SopMultiPoissionDiscSample::myConstructor,    // How to build the SOP
        SopMultiPoissionDiscSample::buildTemplates(), // My parameters
        2,                          // Min # of sources
        2,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR));        // Flag it as generator
}


static const char *theDsFile = R"THEDSFILE(
{
    name        parameters

}
)THEDSFILE";

PRM_Template*
SopMultiPoissionDiscSample::buildTemplates()
{
    static PRM_TemplateBuilder templ("SopMultiPoissionDiscSample.cpp"_sh, theDsFile);
    return templ.templates();
}

class SopMultiPoissionDiscSampleVerb : public SOP_NodeVerb
{
public:
    SopMultiPoissionDiscSampleVerb() {}
    virtual ~SopMultiPoissionDiscSampleVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SopMultiPoissionDiscSampleParms(); }
    virtual UT_StringHolder name() const { return SopMultiPoissionDiscSample::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_GENERIC; }

    virtual void cook(const CookParms &cookparms) const;
    

    static const SOP_NodeVerb::Register<SopMultiPoissionDiscSampleVerb> theVerb;
};


const SOP_NodeVerb::Register<SopMultiPoissionDiscSampleVerb> SopMultiPoissionDiscSampleVerb::theVerb;

const SOP_NodeVerb *
SopMultiPoissionDiscSample::cookVerb() const
{ 
    return SopMultiPoissionDiscSampleVerb::theVerb.get();
}

void
SopMultiPoissionDiscSampleVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{
    auto &&sopparms = cookparms.parms<SopMultiPoissionDiscSampleParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();
    const GU_Detail* heightfieldInput = cookparms.inputGeo(0);
    const GU_Detail* pointsInput = cookparms.inputGeo(1);

    
    const GEO_Primitive* maskPrim{ heightfieldInput->findPrimitiveByName("mask") };
    if (maskPrim == nullptr)
	{
		return;
	}
    GEO_PrimVolume* vol;
    if (maskPrim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
	{
		vol = ( GEO_PrimVolume *)maskPrim;
    }

    struct ParallelData
    {
        UT_Array<SampleData> sampleArray;
        UT_Vector3 offset;
        exint pointNumber;
    };


    if(pointsInput->getNumPoints())
    {
        UT_Array<ParallelData> ParallelSampleList{};

        ParallelSampleList.setSize(pointsInput->getNumPoints());
        

        
        UTparallelFor(GA_SplittableRange(pointsInput->getPointRange()), [&detail, &pointsInput, &vol, &ParallelSampleList](const GA_SplittableRange &r)
        {
            GA_Offset start;
            GA_Offset end;
            for (GA_Iterator it(r); it.blockAdvance(start, end);) 
            {
                for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
                {
                    UT_Vector3 origin = pointsInput->getPos3(ptoff);
                    exint pointNumber = pointsInput->pointIndex(ptoff);

                    GA_ROHandleV2 sizeHandle(pointsInput->findPointAttribute("size"));
                    UT_Vector2 sizeVal{};
                    UT_Vector2 scaleRangeVal{};
                    if(sizeHandle.isValid())
                    {
                        sizeVal = sizeHandle.get(ptoff);
                    }

                    GA_ROHandleV2 scaleRangeHandle(pointsInput->findPointAttribute("scaleRange"));
                    if(scaleRangeHandle.isValid())
                    {
                        scaleRangeVal = scaleRangeHandle.get(ptoff);
                    }

                    origin[0] -= sizeVal.x() * 0.5;
                    origin[2] -= sizeVal.y() * 0.5;

                    ParallelSampleList[pointNumber].sampleArray.setCapacity(500);
                    ParallelSampleList[pointNumber].offset = origin;
                    ParallelSampleList[pointNumber].pointNumber = pointNumber;

                    PoissionDiscSample(ParallelSampleList[pointNumber].sampleArray, 
                        vol,
                        sizeVal.x(), sizeVal.y(), scaleRangeVal.x(), scaleRangeVal.y() ,
                        ptoff + 12.45, origin);
                }
            }
        } 
        );

        // Merge all elements
        UT_Array<SampleData> totalSampleList{};

        //totalSampleList.setCapacity(counts);
        for(int i = 0; i < ParallelSampleList.size(); i++)
        {
            totalSampleList.concat(ParallelSampleList[i].sampleArray);
        }

        size_t counts = totalSampleList.size();

        GA_Attribute* pscaleAttrib;
            
        if(totalSampleList.size() != detail->getNumPoints())
        {
            detail->clearAndDestroy();
            detail->appendPointBlock(totalSampleList.size());
            pscaleAttrib = detail->addFloatTuple(GA_ATTRIB_POINT, "pscale", 1); 
            detail->bumpDataIdsForAddOrRemove(true, true, true);
        }
        else
        {
            detail->getP()->bumpDataId();
            pscaleAttrib = detail->findPointAttribute("pscale");
        }

        if(totalSampleList.size())
        {
            
            UTparallelFor(GA_SplittableRange(detail->getPointRange()), [&detail, &totalSampleList](const GA_SplittableRange &r)
            {
                GA_Offset start;
                GA_Offset end;
                for (GA_Iterator it(r); it.blockAdvance(start, end);) 
                {
                    for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
                    {
                        UT_Vector3 pos {totalSampleList[ptoff].position.x() + totalSampleList[ptoff].offset.x(), 
                                0, totalSampleList[ptoff].position.y() + totalSampleList[ptoff].offset.z()};
                        detail->setPos3(ptoff, pos);

                    }
                }
            } 
            );
        }

    }




}
