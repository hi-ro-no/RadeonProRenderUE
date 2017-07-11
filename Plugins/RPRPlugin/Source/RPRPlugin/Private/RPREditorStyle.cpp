// RPR COPYRIGHT

#include "RPREditorStyle.h"

#include "SlateStyle.h"
#include "EditorStyle.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( style.RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FRPREditorStyle::RPRStyleInstance = NULL;

void	FRPREditorStyle::Initialize()
{
	if (!RPRStyleInstance.IsValid())
		RPRStyleInstance = Create();

	SetStyle(RPRStyleInstance.ToSharedRef());
}

void	FRPREditorStyle::Shutdown()
{
	ResetToDefault();
	check(RPRStyleInstance.IsUnique());
	RPRStyleInstance.Reset();
}

TSharedRef<FSlateStyleSet>	FRPREditorStyle::Create()
{
	IEditorStyleModule				&editorStyle = FModuleManager::LoadModuleChecked<IEditorStyleModule>(TEXT("EditorStyle"));
	TSharedRef< FSlateStyleSet >	styleRef = editorStyle.CreateEditorStyleInstance();
	FSlateStyleSet					&style = styleRef.Get();

	const FVector2D	Icon20x20(20.0f, 20.0f);
	const FVector2D	Icon16x16(16.0f, 16.0f);

	style.Set("RPRViewport.Render", new IMAGE_BRUSH("Icons/Record_16x", Icon20x20));
	style.Set("RPRViewport.Render.Small", new IMAGE_BRUSH("Icons/Record_16x", Icon16x16));

	style.Set("RPRViewport.SyncOff", new IMAGE_BRUSH("Icons/SourceControlOff_16x", Icon20x20));
	style.Set("RPRViewport.SyncOff.Small", new IMAGE_BRUSH("Icons/SourceControlOff_16x", Icon16x16));

	style.Set("RPRViewport.SyncOn", new IMAGE_BRUSH("Icons/SourceControlOn_16x", Icon20x20));
	style.Set("RPRViewport.SyncOn.Small", new IMAGE_BRUSH("Icons/SourceControlOn_16x", Icon16x16));

	style.Set("RPRViewport.Save", new IMAGE_BRUSH("Icons/icon_file_save_16px", Icon20x20));
	style.Set("RPRViewport.Save.Small", new IMAGE_BRUSH("Icons/icon_file_save_40x", Icon16x16));

	return styleRef;
}
