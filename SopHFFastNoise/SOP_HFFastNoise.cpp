

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
        name    "mode"
        label   "Layer Mode"
        type    ordinal
        default { "replace" }
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
        name    "bind_layer"
        label   "Bind Layer"
        type    string
        default { "" }
        menureplace {
        }
    }
    parm {
        name    "noise_type"
        label   "Noise Type"
        type    ordinal
        default { "0" }
        menu {
            "ValueFractal"      "Value Fractal"
            "PerlinFractal"     "Perlin Fractal"
            "SimplexFractal"    "Simplex Fractal"
            "Cellular"          "Cellular"
            "CubicFractal"      "Cubic Fractal"
        }
    }
    parm {
        name    "seed"
        label   "Seed"
        type    integer
        default { "1984" }
        range   { 0 1e+07 }
    }
    parm {
        name    "size"
        label   "Size"
        type    float
        default { "50" }
        range   { 1 1000 }
    }
    parm {
        name    "offset"
        label   "Offset"
        type    vector
        size    3
        default { "0" "0" "0" }
        range   { 0 5000 }
    }
    parm {
        name    "noise_scale"
        label   "Noise Scale"
        type    float
        default { "1" }
        range   { 0 500 }
    }
    groupsimple {
        name    "fractal"
        label   "Fractal"

        parm {
            name    "fractal_type"
            label   "Type"
            type    ordinal
            default { "0" }
            menu {
                "FBM"           "FBM"
                "Billow"        "Billow"
                "RigidMulti"    "Rigid Multi"
            }
        }
        parm {
            name    "octaves"
            label   "Octaves"
            type    integer
            default { "0" }
            range   { 1 12 }
        }
        parm {
            name    "lacunarity"
            label   "Lacunarity"
            type    float
            default { "2" }
            range   { 0 5 }
        }
        parm {
            name    "gain"
            label   "Gain"
            type    float
            default { "0.5" }
            range   { 0 1 }
        }
    }

    groupsimple {
        name    "cellular"
        label   "Cellular"
        hidewhentab "{ noise_type != Cellular }"

        parm {
            name    "return"
            label   "Return"
            type    ordinal
            default { "0" }
            menu {
                "CellValue"     "Cell Value"
                "Distance"      "Distance"
                "Distancet"     "Distance2"
                "DistancetAdd"  "Distance2 Add"
                "DistancetMul"  "Distance2 Mul"
                "DistancetDiv"  "Distance2 Div"
                "NoiseLookup"   "Noise Lookup"
                "DistancetCave" "Distance2 Cave"
            }
        }
        parm {
            name    "distance_func"
            label   "Distance Function"
            type    ordinal
            default { "0" }
            menu {
                "Euclidean" "Euclidean"
                "Manhattan" "Manhattan"
                "Natural"   "Natural"
            }
        }
        parm {
            name    "distance_indicies"
            label   "Distance2 Indicies"
            type    integer
            default { "1" }
            range   { 1! 3! }
        }
        parm {
            name    "jigger"
            label   "Jigger"
            type    float
            default { "0.45" }
            range   { 0! 1! }
        }
        parm {
            name    "noise_lookup_type"
            label   "Noise Lookup Type"
            type    ordinal
            default { "0" }
            menu {
                "ValueFractal"      "Value Fractal"
                "PerlinFractal"     "Perlin Fractal"
                "SimplexFractal"    "Simplex Fractal"
                "Cellular"          "Cellular"
            }
        }
        parm {
            name    "noise_lookup_freq"
            label   "Noise Lookup Freq"
            type    float
            default { "0" }
            range   { 0 10 }
        }
    }

    groupsimple {
        name    "perturb_lab"
        label   "Perturb"

        parm {
            name    "perturb"
            label   "Perturb"
            type    ordinal
            default { "0" }
            menu {
                "None"                      "None"
                "GradientFractal"           "Gradient Fractal"
                "GradientNormalise"         "Gradient + Normalise"
                "GradientFractalNormalise"  "Gradient Fractal + Normalise"
            }
        }
        parm {
            name    "turb_amp"
            label   "Perturb Amp"
            type    float
            default { "1" }
            disablewhen "{ perturb == None }"
            hidewhen "{ perturb == None }"
            range   { 0 100 }
        }
        parm {
            name    "perturb_frequenoy"
            label   "Frequenoy"
            type    float
            default { "0.5" }
            disablewhen "{ perturb == None }"
            hidewhen "{ perturb == None }"
            range   { 0 3 }
        }
        parm {
            name    "perturb_octaves"
            label   "Fractal Octaves"
            type    integer
            default { "1" }
            disablewhen "{ perturb == None }"
            hidewhen "{ perturb == None }"
            range   { 1 10 }
        }
        parm {
            name    "perturb_lacunarity"
            label   "Fractal Lacunarity"
            type    float
            default { "0" }
            disablewhen "{ perturb == None }"
            hidewhen "{ perturb == None }"
            range   { 0 2 }
        }
        parm {
            name    "perturb_gain"
            label   "Fractal Gain"
            type    float
            default { "0.5" }
            disablewhen "{ perturb == None }"
            hidewhen "{ perturb == None }"
            range   { 0 10 }
        }
        parm {
            name    "normalize_length"
            label   "Normalize Length"
            type    float
            default { "1" }
            hidewhen "{ perturb == None }"
            range   { 0 1000 }
        }
    }
}
)THEDSFILE";

PRM_Template*
SOP_HFFastNoise::buildTemplates()
{
    static PRM_TemplateBuilder templ("SOP_HFFastNoise.cpp"_sh, theDsFile);
	if (templ.justBuilt())
	{
		templ.setChoiceListPtr("bind_layer"_sh, &SOP_Node::namedPrimsMenu);
	}
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

    virtual void cook(const CookParms &cookparms)const;

    static const SOP_NodeVerb::Register<SOP_HFFastNoiseVerb> theVerb;

	THREADED_METHOD6_CONST(SOP_HFFastNoiseVerb, true, bar,  UT_VoxelArrayF*, arr, float*, noiseSet,fpreal64, noiseScale, int, resy, int, resz, SOP_HFFastNoiseParms::Mode, myMode)
	void  barPartial( UT_VoxelArrayF* arr, float* noiseSet, fpreal64 noiseScale, int resy, int resz, SOP_HFFastNoiseParms::Mode myMode,const UT_JobInfo &info)const;


};


const SOP_NodeVerb::Register<SOP_HFFastNoiseVerb> SOP_HFFastNoiseVerb::theVerb;

const SOP_NodeVerb *
SOP_HFFastNoise::cookVerb() const
{ 
    return SOP_HFFastNoiseVerb::theVerb.get();
}

void 
SOP_HFFastNoiseVerb::barPartial(UT_VoxelArrayF* arr, float* noiseSet, fpreal64 noiseScale, int resy, int resz, SOP_HFFastNoiseParms::Mode myMode,const UT_JobInfo &info)const
{
	UT_VoxelArrayIteratorF	vit;
	UT_Interrupt		*boss = UTgetInterrupt();

	vit.setArray(arr);
	vit.setCompressOnExit(true);
	vit.setPartialRange(info.job(), info.numJobs());
// 
// 	UT_VoxelProbeF		probe;
// 	probe.setConstArray(arr);
	for (vit.rewind(); !vit.atEnd(); vit.advance())
	{
		if (vit.isStartOfTile() && boss->opInterrupt())
			break;
/*		probe.setIndex(vit);*/
/*		std::cout << "Volume index " << vit.x() * resy * resz + vit.y() * resz + vit.z() << std::endl;*/
		float val = noiseSet[ vit.x() * resy * resz + vit.y() * resz + vit.z()] * noiseScale;

		switch (myMode)
		{
		case SOP_HFFastNoiseParms::Mode::REPLACE:
		{
			vit.setValue(val);
			break;
		}
		case SOP_HFFastNoiseParms::Mode::ADD:
		{
			val += vit.getValue();
			vit.setValue(val);
			break;
		}
		case SOP_HFFastNoiseParms::Mode::SUBTRACT:
		{
			val -= vit.getValue();
			vit.setValue(val);
			break;
		}
		case SOP_HFFastNoiseParms::Mode::MULTIPLY:
		{
			val *= vit.getValue();
			vit.setValue(val);
			break;
		}
		case SOP_HFFastNoiseParms::Mode::MAX:
		{
			val = SYSmax(val, vit.getValue());
			vit.setValue(val);
			break;
		}
		case SOP_HFFastNoiseParms::Mode::MIN:
		{
			val = SYSmax(val, vit.getValue());
			vit.setValue(val);
			break;
		}
		default:
			break;
		}

		
	}
}

namespace FastNoiseToolFunc
{

	void SetNoiseType(FastNoiseSIMD * myNoise, SOP_HFFastNoiseParms::Noise_type myNoiseType)
	{
		switch (myNoiseType)
		{
		case SOP_HFFastNoiseParms::Noise_type::VALUEFRACTAL:
		{
			myNoise->SetNoiseType(FastNoiseSIMD::NoiseType::ValueFractal);
			break;
		}
		case SOP_HFFastNoiseParms::Noise_type::SIMPLEXFRACTAL:
		{
			myNoise->SetNoiseType(FastNoiseSIMD::NoiseType::SimplexFractal);
			break;
		}
		case SOP_HFFastNoiseParms::Noise_type::PERLINFRACTAL:
		{
			myNoise->SetNoiseType(FastNoiseSIMD::NoiseType::PerlinFractal);
			break;
		}
		case SOP_HFFastNoiseParms::Noise_type::CUBICFRACTAL:
		{
			myNoise->SetNoiseType(FastNoiseSIMD::NoiseType::CubicFractal);
			break;
		}
		case SOP_HFFastNoiseParms::Noise_type::CELLULAR:
		{
			myNoise->SetNoiseType(FastNoiseSIMD::NoiseType::Cellular);
			break;
		}
		default:
			break;
		}
	}

	void SetFractalType(FastNoiseSIMD * myNoise, SOP_HFFastNoiseParms::Fractal_type myFractalType)
	{
		switch (myFractalType)
		{
			case SOP_HFFastNoiseParms::Fractal_type::FBM:
			{
				myNoise->SetFractalType(FastNoiseSIMD::FractalType::FBM);
				break;
			}
			case SOP_HFFastNoiseParms::Fractal_type::BILLOW:
			{
				myNoise->SetFractalType(FastNoiseSIMD::FractalType::Billow);
				break;
			}
			case SOP_HFFastNoiseParms::Fractal_type::RIGIDMULTI:
			{
				myNoise->SetFractalType(FastNoiseSIMD::FractalType::RigidMulti);
				break;
			}

			default:
				break;
		}
	}

	void SetReturnType(FastNoiseSIMD * myNoise,  SOP_HFFastNoiseParms::Return myReturn)
	{
		switch (myReturn)
		{
		case SOP_HFFastNoiseParms::Return::CELLVALUE:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::CellValue);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCE:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCET:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCETADD:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2Add);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCETCAVE:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2Cave);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCETDIV:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2Div);
			break;
		}
		case SOP_HFFastNoiseParms::Return::DISTANCETMUL:
		{
			myNoise->SetCellularReturnType(FastNoiseSIMD::CellularReturnType::Distance2Mul);
			break;
		}
		default:
			break;
		}
	}

	void SetDistanceFunc(FastNoiseSIMD * myNoise, SOP_HFFastNoiseParms::Distance_func myDistFunc)
	{
		switch (myDistFunc)
		{
		case SOP_HFFastNoiseParms::Distance_func::EUCLIDEAN:
		{
			myNoise->SetCellularDistanceFunction(FastNoiseSIMD::Euclidean);
			break;
		}
		case SOP_HFFastNoiseParms::Distance_func::MANHATTAN:
		{
			myNoise->SetCellularDistanceFunction(FastNoiseSIMD::Manhattan);
			break;
		}
		case SOP_HFFastNoiseParms::Distance_func::NATURAL:
		{
			myNoise->SetCellularDistanceFunction(FastNoiseSIMD::Natural);
			break;
		}
		default:
			break;
		}
	}

	void SetNoiseLookUp(FastNoiseSIMD * myNoise,  SOP_HFFastNoiseParms::Noise_lookup_type myLookupType)
	{
		switch (myLookupType)
		{
		case  SOP_HFFastNoiseParms::Noise_lookup_type::CELLULAR:
		{
			myNoise->SetCellularNoiseLookupType(FastNoiseSIMD::Cellular);
			break;
		}
		case  SOP_HFFastNoiseParms::Noise_lookup_type::PERLINFRACTAL:
		{
			myNoise->SetCellularNoiseLookupType(FastNoiseSIMD::PerlinFractal);
			break;
		}
		case  SOP_HFFastNoiseParms::Noise_lookup_type::SIMPLEXFRACTAL:
		{
			myNoise->SetCellularNoiseLookupType(FastNoiseSIMD::SimplexFractal);
			break;
		}
		case  SOP_HFFastNoiseParms::Noise_lookup_type::VALUEFRACTAL:
		{
			myNoise->SetCellularNoiseLookupType(FastNoiseSIMD::ValueFractal);
			break;
		}
		default:
			break;
		}
	}

	void SetTurbType(FastNoiseSIMD * myNoise,  SOP_HFFastNoiseParms::Perturb myTurbType)
	{
		switch (myTurbType)
		{
		case SOP_HFFastNoiseParms::Perturb::NONE:
		{
			myNoise->SetPerturbType(FastNoiseSIMD::PerturbType::None);
			break;
		}
		case SOP_HFFastNoiseParms::Perturb::GRADIENTFRACTAL:
		{
			myNoise->SetPerturbType(FastNoiseSIMD::PerturbType::GradientFractal);
			break;
		}
		case SOP_HFFastNoiseParms::Perturb::GRADIENTFRACTALNORMALISE:
		{
			myNoise->SetPerturbType(FastNoiseSIMD::PerturbType::GradientFractal_Normalise);
			break;
		}
		case SOP_HFFastNoiseParms::Perturb::GRADIENTNORMALISE:
		{
			myNoise->SetPerturbType(FastNoiseSIMD::PerturbType::Gradient_Normalise);
			break;
		}
		default:
			break;
		}
	}

}


void
SOP_HFFastNoiseVerb::cook(const SOP_NodeVerb::CookParms &cookparms) const
{

    // My code in theres.
    auto &&sopparms = cookparms.parms<SOP_HFFastNoiseParms>();
    GU_Detail *detail = cookparms.gdh().gdpNC();

	SOP_HFFastNoiseParms::Mode myMode{ sopparms.getMode() };
	UT_StringHolder sourceLayer{ sopparms.getBind_layer() };
	fpreal64 noiseScale{ sopparms.getNoise_scale() };
	UT_Vector3 offset{ sopparms.getOffset() * -1};
	GEO_Primitive* maskPrim = detail->findPrimitiveByName(sourceLayer);

	if (maskPrim == nullptr)
		return;

	if (maskPrim->getPrimitiveId() == GEO_PrimTypeCompat::GEOPRIMVOLUME)
	{
		//std::cout << "Find Mask Volume.\n";
		GEO_PrimVolume *vol = (GEO_PrimVolume *)maskPrim;

		int resx, resy, resz;
		UT_Vector3 p1, p2;
		vol->getRes(resx, resy, resz);
		vol->indexToPos(0, 0, 0, p1);
		vol->indexToPos(1, 0, 0, p2);


		float voxelLength = (p1 - p2).length();
		UT_Vector3 center{ detail->getPos3(vol->getPointOffset(0)) + offset };

// 		std::cout << "Volume Res : " << resx << " " << resy << " " << resz << std::endl;
// 		std::cout << "Voxel Length " << voxelLength << std::endl;

		// Noise Parameters
		SOP_HFFastNoiseParms::Noise_type noiseType{ sopparms.getNoise_type() };
		int64 seed{ sopparms.getSeed() };
		fpreal64 size{ sopparms.getSize() };

		// Fractal Noise Parameter
		SOP_HFFastNoiseParms::Fractal_type fractalType{ sopparms.getFractal_type() };
		int64 octaves{ sopparms.getOctaves() };
		fpreal64 lacunaroty{ sopparms.getLacunarity() };
		fpreal64 grain{ sopparms.getGain() };

		//Cellular Noise Parameter
		SOP_HFFastNoiseParms::Return returnType{ sopparms.getReturn() };
		SOP_HFFastNoiseParms::Distance_func distFunc{ sopparms.getDistance_func() };
		int64 distIndic{ sopparms.getDistance_indicies() };
		fpreal64 jigger{ sopparms.getJigger() };
		SOP_HFFastNoiseParms::Noise_lookup_type noiseLookUpType{ sopparms.getNoise_lookup_type() };
		fpreal64 lookupFrq{ sopparms.getNoise_lookup_freq() };

		//Perturb Noise Parameter
		SOP_HFFastNoiseParms::Perturb perturb{ sopparms.getPerturb() };
		fpreal64 turbAmp{ sopparms.getTurb_amp() };
		fpreal64 turbFreq{ sopparms.getPerturb_frequenoy() };
		int64 turbFtactalOct{ sopparms.getPerturb_octaves() };
		fpreal64 turbLacunaroty{ sopparms.getPerturb_lacunarity() };
		fpreal64 turbGain{ sopparms.getPerturb_gain() };
		fpreal64 turbNormalLength{ sopparms.getNormalize_length() };

		// Create Noise type 
		FastNoiseSIMD* myNoise = FastNoiseSIMD::NewFastNoiseSIMD();

		// Set Noise Type Parameter
		FastNoiseToolFunc::SetNoiseType(myNoise, noiseType);
		myNoise->SetSeed(seed);
		myNoise->SetFrequency( 1 / size);

		// Set Noise Fractal Parameter
		FastNoiseToolFunc::SetFractalType(myNoise, fractalType);
		myNoise->SetFractalOctaves(octaves);
		myNoise->SetFractalLacunarity(lacunaroty);
		myNoise->SetFractalGain(grain);

		//Set Cellular Noise Parameter
		FastNoiseToolFunc::SetReturnType(myNoise, returnType);
		FastNoiseToolFunc::SetDistanceFunc(myNoise, distFunc);
		/*myNoise->SetCellularDistance2Indicies(0, distIndic);*/
		myNoise->SetCellularJitter(jigger);
		FastNoiseToolFunc::SetNoiseLookUp(myNoise, noiseLookUpType);
		myNoise->SetCellularNoiseLookupFrequency(lookupFrq);

		//Set perturb 
		FastNoiseToolFunc::SetTurbType(myNoise, perturb);
		myNoise->SetPerturbAmp(turbAmp);
		myNoise->SetPerturbFrequency(turbFreq);
		myNoise->SetPerturbFractalOctaves(turbFtactalOct);
		myNoise->SetPerturbFractalLacunarity(turbLacunaroty);
		myNoise->SetPerturbFractalGain(turbGain);
		myNoise->SetPerturbNormaliseLength(turbNormalLength);

		//Set Axi Scale
		myNoise->SetAxisScales(voxelLength, voxelLength, voxelLength);

		float* noiseSet;
		noiseSet = myNoise->GetNoiseSet(center.x(), center.y(), center.z(), resx, resy, resz);

		UT_VoxelArrayWriteHandleF	handle = vol->getVoxelWriteHandle();
		UT_VoxelArrayF *arr = &(*handle);

		bar(arr, noiseSet, noiseScale, resy, resz, myMode);

		FastNoiseSIMD::FreeNoiseSet(noiseSet);

	}
	detail->getPrimitiveList().bumpDataId();
}
