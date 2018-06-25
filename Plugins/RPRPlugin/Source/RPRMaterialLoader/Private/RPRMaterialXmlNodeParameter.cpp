#include "RPRMaterialXmlNodeParameter.h"
#include "Factory/NodeParamTypeFactory.h"
#include "RPRUberMaterialParameters.h"
#include "RPRMaterialXmlGraph.h"
#include "INodeParamType.h"
#include "XmlNode.h"
#include "UberMaterialPropertyHelper.h"
#include "RPRMaterialGraphSerializationContext.h"

#define NODE_ATTRIBUTE_NAME		TEXT("name")
#define NODE_ATTRIBUTE_TYPE		TEXT("type")
#define NODE_ATTRIBUTE_VALUE	TEXT("value")

TMap<FString, ERPRMaterialNodeParameterValueType>	FRPRMaterialXmlNodeParameter::TypeStringToTypeEnumMap;

FRPRMaterialXmlNodeParameter::FRPRMaterialXmlNodeParameter()
{
	if (TypeStringToTypeEnumMap.Num() == 0)
	{
		TypeStringToTypeEnumMap.Add(TEXT("connection"), ERPRMaterialNodeParameterValueType::Connection);
		TypeStringToTypeEnumMap.Add(TEXT("float4"), ERPRMaterialNodeParameterValueType::Float4);
		TypeStringToTypeEnumMap.Add(TEXT("float"), ERPRMaterialNodeParameterValueType::Float);
		TypeStringToTypeEnumMap.Add(TEXT("uint"), ERPRMaterialNodeParameterValueType::UInt);
		TypeStringToTypeEnumMap.Add(TEXT("bool"), ERPRMaterialNodeParameterValueType::Bool);
		TypeStringToTypeEnumMap.Add(TEXT("file_path"), ERPRMaterialNodeParameterValueType::FilePath);
	}
}

bool FRPRMaterialXmlNodeParameter::ParseFromXml(const FXmlNode& Node)
{
	Name = *Node.GetAttribute(NODE_ATTRIBUTE_NAME);
	Value = Node.GetAttribute(NODE_ATTRIBUTE_VALUE);
	Type = ParseType(Node.GetAttribute(NODE_ATTRIBUTE_TYPE));

	return (!Name.IsNone() && Type != ERPRMaterialNodeParameterValueType::Unsupported && !Value.IsEmpty());
}

void FRPRMaterialXmlNodeParameter::LoadRPRMaterialParameters(FRPRMaterialGraphSerializationContext& SerializationContext, 
																									UProperty* PropertyPtr)
{
	const FRPRUberMaterialParameterBase* uberMaterialParameter =
		FUberMaterialPropertyHelper::GetParameterBaseFromProperty(SerializationContext.MaterialParameters, PropertyPtr);

	FString type = uberMaterialParameter->GetPropertyTypeName(PropertyPtr);
	TSharedPtr<INodeParamType> nodeParam = FNodeParamTypeFactory::CreateNewNodeParam(type);
	if (nodeParam.IsValid())
	{
		nodeParam->LoadRPRMaterialParameters(SerializationContext, *this, PropertyPtr);
	}
}

ERPRMaterialNodeParameterValueType FRPRMaterialXmlNodeParameter::ParseType(const FString& TypeValue)
{
	ERPRMaterialNodeParameterValueType vt = ERPRMaterialNodeParameterValueType::Unsupported;

	const ERPRMaterialNodeParameterValueType* valueType = TypeStringToTypeEnumMap.Find(TypeValue);
	if (valueType != nullptr)
	{
		vt = *valueType;
	}

	return (vt);
}

const FName& FRPRMaterialXmlNodeParameter::GetName() const
{
	return (Name);
}

const FString& FRPRMaterialXmlNodeParameter::GetValue() const
{
	return (Value);
}

ERPRMaterialNodeParameterValueType FRPRMaterialXmlNodeParameter::GetType() const
{
	return (Type);
}
