
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
#include <UT/UT_Quaternion.h>

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
		UT_Vector2 sizeRange;
        exint pointNumber;
		UT_Vector4 orient;
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
					UT_Vector4 orient{};
                    if(sizeHandle.isValid())
                    {
                        sizeVal = sizeHandle.get(ptoff);
                    }

                    GA_ROHandleV2 scaleRangeHandle(pointsInput->findPointAttribute("scaleRange"));
                    if(scaleRangeHandle.isValid())
                    {
                        scaleRangeVal = scaleRangeHandle.get(ptoff);
                    }

					GA_ROHandleV4 orientHandle(pointsInput->findPointAttribute("orient"));
					if (orientHandle.isValid())
					{
						orient = orientHandle.get(ptoff);
					}

                    ParallelSampleList[pointNumber].sampleArray.setCapacity(500);
                    ParallelSampleList[pointNumber].offset = origin;
					ParallelSampleList[pointNumber].sizeRange = sizeVal;
                    ParallelSampleList[pointNumber].pointNumber = pointNumber;
					ParallelSampleList[pointNumber].orient = orient;

					UT_Vector3 move{ origin };
					move[0] -= sizeVal.x() * 0.5;
					move[2] -= sizeVal.y() * 0.5;

                    PoissionDiscSample(ParallelSampleList[pointNumber].sampleArray, 
                        vol,
                        sizeVal.x(), sizeVal.y(), scaleRangeVal.x(), scaleRangeVal.y() ,
                        ptoff + 12.45, move);
                }
            }
        } 
        );

        // Merge all elements
		struct TotalSampleList
		{
			UT_Array<SampleData> dataArray;
			UT_IntArray indexArray;
		};

		TotalSampleList totalSampleList{};
		
        //totalSampleList.setCapacity(counts);
        for(int i = 0; i < ParallelSampleList.size(); i++)
        {
            totalSampleList.dataArray.concat(ParallelSampleList[i].sampleArray);
			totalSampleList.indexArray.appendMultiple(ParallelSampleList[i].pointNumber, 
									ParallelSampleList[i].sampleArray.size());
        }

        size_t counts = totalSampleList.dataArray.size();
       
        if(counts)
        {
			GA_Attribute* pscaleAttrib;
			if (counts != detail->getNumPoints())
			{
				detail->clearAndDestroy();
				detail->appendPointBlock(counts);
				pscaleAttrib = detail->addFloatTuple(GA_ATTRIB_POINT, "pscale", 1);

				detail->bumpDataIdsForAddOrRemove(true, true, true);
			}
			else
			{
				detail->getP()->bumpDataId();
				pscaleAttrib = detail->findPointAttribute("pscale");
			}

			struct VolumeData
			{
				UT_StringHolder name;
				GEO_PrimVolume* primVol;
				GA_Attribute* attributeHandle;
			};

			using VolumeDataArray = UT_Array<VolumeData>;
			VolumeDataArray volDataArr;

			auto primRange = heightfieldInput->getPrimitiveRange();
			GA_Offset primStart;
			GA_Offset primEnd;
			GA_ROHandleS attribName(heightfieldInput->findPrimitiveAttribute("name"));
			for (GA_Iterator it(primRange); it.blockAdvance(primStart, primEnd);)
			{
				for (GA_Offset primPtoff = primStart; primPtoff < primEnd; ++primPtoff)
				{
					GEO_Primitive* prim = (GEO_Primitive*)heightfieldInput->getPrimitive(primPtoff);

					if (prim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
					{
						VolumeData volData;
						volData.primVol = (GEO_PrimVolume *)prim;
						volData.name = attribName.get(primPtoff);
						//std::cout << volData.name << std::endl;
						GA_Attribute* attrib = detail->findFloatTuple(GA_ATTRIB_POINT, volData.name);
						GA_RWHandleF attribHandle{ attrib };
						if (!attribHandle.isValid())
						{
							//sstd::cout << "Create Attrib." << std::endl;
							attrib = detail->addFloatTuple(GA_ATTRIB_POINT, volData.name, 1);
						}
						
						volData.attributeHandle = attrib;
						volDataArr.append(volData);
					}
				}
			}
			
			

            UTparallelFor(GA_SplittableRange(detail->getPointRange()), [&detail, &pscaleAttrib, &totalSampleList, &ParallelSampleList, &volDataArr](const GA_SplittableRange &r)
            {
                GA_Offset start;
                GA_Offset end;
                for (GA_Iterator it(r); it.blockAdvance(start, end);) 
                {
                    for (GA_Offset ptoff = start; ptoff < end; ++ptoff)
                    {
                        UT_Vector3 pos {totalSampleList.dataArray[ptoff].position.x() ,
                                0, totalSampleList.dataArray[ptoff].position.y() };

						pos[0] -= ParallelSampleList[totalSampleList.indexArray[ptoff]].sizeRange[0] * 0.5;
						pos[2] -= ParallelSampleList[totalSampleList.indexArray[ptoff]].sizeRange[1] * 0.5;

						UT_Quaternion quat{ ParallelSampleList[totalSampleList.indexArray[ptoff]].orient };
						pos = quat.rotate(pos);

						pos[0] += ParallelSampleList[totalSampleList.indexArray[ptoff]].offset.x();
						pos[2] += ParallelSampleList[totalSampleList.indexArray[ptoff]].offset.z();

						

						//Sample Volume value to points.
						float heightVal = 0.0;
						UT_Vector3 samplePosition{ pos };
						samplePosition[1] = 0;
						UT_StringHolder heightName{ "height" };
						for (auto& i : volDataArr)
						{
							float val = i.primVol->getValue(samplePosition);
							GA_RWHandleF attribHandle{ i.attributeHandle };
							if (i.name == heightName)
							{
								heightVal = val;
							}
							else
							{
								attribHandle.set(ptoff, val);
							}
							
						}

						GA_RWHandleF scaleHandle{ pscaleAttrib };
						scaleHandle.set(ptoff, totalSampleList.dataArray[ptoff].scale);

						pos[1] += heightVal;
                        detail->setPos3(ptoff, pos);
                    }
                }
            } 
            );
        }

    }




}
