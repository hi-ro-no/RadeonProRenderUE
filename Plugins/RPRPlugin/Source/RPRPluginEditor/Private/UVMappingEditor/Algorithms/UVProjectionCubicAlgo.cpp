#include "UVProjectionCubicAlgo.h"
#include "RPRPluginEditorModule.h"
#include "RPRStaticMeshEditor.h"
#include "UVUtility.h"
#include "IUVCubeLayout.h"
#include "UVCubeLayout_CubemapRight.h"
#include "RPRMeshFace.h"
#include "RPRVectorTools.h"

void FUVProjectionCubicAlgo::StartAlgorithm()
{
	FUVProjectionAlgorithmBase::StartAlgorithm();

	PrepareUVs(NewUVs);
	StartCubicProjection(RawMesh, NewUVs);
	
	FUVUtility::ShrinkUVsToBounds(NewUVs);
	FUVUtility::CenterUVs(NewUVs);

	StopAlgorithmAndRaiseCompletion(true);
}

void FUVProjectionCubicAlgo::Finalize()
{
	SetUVsOnMesh(NewUVs);
	SaveRawMesh();
}

void FUVProjectionCubicAlgo::StartCubicProjection(FRawMesh& InRawMesh, TArray<FVector2D>& OutNewUVs)
{
	TArray<uint32>& triangles = InRawMesh.WedgeIndices;
	EAxis::Type dominantAxisComponentA;
	EAxis::Type dominantAxisComponentB;
	
	FQuat inverseCubeRotation = Settings.CubeTransform.GetRotation().Inverse();

	for (int32 i = 0; i < triangles.Num(); i += 3)
	{
		int32 triA = triangles[i];
		int32 triB = triangles[i+1];
		int32 triC = triangles[i+2];

		const FVector& pA = InRawMesh.VertexPositions[triA];
		const FVector& pB = InRawMesh.VertexPositions[triB];
		const FVector& pC = InRawMesh.VertexPositions[triC];

		FVector lpA = inverseCubeRotation * pA;
		FVector lpB = inverseCubeRotation * pB;
		FVector lpC = inverseCubeRotation * pC;

		FVector faceNormal = FRPRVectorTools::CalculateFaceNormal(pA, pB, pC);
		FRPRVectorTools::GetDominantAxisComponents(faceNormal, dominantAxisComponentA, dominantAxisComponentB);

		ProjectUVAlongAxis(NewUVs, triA, dominantAxisComponentA, dominantAxisComponentB);
		ProjectUVAlongAxis(NewUVs, triB, dominantAxisComponentA, dominantAxisComponentB);
		ProjectUVAlongAxis(NewUVs, triC, dominantAxisComponentA, dominantAxisComponentB);
	}
}

void FUVProjectionCubicAlgo::ProjectUVAlongAxis(TArray<FVector2D>& UVs, int32 VertexIndex, EAxis::Type AxisComponentA, EAxis::Type AxisComponentB)
{
	FVector scale = Settings.CubeTransform.GetScale3D();
	FVector origin = Settings.CubeTransform.GetLocation();
	const FVector& vertexLocation = RawMesh.VertexPositions[VertexIndex];
	FVector localVertexLocation = Settings.CubeTransform.GetRotation().Inverse() * vertexLocation;

	TFunction<float(EAxis::Type)> getScalarAlongAxis = [this, &scale, &origin, localVertexLocation](EAxis::Type Axis)
	{
		return (0.5f + 0.25f * scale.GetComponentForAxis(Axis) * (localVertexLocation.GetComponentForAxis(Axis) - origin.GetComponentForAxis(Axis)));
	};

	FVector2D uv(
		getScalarAlongAxis(AxisComponentA),
		getScalarAlongAxis(AxisComponentB)
	);

	FUVUtility::InvertUV(uv);

	UVs.Add(uv);
}

void FUVProjectionCubicAlgo::SetSettings(const FSettings& InSettings)
{
	Settings = InSettings;
}