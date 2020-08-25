#pragma once


#include <SOP/SOP_Node.h>
#include <UT/UT_StringHolder.h>


namespace Fenrisulfr
{
    class SOP_HeightFieldCombine : public SOP_Node
    {

    public:
        static PRM_Template *buildTemplates();
        static OP_Node *myConstructor(OP_Network *net, const char *name, OP_Operator *op)
        {
            return new SOP_HeightFieldCombine(net, name, op);
        }

        static const UT_StringHolder theSOPTypeName;
        
        virtual const SOP_NodeVerb *cookVerb() const override;

    protected:
        SOP_HeightFieldCombine(OP_Network *net, const char *name, OP_Operator *op)
            : SOP_Node(net, name, op)
        {
            mySopFlags.setManagesDataIDs(true);
        }
        
        virtual ~SOP_HeightFieldCombine() {}

        virtual OP_ERROR cookMySop(OP_Context &context) override
        {
            return cookMyselfAsVerb(context);
        }
    };

}
