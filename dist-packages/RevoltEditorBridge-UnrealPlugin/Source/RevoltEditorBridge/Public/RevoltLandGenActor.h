#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RevoltLandGenActor.generated.h"

class UBoxComponent;
class URevoltBiomeDataAsset;
class USceneComponent;

UENUM(BlueprintType)
enum class ERevoltLandGenPreviewZoneType : uint8
{
	TerrainRegion,
	CandidateSpawnZone,
	BlockedZone
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltLandGenPreviewZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Preview")
	ERevoltLandGenPreviewZoneType ZoneType = ERevoltLandGenPreviewZoneType::TerrainRegion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Preview")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Preview")
	FVector Extent = FVector(250.0f, 250.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Preview")
	FString Label;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltBiomeSpawnPreview
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FString RuleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FString RuleType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Biome Spawn")
	FString AssetOrClassPath;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API ARevoltLandGenActor : public AActor
{
	GENERATED_BODY()

public:
	ARevoltLandGenActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Generation")
	TObjectPtr<URevoltBiomeDataAsset> Biome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Generation")
	int32 SeedOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Generation")
	FVector GenerationBounds = FVector(10000.0f, 10000.0f, 2000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Generation")
	bool bPreviewMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Land Generation")
	FString GeneratedContentFolderName = TEXT("RevoltLandPreview");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Land Generation")
	TArray<FRevoltLandGenPreviewZone> PreviewZones;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Land Generation")
	TArray<FString> LastValidationErrors;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void ValidateBiome();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void GenerateTerrainPreview();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void ClearTerrainPreview();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void RandomizeSeed();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void SpawnBiomeContent();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void ClearGeneratedContent();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	void BakeGeneratedContent();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Land Generation")
	FString GetGeneratedSummary() const;

	TArray<FString> GetValidationErrors() const;
	int32 GetEffectiveSeed() const;
	TArray<FRevoltBiomeSpawnPreview> BuildBiomeSpawnPreview() const;
	int32 GetGeneratedActorCount() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Revolt|Land Generation")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> PreviewComponents;

	void BuildDeterministicPreviewPlan();
	void CreatePreviewComponent(const FRevoltLandGenPreviewZone& Zone);
	void SpawnPreviewPlan(const TArray<FRevoltBiomeSpawnPreview>& SpawnPreview);
	AActor* SpawnEditableActorFromPreview(const FRevoltBiomeSpawnPreview& Preview);
	void DrawDebugPreview() const;
	FColor GetZoneColor(ERevoltLandGenPreviewZoneType ZoneType) const;
	bool IsGeneratedPreviewComponent(const UActorComponent* Component) const;
	bool IsGeneratedBiomeActor(const AActor* Actor) const;
	FName GetBiomeTag() const;
	bool IsPlacementInsideBounds(const FVector& Location) const;
	bool PassesPlacementFilters(const FVector& Location, const FVector& Extent, const TArray<FVector>& ExistingLocations, float MinDistance) const;
};
