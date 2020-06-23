

#include "SOP_HFFastNoise.h"


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


const UT_StringHolder SOP_HFFastNoise::theSOPTypeName("heightfield_fastnoise"_sh);


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


static const char *theDsFile = R"THEDSFILE(
{
    name        parameters
    parm {
        name    "layer"
        label   "Bind Layer"
        type    string
        default { "height" }
    }
	groupsimple {
		name	"general"
		label	"General"
		parm {
			name    "noisetype"
			label   "Noise Type"
			type    ordinal
			default { "1" }
			menu {        
				"value"    "Value"
				"perlin"   "Perlin"
				"simple"   "Simple"
				"cubic"    "Cubic"
				"cellular" "Cellular"
				}
        }
		parm {
			name    "noiseseed"
			cppname "NoiseSeed"
			label   "Noise Seed"
			type    integer
			default { "1337" }
			range   { 0! 10000000 }
		}
		parm {
			name    "noisefrequency"
			cppname "Frequency"
			label   "Frequency"
			type    float
			default { "0.01" }
			range   { 0! 1 }
		}
	}
	groupsimple {
		name	"fractal"
		label	"Fractal"
		disablewhen "{ noisetype == cellular }"
		parm {
			name    "fractaltype"
			cppname "Fractaltype"
			label   "Type"
			type    ordinal
			default { "1" }
			menu {        
				"none"			"None"
				"fbm"			"FBM"
				"billow"		"Billow"
				"rigidmulti"    "Rigid Multi"
				}
        }
		parm {
			name    "octaves"
			cppname "Octaves"
			label   "Octaves"
			type    integer
			default { "3" }
			range   { 0! 10 }
			disablewhen "{ fractaltype == none }"
		}
		parm {
			name    "lacunarity"
			cppname "Lacunarity"
			label   "Lacunarity"
			type    float
			default { "2.0" }
			range   { 1! 5 }
			disablewhen "{ fractaltype == none }"
		}
		parm {
			name    "gain"
			cppname "Gain"
			label   "Gain"
			type    float
			default { "0.5" }
			range   { 0! 1! }
			disablewhen "{ fractaltype == none }"
		}
	}
	groupsimple {
		name	"cellularp"
		label	"Cellular"
		hidewhen "{ noisetype != cellular }"
		parm {
			name	"return"
			label   "Return Method"
			type     ordinal
			default { "1" }
			menu {        
				"cell"    "Cell Value"
				"distance"   "Distance"
				"distancet"   "Distance2"
				"distancetadd"    "Distance2 Add"
				"distancetsub" "Distance2 Sub"
				"distancetmul" "Distance2 Mul"
				"distancetdiv" "Distance2 Div"
				"noiselookup"  "Noise Lookup"
				"distancetcave" "Distance2 Cave"
				}
		}
		parm {
			name	"distancefunction"
			label   "Distance Function"
			type     ordinal
			default { "0" }
			menu {        
				"euclidean"    "Euclidean"
				"manhattan"	   "Manhattan"
				"natural"      "Natural"
				}
		}
		parm {
			name    "distancetindicies"
			label   "DistanceIndicies"
			type    integer
			default { "1" }
			range   { 0! 3 }
		}
		parm {
			name    "jitter"
			label   "Jitter"
			type    float
			default { "0.5" }
			range   { 0! 1 }
		}
		parm {
			name	"noiselookuptype"
			label   "Noise Lookup Type"
			type     ordinal
			default { "0" }
			menu {        
				"value"    "Value"
				"simplex"	"Simplex"
				"perlin"    "Perlin"
				}
		}
		parm {
			name    "noiselookupfreq"
			label   "Noise Lookup Freq"
			type    float
			default { "0.5" }
			range   { 0! 1! }
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

    static const SOP_NodeVerb::Register<SOP_HFFastNoiseVerb> theVerb;

	THREADED_METHOD1_CONST(SOP_HFFastNoiseVerb, true, bar,  UT_VoxelArrayF*, arr)
	void  barPartial( UT_VoxelArrayF* arr,  const UT_JobInfo &info) const;


};


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

	vit.setCompressOnExit(true);


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



void
SOP_HFFastNoiseVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SOP_HFFastNoiseParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();

	// Noise Setups
	SOP_HFFastNoiseParms::Noisetype noisetype =  sopparms.getNoisetype();
	int64 noiseseed = sopparms.getNoiseSeed();
	fpreal64 noisefreq = sopparms.getFrequency();

	SOP_HFFastNoiseParms::Fractaltype fractaltype = sopparms.getFractaltype();
	int64 fractaloctaves = sopparms.getOctaves();
	fpreal64 fractallacunarity = sopparms.getLacunarity();
	fpreal64 fractalgain = sopparms.getGain();

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

				//float freq = sopparms.getFrequency();

				myNoise->SetFrequency(noisefreq);
				myNoise->SetSeed(noiseseed);

				// Set Fractal Parm
				
				switch (fractaltype)
				{
					case SOP_HFFastNoiseParms::Fractaltype::FBM :
					{
						myNoise->SetFractalType(FastNoiseSIMD::FBM);
						break;
					}
					case SOP_HFFastNoiseParms::Fractaltype::BILLOW:
					{
						myNoise->SetFractalType(FastNoiseSIMD::Billow);
						break;
					}
					case SOP_HFFastNoiseParms::Fractaltype::RIGIDMULTI:
					{
						myNoise->SetFractalType(FastNoiseSIMD::RigidMulti);
						break;
					}

					default:
						break;
				}

				myNoise->SetFractalOctaves(fractaloctaves);
				myNoise->SetFractalLacunarity(fractallacunarity);
				myNoise->SetFractalGain(fractalgain);

				
				float* noiseSet;



				

				switch (noisetype)
				{
					case SOP_HFFastNoiseParms::Noisetype::VALUE : 
					{
						if(fractaltype != SOP_HFFastNoiseParms::Fractaltype::NONE)
							noiseSet = myNoise->GetValueFractalSet(0, 0, 0, resx, resy, 1);
						else
							noiseSet = myNoise->GetValueSet(0, 0, 0, resx, resy, 1);

						break;
					}

					case SOP_HFFastNoiseParms::Noisetype::PERLIN :
					{
						if (fractaltype != SOP_HFFastNoiseParms::Fractaltype::NONE)
							noiseSet = myNoise->GetPerlinFractalSet(0, 0, 0, resx, resy, 1);
						else
							noiseSet = myNoise->GetPerlinSet(0, 0, 0, resx, resy, 1);
						break;
					}

					case SOP_HFFastNoiseParms::Noisetype::SIMPLE :
					{
						if (fractaltype != SOP_HFFastNoiseParms::Fractaltype::NONE)
							noiseSet = myNoise->GetSimplexFractalSet(0, 0, 0, resx, resy, 1);
						else
							noiseSet = myNoise->GetSimplexSet(0, 0, 0, resx, resy, 1);

						break;
					}

					case SOP_HFFastNoiseParms::Noisetype::CUBIC :
					{
						if (fractaltype != SOP_HFFastNoiseParms::Fractaltype::NONE)
							noiseSet = myNoise->GetCubicFractalSet(0, 0, 0, resx, resy, 1);
						else
							noiseSet = myNoise->GetCubicSet(0, 0, 0, resx, resy, 1);

						break;
					}

					case SOP_HFFastNoiseParms::Noisetype::CELLULAR:
					{
						noiseSet = myNoise->GetCellularSet(0, 0, 0, resx, resy, 1);
						break;
					}

					default:
						break;
				}

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
