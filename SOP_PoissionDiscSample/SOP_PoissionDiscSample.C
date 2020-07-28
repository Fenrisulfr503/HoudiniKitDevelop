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

#include "SOP_PoissionDiscSample.h"

// This is an automatically generated header file based on theDsFile, below,
// to provide SOP_PoissionDiscSampleParms, an easy way to access parameter values from
// SOP_PoissionDiscSampleVerb::cook with the correct type.
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

#include <UT/UT_Options.h>
#include <GA/GA_Attribute.h>
#include <UT/UT_FastRandom.h>
#include <UT/UT_Math.h>
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
const UT_StringHolder SOP_PoissionDiscSample::theSOPTypeName("hdk_poissondisc"_sh);

/// newSopOperator is the hook that Houdini grabs from this dll
/// and invokes to register the SOP.  In this case, we add ourselves
/// to the specified operator table.
void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        SOP_PoissionDiscSample::theSOPTypeName,   // Internal name
        "PoissonDisc",                     // UI name
        SOP_PoissionDiscSample::myConstructor,    // How to build the SOP
        SOP_PoissionDiscSample::buildTemplates(), // My parameters
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
        name    "size"
        label   "Grid Size"
        type    vector
        size    2           // 2 components in a vector2
        default { "100" "100" } // Outside and inside radius defaults

    }
    parm {
        name    "mindist"      
        label   "Min Distance" 
        type    float
        default { "2" }     
        range   { 0.25! 20 }   
    }
    parm {
        name    "mybutton"
        label   "My Button"
        type    button
    }

    parm {
        name    "direct"
        label   "Direct"
        type    direction
        size    3
        default { "0" "0" "0" }
        range   { 0 1 }
    }
    
    parm {
        name    "ramp"
        label   "Ramp"
        type    rgbamask
        default { "15" }
    }
    parm {
        name    "ramprgb"
        label   "RampRgb"
        type    ramp_rgb
        default { "2" }
        range   { 1! 10 }

    }
    parm {
        name    "gridsize"
        label   "Grid Size"
        type    intvector2
        size    2
        default { "5" "5" }
        range   { 0 1000 }

    }
    parm {
        name    "sepparm"
        label   "Separator"
        type    separator
        default { "" }
    }
}
)THEDSFILE";

int func(void *data, int index, fpreal64 time,
				  const PRM_Template *tplate)
{
    std::cout << "test for button\n" ;
    std::cout << time << std::endl; 
    return 1;
}


PRM_Template*
SOP_PoissionDiscSample::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_PoissionDiscSample.C"_sh, theDsFile);
    std::cout << templ.templateLength() << std::endl;

    auto prmPtr = templ.templates();
    for(int i = 0; i < templ.templateLength(); ++i)
    {
        
        // std::cout << prmPtr->getLabel() << std::endl;
        if(UT_StringHolder(prmPtr->getLabel()) == UT_StringHolder("My Button"))
        {
            std::cout << "I got my button ui. \n";

            
            prmPtr->setCallback(PRM_Callback(func));
        }
        prmPtr++;

    }
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
    GU_Detail *detail {cookparms.gdh().gdpNC()} ;
	const GEO_Detail *const firstInput {cookparms.inputGeo(0)} ;
  
    
    const GEO_Primitive *prim;
    GA_ROHandleS attrib(firstInput->findPrimitiveAttribute("name"));
    UT_String name;
    UT_Vector3 min;
    int resx, resy, resz;
    float volMinValue = 5;
    float volMaxValue = 25;
    const GEO_PrimVolume *vol;

    GA_FOR_ALL_PRIMITIVES(firstInput, prim)
    {
        if (prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
        {
            if (attrib.isValid())
            {
                name = attrib.get(prim->getMapOffset());
            }
           
            if(name == "mask")
            {
                vol = (GEO_PrimVolume *) prim;
                vol->getRes(resx, resy, resz);
                //volMinValue = vol->calcMinimum();
                //volMaxValue = vol->calcMaximum();
                UT_Vector3 p1, p2;
                vol->indexToPos(0, 0, 0, p1);
	            vol->indexToPos(1, 0, 0, p2);
                resx *=(p1 - p2).length();
                resy *=(p1 - p2).length();
                resz *=(p1 - p2).length();

                
                UT_BoundingBox* bbox = new UT_BoundingBox{};
                vol->getBBox(bbox);
                min =  bbox->minvec();
                min[1] = 0;
                delete bbox;

            }
        }
    }

    //exint loopIndex  = sopparms.getSize().x();
    // exint height = sopparms.getSize().y();
    exint width = resx;
    exint height = resy;

    // float minDistance = sopparms.getMindist();
    float minDistance = volMaxValue;

    UT_AutoInterrupt boss("Start Cacl Poisson Dic Sampleing.");
    if (boss.wasInterrupted())
        return;
    
    //volMaxValue
    float cellSize  = volMaxValue / sqrt(2) ;

    exint gridwidth  {int(width / cellSize) + 1} ;
    exint gridheight {int(height / cellSize) + 1} ;

    if(gridheight <5 || gridwidth < 5)
    {
        return;
    }

    exint sampleCounts {36};

    // init Grid Arrays
    
    using GridArray = UT_Array<UT_Array<UT_IntArray>>;
    GridArray grid;
    grid.setSize(gridheight);
    for(auto &gridx : grid)
    {
        gridx.setSize(gridwidth);
    }

    // for(auto &gridx : grid)
    // {
    //     for(auto &item : gridx)
    //     {
    //         item = -1;
    //     }
    // }

    UT_IntArray processList{};
    UT_Array<UT_Vector2> sampleList{};
    UT_Array<float> pscaleAttrib{};
    uint initSeed1 = 1984;
    uint initSeed2 = 1995;
    UT_Vector2 firstPoint{ UTrandom(initSeed1) * width, UTrandom(initSeed2) * height };
    UT_Vector3 samplePoint3(firstPoint[0], 0, firstPoint[1]);

    float grey = vol->getValue( samplePoint3 + min);
    minDistance = volMinValue  + grey * (volMaxValue - volMinValue);

    exint SamplelistIndex = sampleList.append(firstPoint);
    processList.append(SamplelistIndex);
    pscaleAttrib.append(minDistance);

    grid[int(firstPoint[1] / cellSize)][int(firstPoint[0] / cellSize)].append(SamplelistIndex) ;
    
    //Vire First Point
    // std::cout << firstPoint << std::endl;

    // for(auto &gridx : grid)
    // {
    //     for(auto &item : gridx)
    //     {
    //         std::cout << item << " ";
    //     }
    //     std::cout << std::endl;
    // }

    auto GeneratPoint = [](UT_Vector2& point, float mindist, int id) -> UT_Vector2
    {
        uint i1 = point[0] * 1984 + 115 + id ;
        uint i2 = point[1] * 1995 + 775 + id ;
        float r1 {UTrandom(i1)};
        float r2 {UTrandom(i2)};
        float radius {mindist * (r1 + 1)} ;
        float angle  = 2 * 3.1415 * r2 ;

        float newX {point[0] + radius * cos(angle)};
        float newY {point[1] + radius * sin(angle)};
        return {newX, newY};

    };

    auto IsVaild = [ &width, &height, &gridwidth, &gridheight]( UT_Array<UT_Vector2>& sampleList, GridArray& grid, UT_Vector2& point, float mindist, float cellSize) -> bool
    {
        if(point[0] >=0 && point[0] <= width && point[1] >= 0 && point[1]<=height)
        {
            exint indexWidth  = int(point[0] / cellSize );
            exint indexHeight = int(point[1] / cellSize );

            indexWidth  = UTclamp(int(indexWidth), 3, int(gridwidth - 4)) ;
            indexHeight  = UTclamp(int(indexHeight), 3, int(gridheight - 4)) ;
            int index = -1;
            for (int h = -3; h < 4; h++)
            {
                for (int w = -3; w < 4; w++)
                {
                    
                    auto& indexArr = grid[indexHeight + h][indexWidth+w] ;
                    
                    for(auto ptindex : indexArr)
                    {
                        if(ptindex > -1)
                        {
                            if( point.distance( sampleList[ptindex] ) < mindist)
                            {
                                return false;
                            }
                        }
                    }
                    
                }
            }

            // if(index > -1) 
            // {
            //     UT_Vector2 ptTemp {sampleList[index]};
            //     //std::cout << "Index is : " << index << std::endl;
            //     std::cout << "Source Point : " << point << std::endl;
            //     std::cout << "Grid Point : " << ptTemp << std::endl;
            //     std::cout << "This Point Will Add : " << distance2d(point, ptTemp) << std::endl;
            // }        

            
            return true;
        }
        else
        {
            return false;
        }
    };


    // int idx = processList[0];
    // processList.removeLast();

    // UT_Vector2 pp {0,0};
    // std::cout << "Distance is : " << firstPoint.distance(pp);

    // std::cout << processList.size() << std::endl;

    while ( !processList.isEmpty())
    {
    // for (size_t i = 0; i < 10; i++)
    // {


    
        int precessIndex { processList[processList.size() - 1] } ;
        processList.removeLast();
        for (size_t i = 0; i < sampleCounts; i++)
        {
            UT_Vector2 point {sampleList[precessIndex]} ;
            
            UT_Vector2 newPoint {GeneratPoint(point, minDistance, i)};
            UT_Vector3 samplePoint3(newPoint[0], 0, newPoint[1]);

            float grey = vol->getValue( samplePoint3 + min);
            minDistance = volMinValue  + grey * (volMaxValue - volMinValue);

            if(IsVaild(sampleList, grid, newPoint, minDistance, cellSize))
            {
                SamplelistIndex = sampleList.append(newPoint);
                processList.append(SamplelistIndex);
                pscaleAttrib.append(minDistance);
                grid[int(newPoint[1] / cellSize)][int(newPoint[0] / cellSize)].append(SamplelistIndex) ;
            }
        }
    }


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


    for (size_t i = 0; i < pointNumbers; i++)
    {
        UT_Vector3 pos{sampleList[i][0], 0, sampleList[i][1]};

        GA_Offset offset= detail->pointOffset(i);
        detail->setPos3(offset, pos + min);
        pscaleValueHandle.set(offset, pscaleAttrib[i]);
    }
    
    detail->bumpAllDataIds();



    // Ceck for Sample List
    // for(auto & p : sampleList)
    // {
    //     std::cout << p << std::endl;
    // }

    // for (uint i = 0; i < 50; i++)
    // {  
    //     std::cout << UTrandom(i) << std::endl;
    // }
    
	// UT_ASSERT(firstInput);

	// detail->replaceWith(*firstInput);
	// GOP_Manager group_manager;
	// const GA_PointGroup *point_group = nullptr;

	// const UT_StringHolder &groupcontent = sopparms.getGroup();
	// if (groupcontent.isstring())
	// {
	// 	point_group = group_manager.parsePointGroups(groupcontent, GOP_Manager::GroupCreator(detail));
	// }

	// if (!point_group)
	// {
	// 	cookparms.sopAddWarning(SOP_MESSAGE, "Input geometry contains non-polygon primitives, so results may not be as expected");
	// }

	// GA_PointGroup *pGroup = detail->newPointGroup("__test_group__", false);

	
	// pGroup->addOffset(detail->pointOffset(GA_Index(0)));
	// pGroup->addRange(detail->getPointRange(point_group));
    
	// //Check Polygon Numbers

	// auto prim = detail->getPrimitiveRange();
	// GA_Offset primStart;
	// GA_Offset primEnd;
	// for (GA_Iterator it(detail->getPrimitiveRange()); it.blockAdvance(primStart, primEnd);)
	// {
	// 	for (GA_Offset ptoff = primStart; ptoff < primEnd; ++ptoff)
	// 	{
	// 		if (detail->getPrimitive(ptoff)->getVertexCount() != 3)
	// 		{
	// 			cookparms.sopAddError(SOP_MESSAGE, "This Gemotry must be triangles.");
	// 			return;
	// 		}
	// 	}
	// }




    // GA_RWHandleFA myAttrb2;
    
    // myAttrb2.bind(detail->addFloatArray(GA_ATTRIB_PRIMITIVE, "prims"));
    // GA_Offset prim_start;
    // GA_Offset prim_end;
    // for (GA_Iterator it(detail->getPrimitiveRange()); it.blockAdvance(prim_start, prim_end);) 
    // {
    //     for (GA_Offset ptoff = prim_start; ptoff < prim_end; ++ptoff)
    //         {
    //             auto primTive = detail->getPrimitive(ptoff);


    //             GA_Offset start;
    //             GA_Offset end;
    //             auto pointRange = primTive->getPointRange();
                
    //             UT_Fpreal32Array arr;
    //             for (GA_Iterator pit(pointRange); pit.blockAdvance(start, end);) 
    //             {
    //                 for (GA_Offset ptoff1 = start; ptoff1 < end; ++ptoff1)
    //                 {
    //                     arr.append(ptoff1);
    //                 }
    //             }
    //             myAttrb2.set(ptoff, arr);
    //         }
    // }

    // GA_RWHandleS myAttrb;
    // auto attr =  detail->addStringTuple(GA_ATTRIB_POINT, "starstarnine", 1);
    // myAttrb.bind(attr);

    // UTparallelFor(GA_SplittableRange(detail->getPointRange()), [detail, myAttrb](const GA_SplittableRange &r)
    // {
    //     GA_Offset start;
    //     GA_Offset end;
    //     for (GA_Iterator it(r); it.blockAdvance(start, end);) 
    //     {
    //         for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
    //         {
    //             auto pos = detail->getPos3(ptoff);
    //             pos[2] += 1;

    //             detail->setPos3(ptoff, pos );
    //             myAttrb.set(ptoff, "Fuck");
    //         }
    //     }
    // } 
    // );

    // attr->bumpDataId();


    // UT_StringHolder file = sopparms.getShader_file();
    // UT_Options option =  UT_Options();
    // GA_AttributeOptions attribOptions = GA_AttributeOptions();

 
    // auto attrib = detail->addAttribute("filePath", &option, &attribOptions, "string", GA_AttributeOwner::GA_ATTRIB_DETAIL);

    // auto tuple = attrib->getAIFStringTuple();
    // tuple->setString(attrib, 0, file, 0);
    
    // attrib->bumpDataId();
    // auto &&sopparms = cookparms.parms<SOP_PoissionDiscSampleParms>();
    // GU_Detail *detail = cookparms.gdh().gdpNC();

    // // We need two points per division
    // exint npoints = sopparms.getDivs()*2;

    // if (npoints < 4)
    // {
    //     // With the range restriction we have on the divisions, this
    //     // is actually impossible, (except via integer overflow),
    //     // but it shows how to add an error message or warning to the SOP.
    //     cookparms.sopAddWarning(SOP_MESSAGE, "There must be at least 2 divisions; defaulting to 2.");
    //     npoints = 4;
    // }

    // // If this SOP has cooked before and it wasn't evicted from the cache,
    // // its output detail will contain the geometry from the last cook.
    // // If it hasn't cooked, or if it was evicted from the cache,
    // // the output detail will be empty.
    // // This knowledge can save us some effort, e.g. if the number of points on
    // // this cook is the same as on the last cook, we can just move the points,
    // // (i.e. modifying P), which can also save some effort for the viewport.
    // GA_Detail
    // GA_Offset start_ptoff;
    // if (detail->getNumPoints() != npoints)ee
    // {
    //     // Either the SOP hasn't cooked, the detail was evicted from
    //     // the cache, or the number of points changed since the last cook.

    //     // This destroys everything except the empty P and topology attributes.
    //     detail->clearAndDestroy();

    //     // Build 1 closed polygon (as opposed to a curve),
    //     // namely that has its closed flag set to true,
    //     // and the right number of vertices, as a contiguous block
    //     // of vertex offsets.
    //     GA_Offset start_vtxoff;
    //     detail->appendPrimitivesAndVertices(GA_PRIMPOLY, 1, npoints, start_vtxoff, true);

    //     // Create the right number of points, as a contiguous block
    //     // of point offsets.
    //     start_ptoff = detail->appendPointBlock(npoints);

    //     // Wire the vertices to the points.
    //     for (exint i = 0; i < npoints; ++i)
    //     {
    //         detail->getTopology().wireVertexPoint(start_vtxoff+i,start_ptoff+i);
    //     }

    //     // We added points, vertices, and primitives,
    //     // so this will bump all topology attribute data IDs,
    //     // P's data ID, and the primitive list data ID.
    //     detail->bumpDataIdsForAddOrRemove(true, true, true);
    // }
    // else
    // {
    //     // Same number of points as last cook, and we know that last time,
    //     // we created a contiguous block of point offsets, so just get the
    //     // first one.
    //     start_ptoff = detail->pointOffset(GA_Index(0));

    //     // We'll only be modifying P, so we only need to bump P's data ID.
    //     detail->getP()->bumpDataId();
    // }

    // // Everything after this is just to figure out what to write to P and write it.

    // const SOP_PoissionDiscSampleParms::Orient plane = sopparms.getOrient();
    // const bool allow_negative_radius = sopparms.getNradius();

    // UT_Vector3 center = sopparms.getT();

    // int xcoord, ycoord, zcoord;
    // switch (plane)
    // {
    //     case SOP_PoissionDiscSampleParms::Orient::XY:         // XY Plane
    //         xcoord = 0;
    //         ycoord = 1;
    //         zcoord = 2;
    //         break;
    //     case SOP_PoissionDiscSampleParms::Orient::YZ:         // YZ Plane
    //         xcoord = 1;
    //         ycoord = 2;
    //         zcoord = 0;
    //         break;
    //     case SOP_PoissionDiscSampleParms::Orient::ZX:         // XZ Plane
    //         xcoord = 0;
    //         ycoord = 2;
    //         zcoord = 1;
    //         break;
    // }

    // // Start the interrupt scope
    // UT_AutoInterrupt boss("Building Star");
    // if (boss.wasInterrupted())
    //     return;

    // float tinc = M_PI*2 / (float)npoints;
    // float outer_radius = sopparms.getRad().x();
    // float inner_radius = sopparms.getRad().y();
    
    // // Now, set all the points of the polygon
    // for (exint i = 0; i < npoints; i++)
    // {
    //     // Check to see if the user has interrupted us...
    //     if (boss.wasInterrupted())
    //         break;

    //     float angle = (float)i * tinc;
    //     bool odd = (i & 1);
    //     float rad = odd ? inner_radius : outer_radius;
    //     if (!allow_negative_radius && rad < 0)
    //         rad = 0;

    //     UT_Vector3 pos(SYScos(angle)*rad, SYSsin(angle)*rad, 0);
    //     // Put the circle in the correct plane.
    //     pos = UT_Vector3(pos(xcoord), pos(ycoord), pos(zcoord));
    //     // Move the circle to be centred at the correct position.
    //     pos += center;

    //     // Since we created a contiguous block of point offsets,
    //     // we can just add i to start_ptoff to find this point offset.
    //     GA_Offset ptoff = start_ptoff + i;
    //     detail->setPos3(ptoff, pos);
    // }
}
