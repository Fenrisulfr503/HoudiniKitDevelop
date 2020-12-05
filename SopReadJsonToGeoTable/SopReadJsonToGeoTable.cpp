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


/// This is the function that does the actual work.
void
SopReadJsonToGeoTableVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SopReadJsonToGeoTableParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();

	UT_StringHolder filepath = sopparms.getJson_path();
	UT_JSONValue parseFileValue;
	if (!parseFileValue.loadFromFile(filepath))
	{
		cookparms.sopAddError(SOP_MESSAGE, "Json file open failed.");
		return;
	}

// Step 1 : Gather all data int tables.
	UT_Map<UT_StringHolder, UT_Array<UT_JSONValue*>> myAssetTable;
	UT_Array<UT_StringHolder> myAssetStaticMeshInfo;

	UT_JSONValueArray* myAssetTableArray = parseFileValue.getArray();
	for (exint i = 0; i < myAssetTableArray->entries(); i++)
	{
		UT_JSONValueMap* myAssetTableMap = myAssetTableArray->get(i)->getMap();
		UT_StringArray myAssetTableKeys;
		myAssetTableMap->getKeyReferences(myAssetTableKeys);

		for (UT_StringHolder& myAssetTableKey : myAssetTableKeys)
		{
			if (myAssetTableKey == "Asset")
			{
				UT_JSONValueMap* myAssetMap = myAssetTableMap->get(myAssetTableKey)->getMap();

				UT_StringArray myAssetKeys;
				myAssetMap->getKeyReferences(myAssetKeys);
				for (UT_StringHolder& myAssetKey : myAssetKeys)
				{
					UT_JSONValueMap* myAssetAttribMap = myAssetMap->get(myAssetKey)->getMap();

					UT_StringArray myAssetAttribKeys;
					myAssetAttribMap->getKeyReferences(myAssetAttribKeys);
					for (UT_StringHolder& myAssetAttribKey : myAssetAttribKeys)
					{
						myAssetTable[myAssetAttribKey].append(myAssetAttribMap->get(myAssetAttribKey));
					}
					for (UT_StringHolder& tempAssetTableKey : myAssetTableKeys)
					{
						if (tempAssetTableKey == "Asset")
						{
							myAssetStaticMeshInfo.append(myAssetKey);
						}
						else
						{
							myAssetTable[tempAssetTableKey].append(myAssetTableMap->get(tempAssetTableKey));
						}
					}
				}
			}
		}
	}

// Step 2 : create Point and Attribute for geometry.
	exint assetNumbers = myAssetStaticMeshInfo.entries();

	GA_Offset start_ptoff;
	if (detail->getNumPoints() != assetNumbers)
	{
		detail->clearAndDestroy();
		start_ptoff = detail->appendPointBlock(assetNumbers);
		detail->bumpDataIdsForAddOrRemove(true, true, true);
	}
	else
	{
		start_ptoff = detail->pointOffset(GA_Index(0));
		detail->getP()->bumpDataId();
	}

	//
	UT_AutoInterrupt boss("ReBuilding JSON Object");
	if (boss.wasInterrupted())
		return;

	for (auto& myPairs : myAssetTable)
	{
		if (myPairs.second.entries())
		{
			switch (myPairs.second[0]->getType())
			{
			case UT_JSONValue::Type::JSON_STRING:
			{
				GA_RWHandleS myStringAttribHandle (detail->addStringTuple(GA_ATTRIB_POINT, myPairs.first, 1));

				for (exint i = 0; i < myPairs.second.entries(); ++i)
				{
					myStringAttribHandle.set(start_ptoff + i, myPairs.second[i]->getS());
				}
				break;
			}
			case UT_JSONValue::Type::JSON_INT:
			{
				GA_RWHandleF myStringAttribHandle(detail->addFloatTuple(GA_ATTRIB_POINT, myPairs.first, 1));
				for (exint i = 0; i < myPairs.second.entries(); ++i)
				{
					myStringAttribHandle.set(start_ptoff + i, static_cast<float>(myPairs.second[i]->getI()));
				}
				break;
			}
			case UT_JSONValue::Type::JSON_REAL:
			{
				GA_RWHandleF myStringAttribHandle(detail->addFloatTuple(GA_ATTRIB_POINT, myPairs.first, 1));
				for (exint i = 0; i < myPairs.second.entries(); ++i)
				{
					myStringAttribHandle.set(start_ptoff + i, myPairs.second[i]->getF());
				}
				break;
			}
			case UT_JSONValue::Type::JSON_MAP:
			{
				if (myPairs.second[0]->getMap()->entries() == 2)
				{
					GA_RWHandleV2 myStringAttribHandle(detail->addFloatTuple(GA_ATTRIB_POINT, myPairs.first, 2));
					for (exint i = 0; i < myPairs.second.entries(); ++i)
					{
						UT_Vector2 val(myPairs.second[i]->getMap()->get("X")->getF(), 
							myPairs.second[i]->getMap()->get("Y")->getF());
						myStringAttribHandle.set(start_ptoff + i, val);
					}

				}
				else if (myPairs.second[0]->getMap()->entries() == 3)
				{
					GA_RWHandleV3 myStringAttribHandle(detail->addFloatTuple(GA_ATTRIB_POINT, myPairs.first, 3));
					for (exint i = 0; i < myPairs.second.entries(); ++i)
					{
						UT_Vector3 val(myPairs.second[i]->getMap()->get("X")->getF(),
							myPairs.second[i]->getMap()->get("Y")->getF(),
							myPairs.second[i]->getMap()->get("Z")->getF());
						myStringAttribHandle.set(start_ptoff + i, val);
					}
				}
				else
				{
				}
				break;
			}
			case UT_JSONValue::Type::JSON_ARRAY:
			{
				break;
			}
			default:
				break;
			}
		}

	}

	// add for Asset Attribute.
	detail->addStringTuple(GA_ATTRIB_POINT, "Asset", 1);
	GA_RWHandleS assetAttributeHandle(detail->findPointAttribute("Asset"));

	for (exint i = 0; i < myAssetStaticMeshInfo.entries(); ++i)
	{
		assetAttributeHandle.set(start_ptoff + i, myAssetStaticMeshInfo[i]);
	}

}
