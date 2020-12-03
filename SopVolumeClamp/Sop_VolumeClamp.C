/*
 * Copyright (c) 2020
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 * The Star SOP
 */

#include "Sop_VolumeClamp.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide Sop_VolumeClampParms, an easy way to access parameter values from
// Sop_VolumeClampVerb::cook with the correct type.
#include "Sop_VolumeClamp.proto.h"

#include <GU/GU_Detail.h>
#include <GEO/GEO_PrimPoly.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_TemplateBuilder.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_StringHolder.h>
#include <SYS/SYS_Math.h>
#include <limits.h>

#include <GU/GU_PrimVolume.h>
using namespace HDK_Sample;

//
// Help is stored in a "wiki" style text file.  This text file should be copied
// to $HOUDINI_PATH/help/nodes/sop/star.txt
//
// See the sample_install.sh file for an example.
//

/// This is the internal name of the SOP type.
/// It isn't allowed to be the same as any other SOP's type name.
const UT_StringHolder Sop_VolumeClamp::theSOPTypeName("heightfield_clamp"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        Sop_VolumeClamp::theSOPTypeName,   // Internal name
        "Heightfield Clamp",                     // UI name
        Sop_VolumeClamp::myConstructor,    // How to build the SOP
        Sop_VolumeClamp::buildTemplates(), // My parameters
        1,                          // Min # of sources
        1,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR));        // Flag it as generator
}

/// This is a multi-line raw string specifying the parameter interface
/// for this SOP.
static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "threshold"
        label   "Threshold"
        type    float
        default { "0.05" }
        range   { 0! 1! }
    }
}
)THEDSFILE";

PRM_Template*
Sop_VolumeClamp::buildTemplates()
{
    static PRM_TemplateBuilder templ("Sop_VolumeClamp.C"_sh, theDsFile);
    return templ.templates();
}

class Sop_VolumeClampVerb : public SOP_NodeVerb
{
public:
    Sop_VolumeClampVerb() {}
    virtual ~Sop_VolumeClampVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new Sop_VolumeClampParms(); }
    virtual UT_StringHolder name() const { return Sop_VolumeClamp::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_DUPLICATE; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<Sop_VolumeClampVerb> theVerb;

    THREADED_METHOD2_CONST(Sop_VolumeClampVerb, true, bar,  UT_VoxelArrayF*, arr, float, threshold)
	void  barPartial( UT_VoxelArrayF* arr, float threshold, const UT_JobInfo &info) const;
};

void 
Sop_VolumeClampVerb::barPartial(UT_VoxelArrayF* arr, float threshold, const UT_JobInfo &info) const
{
	UT_VoxelArrayIteratorF	vit;
	UT_Interrupt		*boss = UTgetInterrupt();

	vit.setArray(arr);
	vit.setCompressOnExit(true);
	vit.setPartialRange(info.job(), info.numJobs());

	UT_VoxelProbeF		probe;
	probe.setConstArray(arr);

	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
		if (vit.isStartOfTile() && boss->opInterrupt())
			break;

		probe.setIndex(vit);
        float val = probe.getValue() > threshold ? 1 : 0;
		vit.setValue( val );
	}
}

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<Sop_VolumeClampVerb> Sop_VolumeClampVerb::theVerb;

const SOP_NodeVerb *
Sop_VolumeClamp::cookVerb() const 
{ 
    return Sop_VolumeClampVerb::theVerb.get();
}

/// This is the function that does the actual work.
void
Sop_VolumeClampVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{
    auto &&sopparms = cookparms.parms<Sop_VolumeClampParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();
    
    // Get parameter in here.
    fpreal64 threshold = sopparms.getThreshold();

    GEO_Primitive *prim = detail->findPrimitiveByName("mask");

    if(prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME )
    {
        GEO_PrimVolume* vol = (GEO_PrimVolume *) prim;

        int resx, resy, resz;

        UT_VoxelArrayWriteHandleF	handle = vol->getVoxelWriteHandle();
        UT_VoxelArrayF* voxelArr = &(*handle);

        bar(voxelArr, threshold);
    }
    
    detail->getPrimitiveList().bumpDataId();
}
