#pragma once

#include "UVProjectionAlgorithmBase.h"

class FUVProjectionSphericalAlgo : public FUVProjectionAlgorithmBase
{
public:

	struct FSettings : public FUVProjectionAlgorithmBase::FUVProjectionGlobalSettings
	{
		FSettings();

		FVector	SphereCenter;
		FQuat	SphereRotation;
	};

public:

	void	SetSettings(const FSettings& InSettings);

	virtual void StartAlgorithm() override;
	virtual void Finalize() override;

	static void	ProjectVerticesOnSphere(const FSettings& InSettings, 
										TArray<FVector>& VertexPositions, 
										TArray<uint32>& WedgeIndices, 
										TArray<FVector2D>& OutUVs);

	static void	ProjectVertexOnSphere(const FSettings& InSettings, const FVector& Vertex, FVector2D& OutUV);
	static void	FixInvalidTriangles(TArray<FVector>& VertexPositions, TArray<uint32>& WedgeIndices, TArray<FVector2D>& UVs);
	static bool IsTriangleValid(const FVector2D& PointA, const FVector2D& PointB, const FVector2D& PointC);

private:

	TArray<FVector2D>	NewUVs;
	FSettings			Settings;

};