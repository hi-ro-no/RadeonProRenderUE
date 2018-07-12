/**********************************************************************
* Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
********************************************************************/
#include "Material/RPRMaterialLibrary.h"
#include "Logging/LogMacros.h"
#include "Helpers/RPRHelpers.h"
#include "Material/RPRMaterialHelpers.h"
#include "Helpers/RPRXMaterialHelpers.h"
#include "Material/Tools/MaterialCacheMaker/MaterialCacheMaker.h"
#include "Misc/ScopeLock.h"
#include "Assets/RPRMaterial.h"
#include "RPRCoreModule.h"
#include "RPRCoreSystemResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogRPRMaterialLibrary, Log, All)

using namespace RPR;

FRPRXMaterialLibrary::FRPRXMaterialLibrary()
	: bIsInitialized(false)
	, DummyMaterial(nullptr)
{}

void FRPRXMaterialLibrary::Initialize()
{
	bIsInitialized = true;
	InitializeDummyMaterial();
}

bool FRPRXMaterialLibrary::IsInitialized() const
{
	return (bIsInitialized);
}

void FRPRXMaterialLibrary::Close()
{
	if (IsInitialized())
	{
		DestroyDummyMaterial();
		ClearCache();
	}
}

bool FRPRXMaterialLibrary::Contains(const URPRMaterial* InMaterial) const
{
	return UEMaterialToRPRMaterialCaches.Contains(InMaterial);
}

bool FRPRXMaterialLibrary::CacheAndRegisterMaterial(URPRMaterial* InMaterial)
{
	check(!Contains(InMaterial));

	RPRI::FExportMaterialResult exportMaterialResult;
	if (CacheMaterial(InMaterial, exportMaterialResult))
	{
		UEMaterialToRPRMaterialCaches.Add(InMaterial, exportMaterialResult);
		return (true);
	}

	return (false);
}

bool FRPRXMaterialLibrary::RecacheMaterial(URPRMaterial* MaterialKey)
{
	uint32 materialType;
	RPR::FMaterialRawDatas material;
	if (TryGetMaterialRawDatas(MaterialKey, materialType, material))
	{
		RPRX::FMaterialCacheMaker cacheMaker(CreateMaterialContext(), MaterialKey);
		auto materialX = reinterpret_cast<RPRX::FMaterial>(material);
		return (cacheMaker.UpdateUberMaterial(materialX));
	}

	return (false);
}

bool FRPRXMaterialLibrary::TryGetMaterialRawDatas(const URPRMaterial* MaterialKey, FMaterialRawDatas& OutRawDatas) const
{
	uint32 materialType;
	return (TryGetMaterialRawDatas(MaterialKey, materialType, OutRawDatas));
}

bool FRPRXMaterialLibrary::TryGetMaterialRawDatas(const URPRMaterial* MaterialKey, uint32& OutMaterialType, FMaterialRawDatas& OutRawDatas) const
{
	const RPRI::FExportMaterialResult* result = FindMaterialCache(MaterialKey);
	if (result == nullptr)
	{
		OutMaterialType = INDEX_NONE;
		OutRawDatas = nullptr;
		return (false);
	}

	OutMaterialType = result->type;
	OutRawDatas = result->data;
	return (true);
}

FMaterialRawDatas	FRPRXMaterialLibrary::GetMaterialRawDatas(const URPRMaterial* MaterialKey) const
{
	RPR::FMaterialRawDatas rawDatas;
	TryGetMaterialRawDatas(MaterialKey, rawDatas);
	return (rawDatas);
}

uint32 FRPRXMaterialLibrary::GetMaterialType(const URPRMaterial* MaterialKey) const
{
	const RPRI::FExportMaterialResult* result = FindMaterialCache(MaterialKey);
	if (result != nullptr)
	{
		return (result->type);
	}
	return (INDEX_NONE);
}

void FRPRXMaterialLibrary::ClearCache()
{
	if (UEMaterialToRPRMaterialCaches.Num() > 0)
	{
		for (auto it = UEMaterialToRPRMaterialCaches.CreateIterator(); it; ++it)
		{
			RPRI::FExportMaterialResult& material = it.Value();
			ReleaseRawMaterialDatas(material);
		}
		UEMaterialToRPRMaterialCaches.Empty();
	}
}

RPR::FMaterialNode FRPRXMaterialLibrary::GetDummyMaterial() const
{
	return (DummyMaterial);
}

FCriticalSection& FRPRXMaterialLibrary::GetCriticalSection()
{
	return (CriticalSection);
}

const RPRI::FExportMaterialResult* FRPRXMaterialLibrary::FindMaterialCache(const URPRMaterial* MaterialKey) const
{
	return (UEMaterialToRPRMaterialCaches.Find(MaterialKey));
}

void FRPRXMaterialLibrary::InitializeDummyMaterial()
{
	if (DummyMaterial != nullptr)
	{
		UE_LOG(LogRPRMaterialLibrary, Warning, TEXT("Dummy material already initialized"));
		return;
	}

	RPR::FMaterialSystem materialSystem = IRPRCore::GetResources()->GetMaterialSystem();
	
	RPR::FResult result = RPR::FMaterialHelpers::CreateNode(materialSystem, EMaterialNodeType::Diffuse, DummyMaterial);
	if (RPR::IsResultFailed(result))
	{
		UE_LOG(LogRPRMaterialLibrary, Error, TEXT("Couldn't create node for dummy material"));
		return;
	}

	result = RPR::FMaterialHelpers::FMaterialNode::SetInputF(DummyMaterial, TEXT("color"), 0.5f, 0.5f, 0.5f, 1.0f);
	if (RPR::IsResultFailed(result))
	{
		UE_LOG(LogRPRMaterialLibrary, Warning, TEXT("Cannot set the default color on the dummy material"));
	}
}

void FRPRXMaterialLibrary::DestroyDummyMaterial()
{
	if (DummyMaterial != nullptr)
	{
		RPR::FMaterialHelpers::DeleteNode(DummyMaterial);
		DummyMaterial = nullptr;
	}
}

bool FRPRXMaterialLibrary::CacheMaterial(URPRMaterial* InMaterial, RPRI::FExportMaterialResult& OutMaterial)
{
	RPRX::FMaterial newMaterial;
	
	RPRX::FMaterialCacheMaker cacheMaker(CreateMaterialContext(), InMaterial);
	if (!cacheMaker.CacheUberMaterial(newMaterial))
	{
		return (false);
	}

	OutMaterial.type = EMaterialType::MaterialX;
	OutMaterial.data = newMaterial;

	InMaterial->ResetMaterialDirtyFlag();
	return (true);
}

void FRPRXMaterialLibrary::ReleaseRawMaterialDatas(RPRI::FExportMaterialResult& Material)
{
	check(IsInitialized());

	try
	{
		RPRX::FContext rprxSupportCtx = IRPRCore::GetResources()->GetRPRXSupportContext();
		RPRI::DeleteMaterial(rprxSupportCtx, Material);
	}
	catch (std::exception)
	{
		UE_LOG(LogRPRMaterialLibrary, Warning, TEXT("Couldn't delete an object/material correctly"));
	}
}

RPR::FMaterialContext FRPRXMaterialLibrary::CreateMaterialContext() const
{
	auto resources = IRPRCore::GetResources();
	RPR::FMaterialContext materialContext;
	{
		materialContext.MaterialSystem = resources->GetMaterialSystem();
		materialContext.RPRContext = resources->GetRPRContext();
		materialContext.RPRXContext = resources->GetRPRXSupportContext();
	}
	return (materialContext);
}