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

#include "SopReadJsonToGeoTable.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide SOP_StarParms, an easy way to access parameter values from
// SOP_StarVerb::cook with the correct type.
#include "SopReadJsonToGeoTable.proto.h"

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

#include <UT/UT_JSONValue.h>
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
const UT_StringHolder SopReadJsonToGeoTable::theSOPTypeName("sop_readjsontogeo"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SopReadJsonToGeoTable::theSOPTypeName,   // Internal name
        "Sop ReadJsonToGeoTable",                     // UI name
        SopReadJsonToGeoTable::myConstructor,    // How to build the SOP
        SopReadJsonToGeoTable::buildTemplates(), // My parameters
        0,                          // Min # of sources
        0,                          // Max # of sources
        nullptr,                    // Custom local variables (none)
        OP_FLAG_GENERATOR));        // Flag it as generator
}

/// This is a multi-line raw string specifying the parameter interface
/// for this SOP.
static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "json_path"
        label   "Json Path"
        type    file
        default { "" }
        parmtag { "filechooser_pattern" "*.json" }
    }
}
)THEDSFILE";

PRM_Template*
SopReadJsonToGeoTable::buildTemplates()
{
    static PRM_TemplateBuilder templ("SopReadJsonToGeoTable.cpp"_sh, theDsFile);
    return templ.templates();
}

class SopReadJsonToGeoTableVerb : public SOP_NodeVerb
{
public:
    SopReadJsonToGeoTableVerb() {}
    virtual ~SopReadJsonToGeoTableVerb() {}

    virtual SOP_NodeParms *allocParms() const { return new SopReadJsonToGeoTableParms(); }
    virtual UT_StringHolder name() const { return SopReadJsonToGeoTable::theSOPTypeName; }

    virtual CookMode cookMode(const SOP_NodeParms *parms) const { return COOK_GENERIC; }

    virtual void cook(const CookParms &cookparms) const;
    
    /// This static data member automatically registers
    /// this verb class at library load time.
    static const SOP_NodeVerb::Register<SopReadJsonToGeoTableVerb> theVerb;
};

// The static member variable definition has to be outside the class definition.
// The declaration is inside the class.
const SOP_NodeVerb::Register<SopReadJsonToGeoTableVerb> SopReadJsonToGeoTableVerb::theVerb;

const SOP_NodeVerb *
SopReadJsonToGeoTable::cookVerb() const
{ 
    return SopReadJsonToGeoTableVerb::theVerb.get();
}


void ByTypeAddAttributes(GU_Detail *detail, GA_Attribute* pointAttributePtr, 
					const UT_JSONValue* iterValuePtr, const UT_StringHolder& jsonKey,
					UT_Map<UT_StringHolder, GA_Attribute*>& attributeTable)
{

	switch (iterValuePtr->getType())
	{
	case UT_JSONValue::Type::JSON_STRING:
	{
		pointAttributePtr = detail->addStringTuple(GA_ATTRIB_POINT, jsonKey, 1);
		std::cout << "Value : " << iterValuePtr->getS() << "\n";
		break;
	}
	case UT_JSONValue::Type::JSON_INT:
	{
		pointAttributePtr = detail->addFloatTuple(GA_ATTRIB_POINT, jsonKey, 1);
		std::cout << "Value : " << iterValuePtr->getI() << "\n";
		break;
	}
	case UT_JSONValue::Type::JSON_REAL:
	{
		pointAttributePtr = detail->addFloatTuple(GA_ATTRIB_POINT, jsonKey, 1);
		std::cout << "Value : " << iterValuePtr->getF() << "\n";
		break;
	}
	case UT_JSONValue::Type::JSON_ARRAY:
	{
		break;
	}
	case UT_JSONValue::Type::JSON_MAP:
	{

		UT_JSONValueMap* myJsonMapPtr = iterValuePtr->getMap();
		if (myJsonMapPtr->entries() == 3)
		{
			pointAttributePtr = detail->addFloatTuple(GA_ATTRIB_POINT, jsonKey, 3);
		}
		if (myJsonMapPtr->entries() == 2)
		{
			pointAttributePtr = detail->addFloatTuple(GA_ATTRIB_POINT, jsonKey, 2);
		}


		//std::cout << "Value : " << iterValuePtr->getF() << "\n";
		break;
	}
	default:
		break;
	}

	if (pointAttributePtr)
	{
		attributeTable.emplace(jsonKey, pointAttributePtr);
	}
}

/// This is the function that does the actual work.
void
SopReadJsonToGeoTableVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SopReadJsonToGeoTableParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();

	UT_StringHolder filepath = sopparms.getJson_path();

	GA_RWHandleF  floatValueHandle;
	GA_RWHandleV3 vector3ValueHandle;
	GA_RWHandleS  stringValueHandle;


	UT_Map<UT_StringHolder, GA_Attribute*> attributeTable;
	
	GA_Attribute* pointAttributePtr = nullptr;


	// initilization json data struct.
	UT_JSONValue value;
	if (!value.loadFromFile(filepath))
	{
		cookparms.sopAddError(SOP_MESSAGE, "Json file open failed.");
		return;
	}

	// 1, Parse Value Type
	UT_JSONValue*		iterValuePtr;
	UT_JSONValueArray*  jsonArrayPtr;
	UT_JSONValueMap*	jsonMapPtr;
	UT_StringArray      jsonArray;
	jsonArrayPtr = value.getArray();

	exint idx = 0;


	iterValuePtr = jsonArrayPtr->get(idx);
	jsonMapPtr = iterValuePtr->getMap();

	
	jsonMapPtr->getKeys(jsonArray);
	for (UT_StringHolder& jsonKey : jsonArray)
	{
		//UT_StringHolder& jsonKey = jsonArray[0];
		iterValuePtr = jsonMapPtr->get(jsonKey);
		std::cout << "Key   : " << jsonKey << "\n";
		ByTypeAddAttributes(detail, pointAttributePtr, iterValuePtr, jsonKey, attributeTable);
		if (jsonKey == "Asset")
		{
			UT_JSONValueMap* assetMap = iterValuePtr->getMap();
		}
	}

	for (const auto& p : attributeTable) {
		std::cout << p.first << ";" << std::endl;
	}
	
}
