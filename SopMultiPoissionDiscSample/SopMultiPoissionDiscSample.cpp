
#include "SopMultiPoissionDiscSample.h"

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


    GA_ROHandleV2 sizeHandle(pointsInput->findPointAttribute("size"));

    UT_Vector2 sizeVal = sizeHandle.get(GA_Offset(0));
    std::cout << sizeVal << std::endl;


}
