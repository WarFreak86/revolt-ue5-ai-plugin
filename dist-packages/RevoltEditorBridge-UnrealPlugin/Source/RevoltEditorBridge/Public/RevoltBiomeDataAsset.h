#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RevoltBiomeDataAsset.generated.h"

class AActor;
class FJsonObject;
class UStaticMesh;

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltTerrainSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	int32 TerrainResolution = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	float MinHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	float MaxHeight = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	float MinSlope = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	float MaxSlope = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain")
	float NoiseScale = 1.0f;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltTerrainStamp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain Stamp")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain Stamp")
	FVector2D Location = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain Stamp")
	float Radius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain Stamp")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Terrain Stamp")
	float Falloff = 0.5f;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltFoliageSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	FString RuleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	float Density = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	FVector2D ScaleRange = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	FVector2D SlopeRange = FVector2D(0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Foliage")
	FVector2D HeightRange = FVector2D(0.0f, 2000.0f);
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltStructureSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	FString RuleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	float Density = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	FVector2D ScaleRange = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	FVector2D SlopeRange = FVector2D(0.0f, 30.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Structure")
	FVector2D HeightRange = FVector2D(0.0f, 2000.0f);
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltObjectiveSpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	FString RuleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	int32 MaxCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	FVector2D SlopeRange = FVector2D(0.0f, 25.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Objective")
	FVector2D HeightRange = FVector2D(0.0f, 2000.0f);
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltNavigationRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Navigation")
	bool bGenerateNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Navigation")
	float AgentRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Navigation")
	float MaxNavigableSlope = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Navigation")
	bool bAvoidWater = true;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltLightingRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Lighting")
	float DirectionalLightIntensity = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Lighting")
	float SkyLightIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Lighting")
	float FogDensity = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Lighting")
	FLinearColor AtmosphereTint = FLinearColor::White;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltBiomeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	FString BiomeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	FRevoltTerrainSettings TerrainSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	TArray<FRevoltTerrainStamp> TerrainStamps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	TArray<FRevoltFoliageSpawnRule> FoliageSpawnRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	TArray<FRevoltStructureSpawnRule> StructureSpawnRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	TArray<FRevoltObjectiveSpawnRule> ObjectiveSpawnRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	FRevoltNavigationRules NavigationRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome")
	FRevoltLightingRules LightingRules;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Biome")
	FString ToJsonString() const;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Biome")
	bool FromJsonString(const FString& JsonString);

	TSharedRef<FJsonObject> ToJsonObject() const;
	bool FromJsonObject(const TSharedPtr<FJsonObject>& JsonObject);
	void ValidateBiome(TArray<FString>& OutErrors) const;
};
