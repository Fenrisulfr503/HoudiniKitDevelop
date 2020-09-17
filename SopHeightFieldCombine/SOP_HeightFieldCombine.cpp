
#include "SOP_HeightFieldCombine.h"
#include "SOP_HeightFieldCombine.proto.h"

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
#include <UT/UT_Math.h>

using namespace Fenrisulfr;


const UT_StringHolder SOP_HeightFieldCombine::theSOPTypeName("fl_heightfield_combine"_sh);

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_HeightFieldCombine::theSOPTypeName,   // Internal name
        "SOP HeightFieldCombine",                     // UI name
        SOP_HeightFieldCombine::myConstructor,    // How to build the SOP
        SOP_HeightFieldCombine::buildTemplates(), // My parameters
        1,                          // Min # of sources
        1,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR));        // Flag it as generator
}

static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "srcname1"
        label   "Source"
        type    string
        default { "mask" }
    }
    multiparm {
        name    "folder0"
        label    "Layers"
        default 0
        parm {
            name    "layername_#"
            label   "Layer"
            type    string
            joinnext
        }
        parm {
            name    "clamp_#"
            label   "Clamp 0-1"
            type    toggle
            default { "1" }
        }
        parm {
            name    "mode_#"
            label   "Layer Mode"
            type    ordinal
            default { "subtract" }
            menu {
                "replace"   "Replace"
                "add"       "Add"
                "subtract"  "Subtract"
                "multiply"  "Multiply"
                "max"       "Maximum"
                "min"       "Minimum"
            }
        }
        parm {
            name    "ramp#"
            label   "Ramp"
            type    ramp_flt
            default { "2" }
            range   { 1! 10 }
        }
    }

}
)THEDSFILE";

PRM_Template*
SOP_HeightFieldCombine::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_HeightFieldCombine.cpp"_sh, theDsFile);
	if (templ.justBuilt())
	{
		templ.setChoiceListPtr("srcname1"_sh, &SOP_Node::namedPrimsMenu);
		templ.setChoiceListPtr("layername_#"_sh, &SOP_Node::namedPrimsMenu);
	}
    return templ.templates();
}

class SOP_HeightFieldCombineVerb : public SOP_NodeVerb
{
public:
    SOP_HeightFieldCombineVerb() {}
    virtual ~SOP_HeightFieldCombineVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SOP_HeightFieldCombineParms(); }
    virtual UT_StringHolder name() const { return SOP_HeightFieldCombine::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_INPLACE; }
    virtual void cook(const CookParms &cookparms) const;
    
    static const SOP_NodeVerb::Register<SOP_HeightFieldCombineVerb> theVerb;

	THREADED_METHOD5_CONST(SOP_HeightFieldCombineVerb, true, bar, UT_VoxelArrayF*, src, UT_VoxelArrayF*, dst, 
		 SOP_HeightFieldCombineParms::Mode_, mode, bool, useFit, UT_SharedPtr<UT_Ramp>, ramp)
	void  barPartial(UT_VoxelArrayF* src, UT_VoxelArrayF* dst, SOP_HeightFieldCombineParms::Mode_ mode,
		bool useFit, UT_SharedPtr<UT_Ramp> ramp, const UT_JobInfo &info )const;
};


const SOP_NodeVerb::Register<SOP_HeightFieldCombineVerb> SOP_HeightFieldCombineVerb::theVerb;

const SOP_NodeVerb *
SOP_HeightFieldCombine::cookVerb() const
{ 
    return SOP_HeightFieldCombineVerb::theVerb.get();
}


float ReplaveOp(float a, float b)
{
	return b;
}

float AddOp(float a, float b)
{
	return a + b;
}

float SubOp(float a, float b)
{
	return a - b;
}

float MulOp(float a, float b)
{
	return a * b;
}

float MaxOp(float a, float b)
{
	return SYSmax(a, b);
}

float MinOp(float a, float b)
{
	return SYSmin(a, b);
}

float IsFit01(float a)
{
	return SYSfit01(a, 0, 1);
}

float NotFit01(float a)
{
	return a;
}

void  SOP_HeightFieldCombineVerb::barPartial(UT_VoxelArrayF* src, UT_VoxelArrayF* dst, SOP_HeightFieldCombineParms::Mode_ mode,
	bool useFit, UT_SharedPtr<UT_Ramp> ramp, const UT_JobInfo &info)const
{
	UT_VoxelArrayIteratorF	vit;
	UT_Interrupt		*boss = UTgetInterrupt();

	vit.setArray(dst);
	vit.setCompressOnExit(true);
	vit.setPartialRange(info.job(), info.numJobs());

	UT_VoxelProbeF		probe;
	probe.setConstArray(src);

	UT_Functor2<float, float, float> functor = ReplaveOp;

	switch (mode)
	{
	case SOP_HeightFieldCombineParms::Mode_::ADD:
	{
		functor = AddOp;
		break;
	}
	case SOP_HeightFieldCombineParms::Mode_::SUBTRACT:
	{
		functor = SubOp;
		break;
	}
	case SOP_HeightFieldCombineParms::Mode_::MULTIPLY:
	{
		functor = MulOp;
		break;
	}
	case SOP_HeightFieldCombineParms::Mode_::MIN:
	{
		functor = MinOp;
		break;
	}
	case SOP_HeightFieldCombineParms::Mode_::MAX:
	{
		functor = MaxOp;
		break;
	}
	case SOP_HeightFieldCombineParms::Mode_::REPLACE:
	{
		functor = ReplaveOp;
		break;
	}
	default:
		break;
	}
	UT_Functor1<float, float> fitFunctor = NotFit01;

	if (useFit)
		fitFunctor = IsFit01;

	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
		if (vit.isStartOfTile() && boss->opInterrupt())
			break;

		probe.setIndex(vit);
		float srcValue = fitFunctor(probe.getValue());
		float result[4];
		ramp->rampLookup(srcValue, result);
		
		// Result must fit range to 0 - 1
		vit.setValue( SYSfit01( functor(vit.getValue(), result[0]) , 0, 1) );
	}
}


void
SOP_HeightFieldCombineVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    auto &&sopparms = cookparms.parms<SOP_HeightFieldCombineParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();
	
	GEO_Primitive* maskPrim = detail->findPrimitiveByName(sopparms.getSrcname1());
	GEO_PrimVolume *maskHeightField;
	if (maskPrim == nullptr)
	{
		cookparms.sopAddWarning(SOP_MESSAGE, "You must have a mask heightfield!");
		return ;
	}

	if (maskPrim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
	{
		maskHeightField = (GEO_PrimVolume *)maskPrim;
	}
	else
	{
		return;
	}
	UT_VoxelArrayWriteHandleF	maskHandle = maskHeightField->getVoxelWriteHandle();
	UT_VoxelArrayF *dst = &(*maskHandle);
	for (auto &i : sopparms.getFolder0())
	{
		// find heightfield primitives
		GEO_Primitive* prim = detail->findPrimitiveByName(i.layername_);
		GEO_PrimVolume *heightfield;
		if (prim == nullptr)
		{
			continue;
		}

		if (prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
		{
			heightfield = (GEO_PrimVolume *)prim;
			UT_VoxelArrayWriteHandleF	heightFieldHandle = heightfield->getVoxelWriteHandle();
			UT_VoxelArrayF *src = &(*heightFieldHandle);
			SOP_HeightFieldCombineParms::Mode_ myMode = static_cast<SOP_HeightFieldCombineParms::Mode_>(i.mode_);
			bar(src, dst, myMode, i.clamp_, i.ramp);
		}
	}

	detail->getPrimitiveList().bumpDataId();
}
