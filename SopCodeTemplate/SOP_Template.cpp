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

#include "SOP_Template.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide SOP_StarParms, an easy way to access parameter values from
// SOP_StarVerb::cook with the correct type.
#include "SOP_Template.proto.h"

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
const UT_StringHolder SOP_Template::theSOPTypeName("hdk_template"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_Template::theSOPTypeName,   // Internal name
        "Templaye",                     // UI name
        SOP_Template::myConstructor,    // How to build the SOP
        SOP_Template::buildTemplates(), // My parameters
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
SOP_Template::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_Template.cpp"_sh, theDsFile);
    return templ.templates();
}

class SOP_TemplateVerb : public SOP_NodeVerb
{
public:
    SOP_TemplateVerb() {}
    virtual ~SOP_TemplateVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SOP_TemplateParms(); }
    virtual UT_StringHolder name() const { return SOP_Template::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_GENERIC; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<SOP_TemplateVerb> theVerb;
};

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<SOP_TemplateVerb> SOP_TemplateVerb::theVerb;

const SOP_NodeVerb *
SOP_Template::cookVerb() const
{ 
    return SOP_TemplateVerb::theVerb.get();
}

/// This is the function that does the actual work.
void
SOP_TemplateVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SOP_TemplateParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();


}
