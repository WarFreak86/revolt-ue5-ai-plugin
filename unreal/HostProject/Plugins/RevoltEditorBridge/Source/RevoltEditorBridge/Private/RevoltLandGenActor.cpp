#include "RevoltLandGenActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "RevoltBiomeDataAsset.h"

namespace RevoltLandGen
{
	static const FName GeneratedTag(TEXT("Revolt.Generated"));
	static const FName PreviewTag(TEXT("Revolt.LandPreview"));
	static const FName BakedTag(TEXT("Revolt.Baked"));

	static FString SanitizeBiomeName(const FString& Name)
	{
		FString Sanitized = Name;
		Sanitized.ReplaceInline(TEXT(" "), TEXT("_"));
		Sanitized.ReplaceInline(TEXT("."), TEXT("_"));
		return Sanitized.IsEmpty() ? TEXT("UnnamedBiome") : Sanitized;
	}
}

ARevoltLandGenActor::ARevoltLandGenActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

bool ARevoltLandGenActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ARevoltLandGenActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPreviewMode && !PreviewZones.IsEmpty())
	{
		DrawDebugPreview();
	}
}

void ARevoltLandGenActor::ValidateBiome()
{
	LastValidationErrors = GetValidationErrors();
}

void ARevoltLandGenActor::GenerateTerrainPreview()
{
	ValidateBiome();
	if (!LastValidationErrors.IsEmpty())
	{
		return;
	}

	ClearTerrainPreview();
	BuildDeterministicPreviewPlan();

	for (const FRevoltLandGenPreviewZone& Zone : PreviewZones)
	{
		CreatePreviewComponent(Zone);
	}
}

void ARevoltLandGenActor::ClearTerrainPreview()
{
	TArray<UActorComponent*> ComponentsToDestroy;
	for (UActorComponent* Component : GetComponents())
	{
		if (IsGeneratedPreviewComponent(Component))
		{
			ComponentsToDestroy.Add(Component);
		}
	}

	for (UActorComponent* Component : ComponentsToDestroy)
	{
		Component->DestroyComponent();
	}

	PreviewComponents.Empty();
	PreviewZones.Empty();
}

void ARevoltLandGenActor::RandomizeSeed()
{
	SeedOverride = FMath::Rand();
}

void ARevoltLandGenActor::SpawnBiomeContent()
{
	ValidateBiome();
	if (!LastValidationErrors.IsEmpty())
	{
		return;
	}

	ClearGeneratedContent();
	SpawnPreviewPlan(BuildBiomeSpawnPreview());
}

void ARevoltLandGenActor::ClearGeneratedContent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> ActorsToDestroy;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (IsGeneratedBiomeActor(Actor))
		{
			ActorsToDestroy.Add(Actor);
		}
	}

	for (AActor* Actor : ActorsToDestroy)
	{
		Actor->Destroy();
	}
}

void ARevoltLandGenActor::BakeGeneratedContent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (IsGeneratedBiomeActor(Actor))
		{
			Actor->Tags.Remove(RevoltLandGen::GeneratedTag);
			Actor->Tags.AddUnique(RevoltLandGen::BakedTag);
#if WITH_EDITOR
			Actor->SetFolderPath(FName(*GeneratedContentFolderName));
#endif
		}
	}
}

FString ARevoltLandGenActor::GetGeneratedSummary() const
{
	return FString::Printf(TEXT("Biome=%s Seed=%d GeneratedActors=%d PreviewPlacements=%d"),
		Biome ? *Biome->BiomeName : TEXT("None"),
		GetEffectiveSeed(),
		GetGeneratedActorCount(),
		BuildBiomeSpawnPreview().Num());
}

TArray<FString> ARevoltLandGenActor::GetValidationErrors() const
{
	TArray<FString> Errors;
	if (!Biome)
	{
		Errors.Add(TEXT("Land generation actor requires a biome asset."));
	}
	else
	{
		Biome->ValidateBiome(Errors);
	}

	if (!FMath::IsFinite(GenerationBounds.X) || !FMath::IsFinite(GenerationBounds.Y) || !FMath::IsFinite(GenerationBounds.Z) ||
		GenerationBounds.X <= 0.0f || GenerationBounds.Y <= 0.0f || GenerationBounds.Z <= 0.0f)
	{
		Errors.Add(TEXT("GenerationBounds must be finite positive extents."));
	}

	if (GeneratedContentFolderName.TrimStartAndEnd().IsEmpty())
	{
		Errors.Add(TEXT("GeneratedContentFolderName cannot be empty."));
	}

	return Errors;
}

int32 ARevoltLandGenActor::GetEffectiveSeed() const
{
	if (SeedOverride > 0)
	{
		return SeedOverride;
	}

	return Biome ? Biome->TerrainSettings.Seed : 0;
}

TArray<FRevoltBiomeSpawnPreview> ARevoltLandGenActor::BuildBiomeSpawnPreview() const
{
	TArray<FRevoltBiomeSpawnPreview> SpawnPreview;
	if (!Biome)
	{
		return SpawnPreview;
	}

	FRandomStream Stream(GetEffectiveSeed());
	TArray<FVector> AcceptedLocations;

	auto MakeLocation = [this, &Stream]() -> FVector
	{
		return GetActorTransform().TransformPosition(FVector(
			Stream.FRandRange(-GenerationBounds.X * 0.5f, GenerationBounds.X * 0.5f),
			Stream.FRandRange(-GenerationBounds.Y * 0.5f, GenerationBounds.Y * 0.5f),
			Stream.FRandRange(-GenerationBounds.Z * 0.5f, GenerationBounds.Z * 0.5f)));
	};

	auto TryAddPreview = [this, &SpawnPreview, &AcceptedLocations, &Stream, &MakeLocation](
		const FString& RuleName,
		const FString& RuleType,
		const FString& AssetOrClassPath,
		const FVector2D& ScaleRange,
		const FVector2D& SlopeRange,
		const FVector2D& HeightRange,
		int32 DesiredCount,
		float MinDistance)
	{
		constexpr float AssumedPreviewSlopeDegrees = 0.0f;
		if (AssumedPreviewSlopeDegrees < SlopeRange.X || AssumedPreviewSlopeDegrees > SlopeRange.Y)
		{
			return;
		}

		const int32 Attempts = FMath::Max(DesiredCount * 12, 12);
		for (int32 Attempt = 0; Attempt < Attempts && DesiredCount > 0; ++Attempt)
		{
			FVector Location = MakeLocation();
			if (Location.Z < HeightRange.X || Location.Z > HeightRange.Y)
			{
				continue;
			}
			const FVector Extent(80.0f, 80.0f, 80.0f);
			if (!PassesPlacementFilters(Location, Extent, AcceptedLocations, MinDistance))
			{
				continue;
			}

			FRevoltBiomeSpawnPreview& Preview = SpawnPreview.AddDefaulted_GetRef();
			Preview.RuleName = RuleName;
			Preview.RuleType = RuleType;
			Preview.Location = Location;
			Preview.Rotation = FRotator(0.0f, Stream.FRandRange(0.0f, 360.0f), 0.0f);
			const float UniformScale = Stream.FRandRange(ScaleRange.X, ScaleRange.Y);
			Preview.Scale = FVector(UniformScale);
			Preview.AssetOrClassPath = AssetOrClassPath;
			AcceptedLocations.Add(Location);
			--DesiredCount;
		}
	};

	for (const FRevoltFoliageSpawnRule& Rule : Biome->FoliageSpawnRules)
	{
		const int32 DesiredCount = FMath::Clamp(FMath::RoundToInt(Rule.Density * 3.0f), 0, 50);
		TryAddPreview(Rule.RuleName, TEXT("foliage"), Rule.Mesh.ToSoftObjectPath().ToString(), Rule.ScaleRange, Rule.SlopeRange, Rule.HeightRange, DesiredCount, 250.0f);
	}

	for (const FRevoltStructureSpawnRule& Rule : Biome->StructureSpawnRules)
	{
		const int32 DesiredCount = FMath::Clamp(FMath::RoundToInt(Rule.Density), 0, 20);
		TryAddPreview(Rule.RuleName, TEXT("structure"), Rule.ActorClass.ToSoftObjectPath().ToString(), Rule.ScaleRange, Rule.SlopeRange, Rule.HeightRange, DesiredCount, 750.0f);
	}

	for (const FRevoltObjectiveSpawnRule& Rule : Biome->ObjectiveSpawnRules)
	{
		const int32 DesiredCount = FMath::Clamp(Rule.MaxCount, 0, 20);
		TryAddPreview(Rule.RuleName, TEXT("objective"), Rule.ActorClass.ToSoftObjectPath().ToString(), FVector2D(1.0f, 1.0f), Rule.SlopeRange, Rule.HeightRange, DesiredCount, 1000.0f);
	}

	return SpawnPreview;
}

int32 ARevoltLandGenActor::GetGeneratedActorCount() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		if (IsGeneratedBiomeActor(*ActorIterator))
		{
			++Count;
		}
	}
	return Count;
}

void ARevoltLandGenActor::BuildDeterministicPreviewPlan()
{
	PreviewZones.Empty();

	FRandomStream Stream(GetEffectiveSeed());
	const FVector TerrainExtent(GenerationBounds.X * 0.5f, GenerationBounds.Y * 0.5f, GenerationBounds.Z * 0.05f);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		FRevoltLandGenPreviewZone& Zone = PreviewZones.AddDefaulted_GetRef();
		Zone.ZoneType = ERevoltLandGenPreviewZoneType::TerrainRegion;
		Zone.Center = FVector(
			Stream.FRandRange(-GenerationBounds.X * 0.35f, GenerationBounds.X * 0.35f),
			Stream.FRandRange(-GenerationBounds.Y * 0.35f, GenerationBounds.Y * 0.35f),
			0.0f);
		Zone.Extent = TerrainExtent * Stream.FRandRange(0.18f, 0.32f);
		Zone.Label = FString::Printf(TEXT("TerrainRegion_%d"), Index);
	}

	const int32 CandidateCount = Biome ? FMath::Clamp(Biome->StructureSpawnRules.Num() + Biome->ObjectiveSpawnRules.Num(), 3, 12) : 4;
	for (int32 Index = 0; Index < CandidateCount; ++Index)
	{
		FRevoltLandGenPreviewZone& Zone = PreviewZones.AddDefaulted_GetRef();
		Zone.ZoneType = ERevoltLandGenPreviewZoneType::CandidateSpawnZone;
		Zone.Center = FVector(
			Stream.FRandRange(-GenerationBounds.X * 0.45f, GenerationBounds.X * 0.45f),
			Stream.FRandRange(-GenerationBounds.Y * 0.45f, GenerationBounds.Y * 0.45f),
			100.0f);
		Zone.Extent = FVector(Stream.FRandRange(250.0f, 900.0f), Stream.FRandRange(250.0f, 900.0f), 120.0f);
		Zone.Label = FString::Printf(TEXT("CandidateSpawn_%d"), Index);
	}

	const int32 BlockedCount = Biome ? FMath::Clamp(Biome->TerrainStamps.Num() + 2, 2, 8) : 3;
	for (int32 Index = 0; Index < BlockedCount; ++Index)
	{
		FRevoltLandGenPreviewZone& Zone = PreviewZones.AddDefaulted_GetRef();
		Zone.ZoneType = ERevoltLandGenPreviewZoneType::BlockedZone;
		Zone.Center = FVector(
			Stream.FRandRange(-GenerationBounds.X * 0.48f, GenerationBounds.X * 0.48f),
			Stream.FRandRange(-GenerationBounds.Y * 0.48f, GenerationBounds.Y * 0.48f),
			80.0f);
		Zone.Extent = FVector(Stream.FRandRange(350.0f, 1200.0f), Stream.FRandRange(350.0f, 1200.0f), 100.0f);
		Zone.Label = FString::Printf(TEXT("BlockedZone_%d"), Index);
	}
}

void ARevoltLandGenActor::CreatePreviewComponent(const FRevoltLandGenPreviewZone& Zone)
{
	UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this, *FString::Printf(TEXT("RevoltPreview_%s"), *Zone.Label));
	if (!BoxComponent)
	{
		return;
	}

	BoxComponent->ComponentTags.AddUnique(RevoltLandGen::GeneratedTag);
	BoxComponent->ComponentTags.AddUnique(RevoltLandGen::PreviewTag);
	BoxComponent->CreationMethod = EComponentCreationMethod::Instance;
	BoxComponent->SetBoxExtent(Zone.Extent);
	BoxComponent->SetRelativeLocation(Zone.Center);
	BoxComponent->ShapeColor = GetZoneColor(Zone.ZoneType);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->SetHiddenInGame(false);
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->RegisterComponent();
	AddInstanceComponent(BoxComponent);
	PreviewComponents.Add(BoxComponent);
}

void ARevoltLandGenActor::SpawnPreviewPlan(const TArray<FRevoltBiomeSpawnPreview>& SpawnPreview)
{
	for (const FRevoltBiomeSpawnPreview& Preview : SpawnPreview)
	{
		SpawnEditableActorFromPreview(Preview);
	}
}

AActor* ARevoltLandGenActor::SpawnEditableActorFromPreview(const FRevoltBiomeSpawnPreview& Preview)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* SpawnedActor = nullptr;
	if (Preview.RuleType == TEXT("foliage"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Preview.Location, Preview.Rotation);
		if (MeshActor)
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(FSoftObjectPath(Preview.AssetOrClassPath).TryLoad()))
			{
				MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			}
			SpawnedActor = MeshActor;
		}
	}
	else
	{
		UClass* ActorClass = Cast<UClass>(FSoftObjectPath(Preview.AssetOrClassPath).TryLoad());
		if (!ActorClass)
		{
			ActorClass = AActor::StaticClass();
		}
		SpawnedActor = World->SpawnActor<AActor>(ActorClass, Preview.Location, Preview.Rotation);
	}

	if (!SpawnedActor)
	{
		return nullptr;
	}

	SpawnedActor->SetActorScale3D(Preview.Scale);
	SpawnedActor->Tags.AddUnique(RevoltLandGen::GeneratedTag);
	SpawnedActor->Tags.AddUnique(GetBiomeTag());
	SpawnedActor->Tags.AddUnique(FName(*FString::Printf(TEXT("Revolt.Rule.%s"), *Preview.RuleType)));
#if WITH_EDITOR
	SpawnedActor->SetFolderPath(FName(*GeneratedContentFolderName));
	SpawnedActor->SetActorLabel(FString::Printf(TEXT("Revolt_%s_%s"), *Preview.RuleType, *Preview.RuleName));
#endif
	return SpawnedActor;
}

void ARevoltLandGenActor::DrawDebugPreview() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DrawDebugBox(World, GetActorLocation(), GenerationBounds * 0.5f, GetActorQuat(), FColor::Cyan, false, -1.0f, 0, 4.0f);
	for (const FRevoltLandGenPreviewZone& Zone : PreviewZones)
	{
		DrawDebugBox(World, GetActorTransform().TransformPosition(Zone.Center), Zone.Extent, GetActorQuat(), GetZoneColor(Zone.ZoneType), false, -1.0f, 0, 3.0f);
	}
}

FColor ARevoltLandGenActor::GetZoneColor(ERevoltLandGenPreviewZoneType ZoneType) const
{
	switch (ZoneType)
	{
	case ERevoltLandGenPreviewZoneType::TerrainRegion:
		return FColor::Green;
	case ERevoltLandGenPreviewZoneType::CandidateSpawnZone:
		return FColor::Yellow;
	case ERevoltLandGenPreviewZoneType::BlockedZone:
		return FColor::Red;
	default:
		return FColor::White;
	}
}

bool ARevoltLandGenActor::IsGeneratedPreviewComponent(const UActorComponent* Component) const
{
	return Component &&
		Component->ComponentTags.Contains(RevoltLandGen::GeneratedTag) &&
		Component->ComponentTags.Contains(RevoltLandGen::PreviewTag);
}

bool ARevoltLandGenActor::IsGeneratedBiomeActor(const AActor* Actor) const
{
	return Actor &&
		Actor->Tags.Contains(RevoltLandGen::GeneratedTag) &&
		Actor->Tags.Contains(GetBiomeTag());
}

FName ARevoltLandGenActor::GetBiomeTag() const
{
	const FString BiomeName = Biome ? RevoltLandGen::SanitizeBiomeName(Biome->BiomeName) : TEXT("Unassigned");
	return FName(*FString::Printf(TEXT("Revolt.Biome.%s"), *BiomeName));
}

bool ARevoltLandGenActor::IsPlacementInsideBounds(const FVector& Location) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(Location);
	return FMath::Abs(LocalLocation.X) <= GenerationBounds.X * 0.5f &&
		FMath::Abs(LocalLocation.Y) <= GenerationBounds.Y * 0.5f &&
		FMath::Abs(LocalLocation.Z) <= GenerationBounds.Z * 0.5f;
}

bool ARevoltLandGenActor::PassesPlacementFilters(const FVector& Location, const FVector& Extent, const TArray<FVector>& ExistingLocations, float MinDistance) const
{
	if (!IsPlacementInsideBounds(Location))
	{
		return false;
	}

	for (const FVector& ExistingLocation : ExistingLocations)
	{
		if (FVector::DistSquared(Location, ExistingLocation) < FMath::Square(MinDistance))
		{
			return false;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RevoltLandGenPlacement), false, this);
	return !World->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeBox(Extent), QueryParams);
}
