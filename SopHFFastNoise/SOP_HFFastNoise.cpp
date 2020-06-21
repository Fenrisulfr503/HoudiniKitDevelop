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

#include "SOP_HFFastNoise.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide SOP_StarParms, an easy way to access parameter values from
// SOP_StarVerb::cook with the correct type.
#include "SOP_HFFastNoise.proto.h"

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

#include "FastNoiseSIMD/FastNoiseSIMD.h"
using namespace Fenrisulfr;

//
// Help is stored in a "wiki" style text file.  This text file should be copied
// to $HOUDINI_PATH/help/nodes/sop/star.txt
//
// See the sample_install.sh file for an example.
//

/// This is the internal name of the SOP type.
/// It isn't allowed to be the same as any other SOP's type name.
const UT_StringHolder SOP_HFFastNoise::theSOPTypeName("heightfield_fastnoise"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    OP_Operator* op =  new OP_Operator(
        SOP_HFFastNoise::theSOPTypeName,   // Internal name
        "FR_HFFastNoise",                     // UI name
        SOP_HFFastNoise::myConstructor,    // How to build the SOP
        SOP_HFFastNoise::buildTemplates(), // My parameters
        1,                          // Min # of sources
        1,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR);        // Flag it as generator

    op->setOpTabSubMenuPath("FRTools");
    table->addOperator(op);
}

/// This is a multi-line raw string specifying the parameter interface
/// for this SOP.
static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "layer"
        label   "Bind Layer"
        type    string
        default { "height" }
    }
    parm {
        name    "noisetype"
        label   "Noise Type"
        type    ordinal
        default { "0" }
        menu {
            "perlin"   "Perlin"
            "simple"   "Simple"
            "value"    "Value"
            "cell"     "Cell"
        }
    }
	parm {
        name    "seed"
        label   "Random Seed"
        type    int
        default { "3456" }
        range   { 1! 10000 }
    }
	parm {
        name    "frequency"
		cppname "Frequency"
        label   "Noise Frequency"
        type    float
        default { "0.01" }
        range   { 0! 1! }
		export all
    }
    parm {
        name    "position"
        label   "Origin Position"
        type    vector2
        size    2          
        default { "1" "0.3" } 

		export all
    }
	group {
		name    "file_folder"
        label   "File"
		parm {
			name  "file"
			label "Geometry File"
			type  file
		}
	}

}
)THEDSFILE";

PRM_Template*
SOP_HFFastNoise::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_HFFastNoise.cpp"_sh, theDsFile);
    return templ.templates();
}

class SOP_HFFastNoiseVerb : public SOP_NodeVerb
{
public:
    SOP_HFFastNoiseVerb() {}
    virtual ~SOP_HFFastNoiseVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SOP_HFFastNoiseParms(); }
    virtual UT_StringHolder name() const { return SOP_HFFastNoise::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_INPLACE; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<SOP_HFFastNoiseVerb> theVerb;

	THREADED_METHOD1_CONST(SOP_HFFastNoiseVerb, true, bar,  UT_VoxelArrayF*, arr)
	void  barPartial( UT_VoxelArrayF* arr,  const UT_JobInfo &info) const;


};

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<SOP_HFFastNoiseVerb> SOP_HFFastNoiseVerb::theVerb;

const SOP_NodeVerb *
SOP_HFFastNoise::cookVerb() const
{ 
    return SOP_HFFastNoiseVerb::theVerb.get();
}

void 
SOP_HFFastNoiseVerb::barPartial(UT_VoxelArrayF* arr, const UT_JobInfo &info) const
{
	UT_VoxelArrayIteratorF	vit;
	UT_Interrupt		*boss = UTgetInterrupt();

	vit.setArray(arr);

	// When we complete each tile the tile is tested to see if it can be
	// compressed, ie, is now constant.  If so, it is compressed.
	vit.setCompressOnExit(true);

	// Restrict our iterator only over part of the range.  Using the
	// info parameters means each thread gets its own subregion.
	vit.setPartialRange(info.job(), info.numJobs());

	UT_VoxelProbeF		probe;

	probe.setConstArray(arr);

	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
		if (vit.isStartOfTile() && boss->opInterrupt())
			break;
		probe.setIndex(vit);
		vit.setValue(probe.getValue() * 2);
	}
}


/// This is the function that does the actual work.
void
SOP_HFFastNoiseVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SOP_HFFastNoiseParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();

	//using Noisetype = SOP_HFFastNoiseParms::Noisetype;

	//Noisetype type = sopparms.getNoisetype();

	//if (type == Noisetype::PERLIN)
	//{
	//	std::cout << "Noise Type is :: Perlin.\n";
	//}
	//if (type == Noisetype::CELL)
	//{
	//	std::cout << "Noise Type is :: Cell.\n";
	//}
	//if (type == Noisetype::VALUE)
	//{
	//	std::cout << "Noise Type is :: Value.\n";
	//}
	//if (type == Noisetype::SIMPLE)
	//{
	//	std::cout << "Noise Type is :: Simple.\n";
	//}

	//float freq = sopparms.getFrequency();
	
	GA_ROHandleS attrib(detail->findPrimitiveAttribute("name"));
	UT_StringHolder name;
	GEO_Primitive *prim;
	
	UT_VoxelArrayF voxl;
	
	FastNoiseSIMD* myNoise = FastNoiseSIMD::NewFastNoiseSIMD();
	
	GA_FOR_ALL_PRIMITIVES(detail, prim)
	{
		if (prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
		{
			if (attrib.isValid())
			{
				name = attrib.get(prim->getMapOffset());
			}

			if (name == UT_StringHolder("height"))
			{
				//std::cout << "Voule Name : " << name << std::endl;
				GEO_PrimVolume *vol = (GEO_PrimVolume *)prim;
				int resx, resy, resz;

				// Save resolution
				vol->getRes(resx, resy, resz);

				float freq = sopparms.getFrequency();

				myNoise->SetFrequency(freq);
				float* noiseSet = myNoise->GetPerlinSet(0, 0, 0, resx, resy, 1);
				UT_VoxelArrayWriteHandleF	handle = vol->getVoxelWriteHandle();
				UT_VoxelArrayF *arr = &(*handle);
				

				//bar(arr);
				UT_VoxelArrayIteratorF	vit;
				vit.setArray(arr);
				for (vit.rewind(); !vit.atEnd(); vit.advance())
				{
					int x = vit.x();
					int y = vit.y();
					int z = vit.z();

					//std::cout << x << " " << y << " " << z << "   " << x * (y + 1) + y << std::endl;

					vit.setValue(noiseSet[x * resy + y]);
				}
				FastNoiseSIMD::FreeNoiseSet(noiseSet);
			}

			
		}
	}

	detail->bumpAllDataIds();
}
