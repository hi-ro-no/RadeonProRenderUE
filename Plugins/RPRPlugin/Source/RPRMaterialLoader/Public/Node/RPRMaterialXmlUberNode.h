#pragma once

#include "RPRMaterialXmlNode.h"
#include "SharedPointer.h"

class RPRMATERIALLOADER_API FRPRMaterialXmlUberNode : public FRPRMaterialXmlNode
{
public:

	virtual ERPRMaterialNodeType GetNodeType() const override;

	void	LoadRPRMaterialParameters(FRPRMaterialGraphSerializationContext& SerializationContext);

};

typedef TSharedPtr<FRPRMaterialXmlUberNode>	FRPRMaterialXmlUberNodePtr;