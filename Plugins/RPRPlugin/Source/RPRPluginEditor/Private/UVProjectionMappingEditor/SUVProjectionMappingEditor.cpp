#include "SUVProjectionMappingEditor.h"
#include "RPRStaticMeshEditor.h"
#include "AssetEditorManager.h"
#include "EditorStyle.h"
#include "SUVProjectionTypeEntry.h"
#include "SListView.h"
#include "SBoxPanel.h"
#include "STextBlock.h"
#include "Engine/StaticMesh.h"
#include "SBox.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "RawMesh.h"
#include "StaticMeshHelper.h"
#include "IUVProjectionModule.h"
#include "UVProjectionFactory.h"

#define LOCTEXT_NAMESPACE "SUVMappingEditor"

SUVProjectionMappingEditor::SUVProjectionMappingEditor()
{}

void SUVProjectionMappingEditor::Construct(const SUVProjectionMappingEditor::FArguments& InArgs)
{
	RPRStaticMeshEditorPtr = InArgs._RPRStaticMeshEditor;

	OnProjectionApplied = InArgs._OnProjectionApplied;
	InitUVProjectionList();
	
	this->ChildSlot
		[
			SNew(SScrollBox)
			.Orientation(EOrientation::Orient_Vertical)
			+SScrollBox::Slot()
			[
				SNew(SBorder)
				[
					SAssignNew(UVProjectionTypeListWidget, SListView<SUVProjectionTypeEntryPtr>)
					.SelectionMode(ESelectionMode::Single)
					.ListItemsSource(&UVProjectionTypeList)
					.OnGenerateRow(this, &SUVProjectionMappingEditor::OnGenerateWidgetForUVProjectionTypeEntry)
					.OnSelectionChanged(this, &SUVProjectionMappingEditor::OnUVProjectionTypeSelectionChanged)
				]
			]
			+SScrollBox::Slot()
			[
				SAssignNew(UVProjectionContainer, SBorder)
				.Visibility(this, &SUVProjectionMappingEditor::GetUVProjectionControlsVisibility)
			]
		];
}

void SUVProjectionMappingEditor::SelectProjectionEntry(SUVProjectionTypeEntryPtr ProjectionEntry)
{
	if (SelectedProjectionEntry != ProjectionEntry)
	{
		HideSelectedUVProjectionWidget();
		SelectedProjectionEntry = ProjectionEntry;
		ShowSelectedUVProjectionWidget();
	}
}

void SUVProjectionMappingEditor::UpdateSelection()
{
	if (CurrentProjectionSettingsWidget.IsValid())
	{
		CurrentProjectionSettingsWidget->OnSectionSelectionChanged();
	}
}

void SUVProjectionMappingEditor::Enable(bool bEnable)
{
	if (bEnable)
	{
		ShowUVProjectionWidget(CurrentProjectionSettingsWidget);
	}
	else
	{
		HideUVProjectionWidget(CurrentProjectionSettingsWidget);
	}
}

void SUVProjectionMappingEditor::InitUVProjectionList()
{
	const TArray<IUVProjectionModule*>& modules = FUVProjectionFactory::GetModules();
	for (int32 i = 0; i < modules.Num(); ++i)
	{
		AddUVProjectionListEntry(modules[i]);
	}
}

void SUVProjectionMappingEditor::AddUVProjectionListEntry(IUVProjectionModule* UVProjectionModule)
{
	SUVProjectionTypeEntryPtr uvProjectionTypeEntryWidget = 
		SNew(SUVProjectionTypeEntry)
		.UVProjectionModulePtr(UVProjectionModule);

	UVProjectionTypeList.Add(uvProjectionTypeEntryWidget);
}

TSharedRef<ITableRow> SUVProjectionMappingEditor::OnGenerateWidgetForUVProjectionTypeEntry(SUVProjectionTypeEntryPtr InItem,
																			const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<SUVProjectionTypeEntryPtr>, OwnerTable)
		[
			InItem.ToSharedRef()
		];
}

void SUVProjectionMappingEditor::OnUVProjectionTypeSelectionChanged(SUVProjectionTypeEntryPtr InItemSelected, ESelectInfo::Type SelectInfo)
{
	SelectProjectionEntry(InItemSelected);
}

bool SUVProjectionMappingEditor::HasUVProjectionTypeSelected() const
{
	return (SelectedProjectionEntry.IsValid());
}

EVisibility SUVProjectionMappingEditor::GetUVProjectionControlsVisibility() const
{
	return (HasUVProjectionTypeSelected() ? EVisibility::Visible : EVisibility::Collapsed);
}

void SUVProjectionMappingEditor::HideSelectedUVProjectionWidget()
{
	if (SelectedProjectionEntry.IsValid())
	{
		HideUVProjectionWidget(CurrentProjectionSettingsWidget);
	}
}

void SUVProjectionMappingEditor::HideUVProjectionWidget(IUVProjectionSettingsWidgetPtr UVProjectionWidget)
{
	if (UVProjectionWidget.IsValid())
	{
		UVProjectionWidget->OnUVProjectionHidden();
	}
}

void SUVProjectionMappingEditor::ShowSelectedUVProjectionWidget()
{
	if (SelectedProjectionEntry.IsValid())
	{
		IUVProjectionModule* module = SelectedProjectionEntry->GetUVProjectionModule();
		CurrentProjectionSettingsWidget = module->CreateUVProjectionSettingsWidget(RPRStaticMeshEditorPtr);
		CurrentProjectionSettingsWidget->OnProjectionApplied().BindSP(this, &SUVProjectionMappingEditor::NotifyProjectionCompleted);

		ShowUVProjectionWidget(CurrentProjectionSettingsWidget);
	}
}

void SUVProjectionMappingEditor::ShowUVProjectionWidget(IUVProjectionSettingsWidgetPtr UVProjectionWidget)
{
	if (UVProjectionWidget.IsValid())
	{
		InjectUVProjectionWidget(UVProjectionWidget);
		UVProjectionWidget->OnUVProjectionDisplayed();
	}
}

void SUVProjectionMappingEditor::NotifyProjectionCompleted()
{
	OnProjectionApplied.ExecuteIfBound();
}

void SUVProjectionMappingEditor::InjectUVProjectionWidget(IUVProjectionSettingsWidgetPtr UVProjectionWidget)
{
	if (UVProjectionWidget.IsValid())
	{
		UVProjectionContainer->SetContent(UVProjectionWidget->TakeWidget());
	}
	else
	{
		ClearUVProjectionWidgetContainer();
	}
}

void SUVProjectionMappingEditor::ClearUVProjectionWidgetContainer()
{
	UVProjectionContainer->SetContent(SNew(SBox));
}

#undef LOCTEXT_NAMESPACE