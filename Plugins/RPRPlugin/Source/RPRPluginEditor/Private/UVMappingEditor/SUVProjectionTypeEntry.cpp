#include "SUVProjectionTypeEntry.h"
#include "UVProjectionFactory.h"
#include "SBox.h"
#include "STextBlock.h"
#include "SBoxPanel.h"
#include "SImage.h"

void SUVProjectionTypeEntry::Construct(const FArguments& InArgs)
{
	ProjectionType = InArgs._ProjectionType;
	ProjectionWidget = FUVProjectionFactory::CreateUVProjectionByType(ProjectionType);

	ChildSlot
		.HAlign(EHorizontalAlignment::HAlign_Left)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				SNew(SBox)
				.WidthOverride(40)
				.HeightOverride(40)
				[
					SNew(SImage)
					.Image(InArgs._Icon)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(10, 0)
			.VAlign(EVerticalAlignment::VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(InArgs._ProjectionName)
			]
		];
}

EUVProjectionType SUVProjectionTypeEntry::GetProjectionType() const
{
	return (ProjectionType);
}

IUVProjectionPtr SUVProjectionTypeEntry::GetUVProjectionWidget() const
{
	return (ProjectionWidget);
}