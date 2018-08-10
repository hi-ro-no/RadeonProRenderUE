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
#include "RPRStaticMeshEditor/RPRStaticMeshDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "RPRStaticMeshEditor/RPRStaticMeshEditor.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "RPRStaticMeshDetailCustomization"

FRPRStaticMeshDetailCustomization::FRPRStaticMeshDetailCustomization(FRPRStaticMeshEditor& InRPRStaticMeshEditor)
	: RPRStaticMeshEditor(InRPRStaticMeshEditor)
	, bIsMaterialListDirty(true)
{}

void FRPRStaticMeshDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory("LodSettings");
	DetailBuilder.HideCategory("Collision");
	DetailBuilder.HideCategory("ImportSettings");
	DetailBuilder.HideCategory("Navigation");
	DetailBuilder.HideCategory("Thumbnail");
	DetailBuilder.HideCategory("StaticMesh");
	DetailBuilder.HideCategory("EditableMesh");
	
	IDetailCategoryBuilder& materialsCategory = DetailBuilder.EditCategory(
		TEXT("StaticMeshMaterials"),
		LOCTEXT("StaticMeshMaterialsLabel", "Material Slots"),
		ECategoryPriority::Important
	);

	SelectedMeshDatasPtr = GetStaticMeshes();

	if (SelectedMeshDatasPtr.IsValid())
	{
		FRPRMeshDataContainer& meshDatas = *SelectedMeshDatasPtr;
		int32 materialIndex = 0;
		for (int32 i = 0; i < meshDatas.Num(); ++i)
		{
			if (meshDatas[i].IsValid())
			{
				FMaterialListDelegates delegates;
				delegates.OnGetMaterials.BindSP(this, &FRPRStaticMeshDetailCustomization::GetMaterials, meshDatas[i]);
				delegates.OnMaterialChanged.BindSP(this, &FRPRStaticMeshDetailCustomization::OnMaterialChanged, meshDatas[i]);
				delegates.OnMaterialListDirty.BindSP(this, &FRPRStaticMeshDetailCustomization::IsMaterialListDirty);
				delegates.OnGenerateCustomNameWidgets.BindSP(this, &FRPRStaticMeshDetailCustomization::OnGenerateCustomNameWidgets, meshDatas[i]);

				AddStaticMeshName(materialsCategory, meshDatas[i]->GetStaticMesh());
				materialsCategory.AddCustomBuilder(MakeShareable(new FMaterialList(DetailBuilder, delegates)));
			}
		}

		AddMaterialUtilityButtons(materialsCategory, SelectedMeshDatasPtr);
	}
}

void FRPRStaticMeshDetailCustomization::MarkMaterialListDirty()
{
	bIsMaterialListDirty = true;
}

FRPRMeshDataContainerPtr FRPRStaticMeshDetailCustomization::GetStaticMeshes() const
{
	return (RPRStaticMeshEditor.GetSelectedMeshes());
}

void FRPRStaticMeshDetailCustomization::GetMaterials(IMaterialListBuilder& MaterialListBuidler, FRPRMeshDataPtr MeshData)
{
	if (MeshData.IsValid())
	{
		UStaticMesh* staticMesh = MeshData->GetStaticMesh();
		for (int32 materialIndex = 0; staticMesh->GetMaterial(materialIndex) != nullptr; ++materialIndex)
		{
			MaterialListBuidler.AddMaterial(materialIndex, staticMesh->GetMaterial(materialIndex), true);
		}
	}

	bIsMaterialListDirty = false;
}

void FRPRStaticMeshDetailCustomization::OnMaterialChanged(UMaterialInterface* NewMaterial, UMaterialInterface* PrevMaterial, int32 MaterialIndex, bool bReplaceAll, FRPRMeshDataPtr MeshData)
{
	if (MeshData.IsValid())
	{
		ChangeMaterial(MeshData->GetStaticMesh(), NewMaterial, MaterialIndex);
	}
}

void FRPRStaticMeshDetailCustomization::ChangeMaterial(UStaticMesh* StaticMesh, UMaterialInterface* Material, int32 MaterialIndex)
{
	UProperty* ChangedProperty = NULL;
	ChangedProperty = FindField<UProperty>(UStaticMesh::StaticClass(), GET_MEMBER_NAME_CHECKED(UStaticMesh, StaticMaterials));
	check(ChangedProperty);

	StaticMesh->StaticMaterials[MaterialIndex].MaterialInterface = Material;

	CallPostEditChange(*StaticMesh, ChangedProperty);
}

bool FRPRStaticMeshDetailCustomization::IsMaterialListDirty() const
{
	return (bIsMaterialListDirty);
}

void FRPRStaticMeshDetailCustomization::CallPostEditChange(UStaticMesh& StaticMesh, UProperty* PropertyChanged)
{
	if (PropertyChanged)
	{
		FPropertyChangedEvent PropertyUpdateStruct(PropertyChanged);
		StaticMesh.PostEditChangeProperty(PropertyUpdateStruct);
	}
	else
	{
		StaticMesh.Modify();
		StaticMesh.PostEditChange();
	}

	if (SelectedMeshDatasPtr.IsValid())
	{
		FRPRMeshDataPtr meshDataPtr = SelectedMeshDatasPtr->FindByStaticMesh(&StaticMesh);
		if (meshDataPtr.IsValid())
		{
			meshDataPtr->NotifyStaticMeshMaterialChanges();
			meshDataPtr->NotifyStaticMeshChanges();
		}
	}

	RPRStaticMeshEditor.RefreshViewport();
}

TSharedRef<SWidget> FRPRStaticMeshDetailCustomization::OnGenerateCustomNameWidgets(UMaterialInterface* Material, int32 MaterialIndex, FRPRMeshDataPtr MeshData)
{
	return
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRPRStaticMeshDetailCustomization::IsSectionSelected, MeshData, MaterialIndex)
			.OnCheckStateChanged(this, &FRPRStaticMeshDetailCustomization::ToggleSectionSelection, MeshData, MaterialIndex)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SelectSection", "Select"))
			]
		]
		+SVerticalBox::Slot()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRPRStaticMeshDetailCustomization::IsSectionHighlighted, MeshData, MaterialIndex)
			.OnCheckStateChanged(this, &FRPRStaticMeshDetailCustomization::ToggleSectionHighlight, MeshData, MaterialIndex)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("HighlightSection", "Highlight"))
			]
		]
	;
}

void FRPRStaticMeshDetailCustomization::AddStaticMeshName(IDetailCategoryBuilder& CategoryBuilder, UStaticMesh* StaticMesh)
{
	FName staticMeshName = StaticMesh->GetFName();
	CategoryBuilder.AddCustomRow(FText::GetEmpty())
		[
			SNew(STextBlock)
			.Text(FText::FromName(staticMeshName))
			.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
		];
}

void FRPRStaticMeshDetailCustomization::AddMaterialUtilityButtons(IDetailCategoryBuilder& CategoryBuilder, FRPRMeshDataContainerPtr MeshDatas)
{
	CategoryBuilder.AddCustomRow(LOCTEXT("MaterialUtilityButtons", "Material Utility Utilities"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SButton)
					.OnClicked(this, &FRPRStaticMeshDetailCustomization::HighlightSelectedSections, MeshDatas)
					.Text(LOCTEXT("HighlightSelected", "Highlight selected"))
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SButton)
					.OnClicked(this, &FRPRStaticMeshDetailCustomization::UnhighlightAllSections, MeshDatas)
					.Text(LOCTEXT("UnhighlightSelected", "Unhighlight all"))
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SButton)
					.OnClicked(this, &FRPRStaticMeshDetailCustomization::SelectAllSections, MeshDatas)
					.Text(LOCTEXT("SelectAll", "Select all"))
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SButton)
					.OnClicked(this, &FRPRStaticMeshDetailCustomization::DeselectAllSections, MeshDatas)
					.Text(LOCTEXT("DeselectSections", "Deselect all"))
				]
			]
		];
}

ECheckBoxState FRPRStaticMeshDetailCustomization::IsSectionSelected(FRPRMeshDataPtr MeshData, int32 MaterialIndex) const
{
	if (!MeshData.IsValid() || MaterialIndex >= MeshData->GetNumSections())
	{
		return (ECheckBoxState::Undetermined);
	}

	return (MeshData->GetMeshSection(MaterialIndex).IsSelected() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void FRPRStaticMeshDetailCustomization::ToggleSectionSelection(ECheckBoxState CheckboxState, FRPRMeshDataPtr MeshData, int32 MaterialIndex)
{
	if (!MeshData.IsValid() || MaterialIndex >= MeshData->GetNumSections())
	{
		return;
	}

	const bool bShouldSelect = CheckboxState == ECheckBoxState::Checked;
	MeshData->GetMeshSection(MaterialIndex).Select(bShouldSelect);
}

ECheckBoxState FRPRStaticMeshDetailCustomization::IsSectionHighlighted(FRPRMeshDataPtr MeshData, int32 MaterialIndex) const
{
	if (!MeshData.IsValid() || MaterialIndex >= MeshData->GetNumSections())
	{
		return (ECheckBoxState::Undetermined);
	}

	return (MeshData->GetMeshSection(MaterialIndex).IsHighlighted() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void FRPRStaticMeshDetailCustomization::ToggleSectionHighlight(ECheckBoxState CheckboxState, FRPRMeshDataPtr MeshData, int32 MaterialIndex)
{
	if (!MeshData.IsValid() || MaterialIndex >= MeshData->GetNumSections())
	{
		return;
	}

	const bool bShouldHighlight = CheckboxState == ECheckBoxState::Checked;
	MeshData->HighlightSection(MaterialIndex, bShouldHighlight);
}

FReply FRPRStaticMeshDetailCustomization::HighlightSelectedSections(FRPRMeshDataContainerPtr MeshDatas)
{
	MeshDatas->OnEachMeshData([](FRPRMeshDataPtr MeshData)
	{
		for (int32 sectionIndex = 0; sectionIndex < MeshData->GetNumSections(); ++sectionIndex)
		{
			const FRPRMeshSection& meshSection = MeshData->GetMeshSection(sectionIndex);
			MeshData->HighlightSection(sectionIndex, meshSection.IsSelected());
		}
	});

	return (FReply::Handled());
}

FReply FRPRStaticMeshDetailCustomization::UnhighlightAllSections(FRPRMeshDataContainerPtr MeshDatas)
{
	MeshDatas->OnEachMeshData([](FRPRMeshDataPtr MeshData)
	{
		for (int32 sectionIndex = 0; sectionIndex < MeshData->GetNumSections(); ++sectionIndex)
		{
			MeshData->HighlightSection(sectionIndex, false);
		}
	});

	return (FReply::Handled());
}

FReply FRPRStaticMeshDetailCustomization::SelectAllSections(FRPRMeshDataContainerPtr MeshDatas)
{
	MeshDatas->OnEachMeshData([](FRPRMeshDataPtr MeshData)
	{
		for (int32 sectionIndex = 0; sectionIndex < MeshData->GetNumSections(); ++sectionIndex)
		{
			FRPRMeshSection& meshSection = MeshData->GetMeshSection(sectionIndex);
			meshSection.Select();
		}
	});

	return (FReply::Handled());
}

FReply FRPRStaticMeshDetailCustomization::DeselectAllSections(FRPRMeshDataContainerPtr MeshDatas)
{
	MeshDatas->OnEachMeshData([](FRPRMeshDataPtr MeshData)
	{
		for (int32 sectionIndex = 0; sectionIndex < MeshData->GetNumSections(); ++sectionIndex)
		{
			FRPRMeshSection& meshSection = MeshData->GetMeshSection(sectionIndex);
			meshSection.Deselect();
		}
	});

	return (FReply::Handled());
}

#undef LOCTEXT_NAMESPACE
