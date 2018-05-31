#pragma once
#include "INodeParamType.h"

class FNodeParamRPRMaterialBool : public INodeParamType
{

public:
	virtual void	LoadRPRMaterialParameters(struct FRPRMaterialNodeSerializationContext& SerializationContext, 
						class FRPRMaterialXmlNodeParameter& CurrentNodeParameter, class UProperty* Property) override;

};