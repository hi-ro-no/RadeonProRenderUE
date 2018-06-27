#include "NodeParamTypeFactory.h"

#include "RPRUberMaterialParameters.h"
#include "IsClass.h"
#include "SharedPointer.h"
#include "NodeParamRPRMaterialMap/NodeParamXml_RPRMaterialMap.h"
#include "NodeParamRPRMaterialCoM/NodeParamXml_RPRMaterialCoM.h"
#include "NodeParamRPRMaterialBool/NodeParamXml_RPRMaterialBool.h"
#include "NodeParamRPRMaterialEnum/NodeParamXml_RPRMaterialEnum.h"
#include "NodeParamRPRMaterialCoMChannel1/NodeParamRPRMaterialCoMChannel1.h"

DECLARE_LOG_CATEGORY_CLASS(LogNodeParamTypeFactory, Log, All)

TMap<FString, FNodeParamTypeCreator> FNodeParamTypeFactory::FactoryMap;

TSharedPtr<INodeParamType> FNodeParamTypeFactory::CreateNewNodeParam(const FString& PropertyType)
{
	if (FactoryMap.Num() == 0)
	{
		#define ADD_TO_FACTORY_CHECK_CLASS(ClassCheck, NodeType) \
			AddClassToFactory<ClassCheck, NodeType>(TEXT(#ClassCheck));
		
		ADD_TO_FACTORY_CHECK_CLASS(FRPRMaterialMap, FNodeParamXml_RPRMaterialMap);
		ADD_TO_FACTORY_CHECK_CLASS(FRPRMaterialCoMChannel1, FNodeParamXml_RPRMaterialCoMChannel1);
		ADD_TO_FACTORY_CHECK_CLASS(FRPRMaterialCoM, FNodeParamXml_RPRMaterialCoM);
		ADD_TO_FACTORY_CHECK_CLASS(FRPRMaterialBool, FNodeParamXml_RPRMaterialBool);
		ADD_TO_FACTORY_CHECK_CLASS(FRPRMaterialEnum, FNodeParamXml_RPRMaterialEnum);
	}

	FNodeParamTypeCreator* nodeParamTypeCreator = FactoryMap.Find(PropertyType);
	if (nodeParamTypeCreator == nullptr)
	{
		UE_LOG(LogNodeParamTypeFactory, Warning, TEXT("Type %s not supported!"), *PropertyType);
		return (nullptr);
	}

	TSharedPtr<INodeParamType> nodeParamType = nodeParamTypeCreator->Execute();
	return (nodeParamType);
}


void FNodeParamTypeFactory::AddToFactory(const FString& Key, FNodeParamTypeCreator Value)
{
	FactoryMap.Add(Key, Value);
}
