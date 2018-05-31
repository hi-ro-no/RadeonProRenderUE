#include "RPRMaterialXmlNodeTypeParser.h"

#define NODE_ATTRIBUTE_TYPE	TEXT("type")


TMap<FString, ERPRMaterialNodeType> FRPRMaterialXmlNodeTypeParser::TypeStringToTypeEnumMap;

void FRPRMaterialXmlNodeTypeParser::InitializeParserMapping()
{
	TypeStringToTypeEnumMap.Add(TEXT("UBER"), ERPRMaterialNodeType::Uber);
	TypeStringToTypeEnumMap.Add(TEXT("INPUT_TEXTURE"), ERPRMaterialNodeType::InputTexture);
}

ERPRMaterialNodeType FRPRMaterialXmlNodeTypeParser::ParseTypeFromXml(const FXmlNode& Node)
{
	if (TypeStringToTypeEnumMap.Num() == 0)
	{
		InitializeParserMapping();
	}

	const ERPRMaterialNodeType* nodeType = TypeStringToTypeEnumMap.Find(Node.GetAttribute(NODE_ATTRIBUTE_TYPE));
	if (nodeType != nullptr)
	{
		return (*nodeType);
	}

	return (ERPRMaterialNodeType::Unsupported);
}