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

#include "SOP_Triangulation.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide SOP_StarParms, an easy way to access parameter values from
// SOP_StarVerb::cook with the correct type.
#include "SOP_Triangulation.proto.h"

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


using namespace Fenrisulfr;

//
// Help is stored in a "wiki" style text file.  This text file should be copied
// to $HOUDINI_PATH/help/nodes/sop/star.txt
//
// See the sample_install.sh file for an example.
//

/// This is the internal name of the SOP type.
/// It isn't allowed to be the same as any other SOP's type name.
const UT_StringHolder SOP_Triangulation::theSOPTypeName("hdk_triangulation"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_Triangulation::theSOPTypeName,   // Internal name
        "Triangulation",                     // UI name
        SOP_Triangulation::myConstructor,    // How to build the SOP
        SOP_Triangulation::buildTemplates(), // My parameters
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

}
)THEDSFILE";

PRM_Template*
SOP_Triangulation::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_Triangulation.cpp"_sh, theDsFile);
    return templ.templates();
}

class SOP_TriangulationVerb : public SOP_NodeVerb
{
public:
    SOP_TriangulationVerb() {}
    virtual ~SOP_TriangulationVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SOP_TriangulationParms(); }
    virtual UT_StringHolder name() const { return SOP_Triangulation::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_GENERIC; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<SOP_TriangulationVerb> theVerb;
};

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<SOP_TriangulationVerb> SOP_TriangulationVerb::theVerb;

const SOP_NodeVerb *
SOP_Triangulation::cookVerb() const
{ 
    return SOP_TriangulationVerb::theVerb.get();
}

/// This is the function that does the actual work.
void
SOP_TriangulationVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SOP_TriangulationParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();
	const GU_Detail *input = cookparms.inputGeo(0);

	detail->clearAndDestroy();

	//display message
	std::cout << "Plugin Triangultion. " << std::endl;
	std::cout << "Mesh Primtives : " << input->getNumPrimitives() << std::endl;
	std::cout << "Mesh Points : " << input->getNumPoints() << std::endl;

	// append mesh construct
	detail->appendPointBlock(input->getNumPoints());
	//detail->appendPrimitiveBlock(GA_PRIMPOLY, input->getNumPrimitives());
	detail->bumpDataIdsForAddOrRemove(true, true, true);

	// set point like inputs
	GA_SplittableRange splitPointRange(input->getPointRange());
	UTparallelFor(GA_SplittableRange(splitPointRange), [detail, input](const GA_SplittableRange &r) {
		GA_Offset pointStart, pointEnd;
		for (GA_Iterator it(r); it.blockAdvance(pointStart, pointEnd);)
		{
			for (GA_Offset pointOffset = pointStart; pointOffset < pointEnd; ++pointOffset)
			{
				UT_Vector3 pos = input->getPos3(pointOffset);
				GA_Index index = input->pointIndex(pointOffset);
				GA_Offset detailPointOffset = detail->pointOffset(index);
				detail->setPos3(detailPointOffset, pos);
			}
		}
	});
	


	GA_SplittableRange splitPrimRange(input->getPrimitiveRange());
	UTparallelFor(GA_SplittableRange(splitPrimRange), [detail, input](const GA_SplittableRange &r) {
		GA_Offset primStart, primEnd;
		for (GA_Iterator it(r); it.blockAdvance(primStart, primEnd);)
		{
			for (GA_Offset primOffset = primStart; primOffset < primEnd; ++primOffset)
			{
				const GA_Primitive* primPtr = input->getPrimitive(primOffset);
				GA_Size vertexCount = primPtr->getVertexCount();
				if (vertexCount > 3)
				{
					//std::cout << "Index " << input->primitiveIndex(primOffset) << " Primtive Vertex Counts > 3." << std::endl;


					for (exint i = 0; i < vertexCount - 2; ++i)
					{
						GEO_PrimPoly *prim_poly_ptr = (GEO_PrimPoly *)detail->appendPrimitive(GA_PRIMPOLY);

						GA_Offset ptoffStart = primPtr->getPointOffset(0);
						GA_Offset ptoffStart1 = primPtr->getPointOffset(i + 1);
						GA_Offset ptoffStart2 = primPtr->getPointOffset(i + 2);


						prim_poly_ptr->appendVertex(ptoffStart1);
						prim_poly_ptr->appendVertex(ptoffStart2);
						prim_poly_ptr->appendVertex(ptoffStart);

						prim_poly_ptr->close();
					}
				}
				else
				{
					GEO_PrimPoly *prim_poly_ptr = (GEO_PrimPoly *)detail->appendPrimitive(GA_PRIMPOLY);
					for (exint i = 0; i < vertexCount; ++i)
					{
						GA_Offset ptoff = primPtr->getPointOffset(i);
						prim_poly_ptr->appendVertex(ptoff);
					}
					prim_poly_ptr->close();

				}
			}
		}
	});



	// Copy Source
	
}
