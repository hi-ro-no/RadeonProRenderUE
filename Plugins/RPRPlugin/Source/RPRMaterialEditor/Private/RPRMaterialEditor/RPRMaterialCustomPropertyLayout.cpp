#include "RPRMaterialCustomPropertyLayout.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "ModuleManager.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "StructOnScope.h"
#include "DetailWidgetRow.h"
#include "STextBlock.h"
#include "RPRMaterial.h"
#include "MaterialEditHelper.h"
#include "WeakObjectPtrTemplates.h"
#include "Object.h"
#include "TriPlanarMaterialEnabler.h"
#include "MaterialEditor/DEditorStaticSwitchParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "Materials/MaterialInterface.h"
#include "TriPlanarSettingsEditorLoader.h"

#define LOCTEXT_NAMESPACE "RPRMaterialCustomPropertyLayout"

TSharedRef<IDetailCustomization> FRPRMaterialCustomPropertyLayout::MakeInstance(URPRMaterialEditorInstanceConstant* MaterialEditorConstant)
{
    return (MakeShareable(new FRPRMaterialCustomPropertyLayout(MaterialEditorConstant)));
}

void FRPRMaterialCustomPropertyLayout::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.GetObjectsBeingCustomized(MaterialsBeingEdited);

    LoadTriPlanarSettings();
    HideDefaultUberMaterialParameters(DetailBuilder);
    AddUberMaterialParameters(DetailBuilder);
    AddTriPlanarParameters(DetailBuilder);
}

FRPRMaterialCustomPropertyLayout::FRPRMaterialCustomPropertyLayout(URPRMaterialEditorInstanceConstant* InMaterialEditorConstant)
    : MaterialEditorConstant(InMaterialEditorConstant)
{}

void FRPRMaterialCustomPropertyLayout::LoadTriPlanarSettings()
{
    for (int32 i = 0; i < MaterialsBeingEdited.Num(); ++i)
    {
        if (MaterialsBeingEdited[i].IsValid())
        {
            if (URPRMaterial* material = Cast<URPRMaterial>(MaterialsBeingEdited[i].Get()))
            {
                FTriPlanarSettingsEditorLoader settingsLoader(&TriPlanarSettings);
                settingsLoader.LoadFromMaterial(material);
                break;
            }
        }
    }
}

void FRPRMaterialCustomPropertyLayout::HideDefaultUberMaterialParameters(IDetailLayoutBuilder& DetailBuilder)
{
    GetUberMaterialParametersPropertyHandle(DetailBuilder)->MarkHiddenByCustomization();
}

void FRPRMaterialCustomPropertyLayout::AddUberMaterialParameters(IDetailLayoutBuilder& DetailBuilder)
{
    IDetailCategoryBuilder& materialCategoryBuilder = GetMaterialCategory(DetailBuilder);
    TSharedRef<IPropertyHandle> materialParametersPropertyHandle = GetUberMaterialParametersPropertyHandle(DetailBuilder);

    uint32 numChildren;
    if (materialParametersPropertyHandle->GetNumChildren(numChildren) == FPropertyAccess::Success)
    {
        for (uint32 i = 0; i < numChildren; ++i)
        {
            TSharedPtr<IPropertyHandle> childPropertyHandle = materialParametersPropertyHandle->GetChildHandle(i);
            materialCategoryBuilder.AddProperty(childPropertyHandle);
        }
    }
}

void FRPRMaterialCustomPropertyLayout::AddTriPlanarParameters(IDetailLayoutBuilder& DetailBuilder)
{
    IDetailCategoryBuilder& triPlanarCategoryBuilder = DetailBuilder.EditCategory(TEXT("TriPlanar"));

    TSharedPtr<FStructOnScope> structOnScope = MakeShareable(new FStructOnScope(FTriPlanarSettings::StaticStruct(), (uint8*) &TriPlanarSettings));
    IDetailPropertyRow* propertyRow = triPlanarCategoryBuilder.AddExternalStructure(structOnScope);
    propertyRow->Visibility(EVisibility::Collapsed);

    TSharedPtr<IPropertyHandle> triPlanarSettingsPH = propertyRow->GetPropertyHandle();

    TSharedPtr<IPropertyHandle> useTriPlanarPH = triPlanarSettingsPH->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTriPlanarSettings, bUseTriPlanar));
    triPlanarCategoryBuilder.AddProperty(useTriPlanarPH);
    useTriPlanarPH->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FRPRMaterialCustomPropertyLayout::OnTriPlanarValueChanged, useTriPlanarPH->GetProperty()));

    // Display remaining properties but disable them if UseTriPlanar is false
    uint32 numChildren;
    triPlanarSettingsPH->GetNumChildren(numChildren);
    for (uint32 childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        TSharedPtr<IPropertyHandle> childPH = triPlanarSettingsPH->GetChildHandle(childIndex);
        if (childPH->GetProperty() != useTriPlanarPH->GetProperty())
        {
            triPlanarCategoryBuilder.AddProperty(childPH)
                .IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateRaw(this, &FRPRMaterialCustomPropertyLayout::IsTriPlanarUsed)));
            childPH->SetOnPropertyValueChanged(FSimpleDelegate::CreateRaw(this, &FRPRMaterialCustomPropertyLayout::OnTriPlanarValueChanged, childPH->GetProperty()));
        }
    }
}

TSharedRef<IPropertyHandle> FRPRMaterialCustomPropertyLayout::GetUberMaterialParametersPropertyHandle(IDetailLayoutBuilder& DetailBuilder)
{
    return DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URPRMaterial, MaterialParameters));
}

IDetailCategoryBuilder& FRPRMaterialCustomPropertyLayout::GetMaterialCategory(IDetailLayoutBuilder& DetailBuilder)
{
    return DetailBuilder.EditCategory(TEXT("Material"));
}

void FRPRMaterialCustomPropertyLayout::OnTriPlanarValueChanged(UProperty* Property)
{        
    FName propertyName = Property->GetFName();
    TMap<FName, FMaterialParameterBrowseDelegate> router;

#define ADD_TO_ROUTER_IF_REQUIRED(MemberName, MaterialParameterName, CallbackName) \
    if (propertyName == GET_MEMBER_NAME_CHECKED(FTriPlanarSettings, MemberName)) \
        router.Add(MaterialParameterName, FMaterialParameterBrowseDelegate::CreateRaw(this, &FRPRMaterialCustomPropertyLayout::CallbackName));

    ADD_TO_ROUTER_IF_REQUIRED(bUseTriPlanar, FTriPlanarMaterialEnabler::MaterialParameterName_UseTriPlanar, UpdateParam_UseTriPlanar);
    ADD_TO_ROUTER_IF_REQUIRED(Scale, FTriPlanarMaterialEnabler::MaterialParameterName_TextureScale, UpdateParam_Scale);
    ADD_TO_ROUTER_IF_REQUIRED(Angle, FTriPlanarMaterialEnabler::MaterialParameterName_TextureAngle, UpdateParam_Angle);

#undef ADD_TO_ROUTER_IF_REQUIRED

    for (int32 i = 0; i < MaterialsBeingEdited.Num(); ++i)
    {
        if (MaterialsBeingEdited[i].IsValid())
        {
            if (URPRMaterial* material = Cast<URPRMaterial>(MaterialsBeingEdited[i].Get()))
            {
                FMaterialEditHelper::BindRouterAndExecute(MaterialEditorConstant, router);
                MaterialEditorConstant->CopyToSourceInstance();
                material->PostEditChange();
            }
        }
    }
}

bool FRPRMaterialCustomPropertyLayout::IsTriPlanarUsed() const
{
    return (TriPlanarSettings.bUseTriPlanar);
}

void FRPRMaterialCustomPropertyLayout::UpdateParam_UseTriPlanar(UDEditorParameterValue* ParameterValue)
{
    auto param = Cast<UDEditorStaticSwitchParameterValue>(ParameterValue);
    if (param)
    {
        param->bOverride = true;
        param->ParameterValue = TriPlanarSettings.bUseTriPlanar;
    }
}

void FRPRMaterialCustomPropertyLayout::UpdateParam_Scale(UDEditorParameterValue* ParameterValue)
{
    auto param = Cast<UDEditorScalarParameterValue>(ParameterValue);
    if (param)
    {
        param->bOverride = true;
        param->ParameterValue = TriPlanarSettings.Scale;
    }
}

void FRPRMaterialCustomPropertyLayout::UpdateParam_Angle(UDEditorParameterValue* ParameterValue)
{
    auto param = Cast<UDEditorScalarParameterValue>(ParameterValue);
    if (param)
    {
        param->bOverride = true;
        param->ParameterValue = TriPlanarSettings.Angle;
    }
}

#undef LOCTEXT_NAMESPACE