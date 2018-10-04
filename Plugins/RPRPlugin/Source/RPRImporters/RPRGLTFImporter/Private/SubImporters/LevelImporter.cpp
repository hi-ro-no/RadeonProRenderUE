#include "SubImporters/LevelImporter.h"
#include "AssetRegistryModule.h"
#include "RPRSettings.h"
#include "RPR_GLTF_Tools.h"
#include "File/RPRFileHelper.h"
#include "Engine/StaticMeshActor.h"
#include "Helpers/RPRHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "RPRShapeDataToStaticMeshComponent.h"
#include "Engine/Light.h"
#include "RPRLightDataToLightComponent.h"
#include "RPRCameraDataToCameraComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SpotLight.h"
#include "Helpers/RPRLightHelpers.h"
#include "Helpers/RPRCameraHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Helpers/RPRShapeHelpers.h"
#include "GTLFImportSettings.h"
#include "Camera/CameraActor.h"
#include "Editor.h"
#include "ActorFactories/ActorFactorySkyLight.h"
#include "Engine/Level.h"
#include "Factories/WorldFactory.h"
#include "Components/SkyLightComponent.h"
#include "FileHelpers.h"

DECLARE_LOG_CATEGORY_CLASS(LogLevelImporter, Log, All)

bool RPR::GLTF::Import::FLevelImporter::ImportLevel(
	const gltf::glTFAssetData& GLTFFileData, 
	RPR::FScene Scene, 
	FResources& Resources,
	UWorld*& OutWorld)
{
	OutWorld = CreateNewWorld(GLTFFileData);
	if (OutWorld == nullptr)
	{
		return (false);
	}

	TArray<AActor*> actors;

	SetupMeshes(OutWorld, Scene, Resources.MeshResources, actors);
	SetupLights(OutWorld, Scene, Resources.ImageResources);
	// SetupCameras(OutWorld, Scene);
	
	SetupHierarchy(actors);

	SaveWorld(GLTFFileData, OutWorld);
	return (true);
}

UWorld* RPR::GLTF::Import::FLevelImporter::CreateNewWorld(const gltf::glTFAssetData& GLTFFileData)
{
	FString sceneName = FString(GLTFFileData.scenes[GLTFFileData.scene].name.c_str());
	UWorld* newWorld = UWorld::CreateWorld(EWorldType::Inactive, false, *sceneName);

	return newWorld;
}

void RPR::GLTF::Import::FLevelImporter::SaveWorld(const gltf::glTFAssetData& GLTFFileData, UWorld* World)
{
	FString sceneName = FString(GLTFFileData.scenes[GLTFFileData.scene].name.c_str());
	URPRSettings* rprSettings = GetMutableDefault<URPRSettings>();
	FString directory = rprSettings->DefaultRootDirectoryForImportLevels.Path;
	FString sceneFilePath = FPaths::Combine(directory, sceneName);
	sceneFilePath = FRPRFileHelper::FixFilenameIfInvalid<UWorld>(sceneFilePath, TEXT("RPRScene"));
	FEditorFileUtils::SaveLevel(World->GetLevel(0), sceneFilePath);

	FAssetRegistryModule::AssetCreated(World);
}

void RPR::GLTF::Import::FLevelImporter::SetupMeshes(UWorld* World, RPR::FScene Scene, RPR::GLTF::FStaticMeshResourcesPtr MeshResources, TArray<AActor*>& OutActors)
{
	TArray<FShape> shapes;
	RPR::FResult status = RPR::GLTF::Import::GetShapes(shapes);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Cannot get shapes in the scene"));
		return;
	}

	RPR::EShapeType shapeType;
	for (int32 i = 0; i < shapes.Num(); ++i)
	{
		status = RPR::Shape::GetType(shapes[i], shapeType);
		if (RPR::IsResultFailed(status))
		{
			UE_LOG(LogLevelImporter, Error, TEXT("Cannot get shape type (%s)"), *RPR::Shape::GetName(shapes[i]));
			continue;
		}

		AActor* actor = nullptr;

		if (shapeType == EShapeType::Mesh)
		{
			actor = SetupMesh(World, shapes[i], i, MeshResources);
		}
		else if (shapeType == EShapeType::Instance)
		{
			actor = SetupMeshForShapeInstance(World, shapes[i], i, MeshResources);
		}

		OutActors.Add(actor);
	}
}

AActor* RPR::GLTF::Import::FLevelImporter::SetupMesh(UWorld* World, RPR::FShape Shape, int32 Index, RPR::GLTF::FStaticMeshResourcesPtr MeshResources)
{
	FString actorMeshName;
	RPR::FResult status = RPR::Shape::GetName(Shape, actorMeshName);
	if (RPR::IsResultFailed(status) || actorMeshName.IsEmpty())
	{
		actorMeshName = FString::Printf(TEXT("shape_%d"), Index);
	}

	return SetupMesh(World, actorMeshName, Shape, Index, MeshResources);
}

AActor* RPR::GLTF::Import::FLevelImporter::SetupMesh(UWorld* World, FString ActorMeshName, RPR::FShape Shape, int32 Index, RPR::GLTF::FStaticMeshResourcesPtr MeshResources)
{
	AStaticMeshActor* meshActor = World->SpawnActor<AStaticMeshActor>();
	meshActor->SetActorLabel(ActorMeshName);
	UStaticMeshComponent* staticMeshComponent = meshActor->FindComponentByClass<UStaticMeshComponent>();

	UE_LOG(LogLevelImporter, Log, TEXT("--- Mesh : %s"), *ActorMeshName);

	RPR::GLTF::Import::FRPRShapeDataToMeshComponent::Setup(Shape, staticMeshComponent, MeshResources, meshActor);
	UpdateTransformAccordingToImportSettings(meshActor);

	return meshActor;
}

AActor* RPR::GLTF::Import::FLevelImporter::SetupMeshForShapeInstance(UWorld* World, RPR::FShape ShapeInstance, int32 Index, RPR::GLTF::FStaticMeshResourcesPtr MeshResources)
{
	RPR::FShape meshShape;
	RPR::FResult status = RPR::Shape::GetInstanceBaseShape(ShapeInstance, meshShape);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Cannot get shape mesh (%s)"), *RPR::Shape::GetName(meshShape));
		return nullptr;
	}

	FString actorMeshName;
	status = RPR::Shape::GetName(ShapeInstance, actorMeshName);
	if (RPR::IsResultFailed(status) || actorMeshName.IsEmpty())
	{
		actorMeshName = FString::Printf(TEXT("shape_instance_%d"), Index);
	}

	AStaticMeshActor* meshActor = World->SpawnActor<AStaticMeshActor>();
	meshActor->SetActorLabel(actorMeshName);
	UStaticMeshComponent* staticMeshComponent = meshActor->FindComponentByClass<UStaticMeshComponent>();

	UE_LOG(LogLevelImporter, Log, TEXT("--- Mesh Instance : %s"), *actorMeshName);

	RPR::GLTF::Import::FRPRShapeDataToMeshComponent::SetupShapeInstance(ShapeInstance, staticMeshComponent, MeshResources, meshActor);
	UpdateTransformAccordingToImportSettings(meshActor);

	return meshActor;
}

void RPR::GLTF::Import::FLevelImporter::SetupLights(UWorld* World, RPR::FScene Scene, RPR::GLTF::FImageResourcesPtr ImageResources)
{
	TArray<RPR::FLight> lights;
	RPR::FResult status = RPR::GLTF::Import::GetLights(lights);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Cannot get lights from RPR scene"));
		return;
	}

	for (int32 i = 0; i < lights.Num(); ++i)
	{
		SetupLight(World, lights[i], i, ImageResources);
	}
}

void RPR::GLTF::Import::FLevelImporter::SetupLight(UWorld* World, RPR::FLight Light, int32 LightIndex, RPR::GLTF::FImageResourcesPtr ImageResources)
{
	FString actorName;
	RPR::FResult status = RPR::Light::GetObjectName(Light, actorName);
	if (RPR::IsResultFailed(status) || actorName.IsEmpty())
	{
		actorName = FString::Printf(TEXT("light_%d"), LightIndex);
	}

	ELightType lightType;
	status = RPR::Light::GetLightType(Light, lightType);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Could not get the light type"));
		return;
	}

	if (!RPR::GLTF::Import::FRPRLightDataToLightComponent::IsLightSupported(lightType))
	{
		UE_LOG(LogLevelImporter, Warning, TEXT("Light type (%d) not supported"), (uint32) lightType);
		return;
	}

	UE_LOG(LogLevelImporter, Log, TEXT("--- Light : %s"), *actorName);
	
	AActor* lightActor = CreateLightActor(World, *actorName, lightType);
	if (lightActor != nullptr)
	{
		ULightComponentBase* lightComponent = Cast<ULightComponentBase>(lightActor->GetComponentByClass(ULightComponentBase::StaticClass()));

		RPR::GLTF::Import::FRPRLightDataToLightComponent::Setup(Light, lightComponent, ImageResources, lightActor);
		UpdateTranslationScaleAccordingToImportSettings(lightActor);
	}
}

AActor* RPR::GLTF::Import::FLevelImporter::CreateLightActor(UWorld* World, const FName& ActorName, RPR::ELightType LightType)
{
	FActorSpawnParameters asp;
	asp.Name = ActorName;
	AActor* actor = nullptr; 

	switch (LightType)
	{
		case RPR::ELightType::Point:
		actor = World->SpawnActor<APointLight>(asp);
		break;;

		case RPR::ELightType::Directional:
		actor = World->SpawnActor<ADirectionalLight>(asp);
		break;

		case RPR::ELightType::Spot:
		actor = World->SpawnActor<ASpotLight>(asp);
		break;

		case RPR::ELightType::Environment:
		case RPR::ELightType::Sky:
		actor = CreateOrGetSkyLight(World, asp);
		break;

		default:
		break;
	}

	return actor;
}

AActor* RPR::GLTF::Import::FLevelImporter::CreateOrGetSkyLight(UWorld* World, const FActorSpawnParameters& ActorSpawnParameters)
{
	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), actors);

	if (actors.Num() > 0)
	{
		return (Cast<ASkyLight>(actors[0]));
	}

	return World->SpawnActor<ASkyLight>(ActorSpawnParameters);
}

void RPR::GLTF::Import::FLevelImporter::SetupCameras(UWorld* World, RPR::FScene Scene)
{
	TArray<RPR::FCamera> cameras;
	RPR::FResult status = RPR::GLTF::Import::GetCameras(cameras);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Cannot get cameras from RPR scene"));
		return;
	}

	for (int32 i = 0; i < cameras.Num(); ++i)
	{
		SetupCamera(World, cameras[i], i);
	}
}

void RPR::GLTF::Import::FLevelImporter::SetupCamera(UWorld* World, RPR::FCamera Camera, int32 CameraIndex)
{
	FString actorName;
	RPR::FResult status = RPR::Camera::GetObjectName(Camera, actorName);
	if (RPR::IsResultFailed(status) || actorName.IsEmpty())
	{
		actorName = FString::Printf(TEXT("camera_%d"), CameraIndex);
	}

	UE_LOG(LogLevelImporter, Log, TEXT("--- Camera : %s"), *actorName);

	FActorSpawnParameters asp;
	asp.Name = *actorName;
	asp.ObjectFlags = RF_Public | RF_Standalone | RF_Transactional;
	ACameraActor* cameraActor = World->SpawnActor<ACameraActor>(asp);
	UCameraComponent* cameraComponent = cameraActor->GetCameraComponent();

	RPR::GLTF::Import::FRPRCameraDataToCameraComponent::Setup(Camera, cameraComponent, cameraActor);
	UpdateTranslationScaleAccordingToImportSettings(cameraActor);
}

void RPR::GLTF::Import::FLevelImporter::SetupHierarchy(const TArray<AActor*>& Actors)
{
	if (Actors.Num() == 0)
	{
		return;
	}

	UWorld* world = Actors[0]->GetWorld();
	check(world);

	TArray<FShape> shapes;
	RPR::FResult status = RPR::GLTF::Import::GetShapes(shapes);
	if (RPR::IsResultFailed(status))
	{
		UE_LOG(LogLevelImporter, Error, TEXT("Cannot get shapes in the scene"));
		return;
	}
	check(shapes.Num() == Actors.Num());
	
	TMap<FString, AActor*> groups;

	// Bind shapes to the groups
	for (int32 shapeIndex = 0; shapeIndex < shapes.Num(); ++shapeIndex)
	{
		FString groupName;
		status = RPR::GLTF::Group::GetParentGroupFromShape(shapes[shapeIndex], groupName);
		if (RPR::IsResultFailed(status))
		{
			UE_LOG(LogLevelImporter, Error, TEXT("Cannot get parent group from shape"));
			continue;
		}

		// If no parent group
		if (groupName.IsEmpty())
		{
			continue;
		}

		UE_LOG(LogLevelImporter, Log, TEXT("%s <- %s"), *groupName, *RPR::Shape::GetName(shapes[shapeIndex]));

		if (!groups.Contains(groupName))
		{
			CreateGroupActor(world, groupName, groups);
		}

		AActor* shapeActor = Actors[shapeIndex];
		shapeActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);

		AActor* groupActor = groups[groupName];
		const bool bWeldSimulatedBodies = false;
		shapeActor->AttachToActor(groupActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, bWeldSimulatedBodies));
	}

	SetupGroupHierarchy(groups);
}

AActor* RPR::GLTF::Import::FLevelImporter::CreateGroupActor(UWorld* World, const FString& GroupName, TMap<FString, AActor*>& Groups)
{
	FActorSpawnParameters asp;
	{
		asp.Name = *GroupName;
		asp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	}
	AActor* groupActor = World->SpawnActor<AActor>(asp);
	groupActor->SetActorLabel(GroupName, false);

	USceneComponent* RootComponent = NewObject<USceneComponent>(groupActor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
	RootComponent->Mobility = EComponentMobility::Movable;
	RootComponent->bVisualizeComponent = true;

	groupActor->SetRootComponent(RootComponent);
	groupActor->AddInstanceComponent(RootComponent);

	RootComponent->RegisterComponent();
	Groups.Add(GroupName, groupActor);

	// Find parent group and bind to it
	FString parentGroupName;
	RPR::FResult status = RPR::GLTF::Group::GetParentGroupFromGroup(GroupName, parentGroupName);
	if (RPR::IsResultSuccess(status) && !parentGroupName.IsEmpty())
	{
		CreateGroupActor(World, parentGroupName, Groups);
	}

	return groupActor;
}

void RPR::GLTF::Import::FLevelImporter::SetupGroupHierarchy(TMap<FString, AActor*>& Groups)
{
	RPR::FResult status;

	for (TPair<FString, AActor*>& group : Groups)
	{
		FString parentGroupName;
		status = RPR::GLTF::Group::GetParentGroupFromGroup(group.Key, parentGroupName);

		if (RPR::IsResultFailed(status) || parentGroupName.IsEmpty())
		{
			continue;
		}

		AActor** parentGroupActorPtr = Groups.Find(parentGroupName);
		if (parentGroupActorPtr != nullptr)
		{
			AActor* parentGroupActor = *parentGroupActorPtr;
			AActor* childGroupActor = group.Value;

			const bool bWieldSimulatedBodies = false;
			childGroupActor->AttachToActor(parentGroupActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, bWieldSimulatedBodies));
		}
	}
}

void RPR::GLTF::Import::FLevelImporter::UpdateTransformAccordingToImportSettings(AActor* Actor)
{
	FTransform transform = Actor->GetTransform();
	UpdateTransformAccordingToImportSettings(transform);
	Actor->SetActorTransform(transform);
}

void RPR::GLTF::Import::FLevelImporter::UpdateTransformAccordingToImportSettings(FTransform& InOutTransform)
{
	UpdateTranslationScaleAccordingToImportSettings(InOutTransform);

	UGTLFImportSettings* gltfSettings = GetMutableDefault<UGTLFImportSettings>();
	FQuat rotation = gltfSettings->Rotation.Quaternion();
	rotation = FQuat(rotation.X, rotation.Z, rotation.Y, rotation.W);
	InOutTransform.SetRotation(InOutTransform.GetRotation() * rotation);
}

void RPR::GLTF::Import::FLevelImporter::UpdateTranslationScaleAccordingToImportSettings(AActor* Actor)
{
	FTransform transform = Actor->GetTransform();
	UpdateTranslationScaleAccordingToImportSettings(transform);
	Actor->SetActorTransform(transform);
}

void RPR::GLTF::Import::FLevelImporter::UpdateTranslationScaleAccordingToImportSettings(FTransform& InOutTransform)
{
	const int32 centimeterInMeter = 100;
	UGTLFImportSettings* gltfSettings = GetMutableDefault<UGTLFImportSettings>();
	InOutTransform.ScaleTranslation(gltfSettings->ScaleFactor * centimeterInMeter);
}

