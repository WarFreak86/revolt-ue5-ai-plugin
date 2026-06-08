#include "RevoltArenaShooterTypes.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"

URevoltArenaHealthComponent::URevoltArenaHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URevoltArenaHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
}

bool URevoltArenaHealthComponent::ApplyArenaDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f)
	{
		return false;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	return CurrentHealth <= 0.0f;
}

void URevoltArenaHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URevoltArenaHealthComponent, CurrentHealth);
}

URevoltArenaDamageComponent::URevoltArenaDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool URevoltArenaDamageComponent::ApplyDamageToActor(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	URevoltArenaHealthComponent* HealthComponent = TargetActor->FindComponentByClass<URevoltArenaHealthComponent>();
	return HealthComponent ? HealthComponent->ApplyArenaDamage(Damage) : false;
}

URevoltInventoryComponent::URevoltInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

URevoltArenaWeaponData* URevoltInventoryComponent::GetActiveWeapon() const
{
	return Weapons.IsValidIndex(ActiveWeaponIndex) ? Weapons[ActiveWeaponIndex] : nullptr;
}

ARevoltArenaPlayerCharacter::ARevoltArenaPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComponent = CreateDefaultSubobject<URevoltArenaHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<URevoltInventoryComponent>(TEXT("InventoryComponent"));
}

ARevoltZombieEnemyCharacter::ARevoltZombieEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComponent = CreateDefaultSubobject<URevoltArenaHealthComponent>(TEXT("HealthComponent"));
	DamageComponent = CreateDefaultSubobject<URevoltArenaDamageComponent>(TEXT("DamageComponent"));
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	Tags.AddUnique(TEXT("Revolt.Generated"));
	Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
}

FVector ARevoltZombieEnemyCharacter::FindSimpleCoverLocation(const FVector& ThreatLocation) const
{
	const FVector AwayFromThreat = (GetActorLocation() - ThreatLocation).GetSafeNormal();
	const FVector CoverDirection = AwayFromThreat.IsNearlyZero() ? -GetActorForwardVector() : AwayFromThreat;
	return GetActorLocation() + CoverDirection * 600.0f;
}

ARevoltArenaPickupActor::ARevoltArenaPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Tags.AddUnique(TEXT("Revolt.Generated"));
}

ARevoltArenaObjectiveActor::ARevoltArenaObjectiveActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	ObjectiveText = FText::FromString(TEXT("Survive the test wave"));
	Tags.AddUnique(TEXT("Revolt.Generated"));
}

ARevoltZombieExtractionZone::ARevoltZombieExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Tags.AddUnique(TEXT("Revolt.Generated"));
	Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
}

ARevoltZombieDirectorActor::ARevoltZombieDirectorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Tags.AddUnique(TEXT("Revolt.Generated"));
	Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
}

void ARevoltZombieDirectorActor::ApplyDayNightCycleHook(float NewTimeOfDayHours)
{
	TimeOfDayHours = FMath::Fmod(FMath::Max(0.0f, NewTimeOfDayHours), 24.0f);
}

void ARevoltZombieDirectorActor::ApplyWeatherHook(FName NewWeatherState)
{
	WeatherState = NewWeatherState;
}

bool ARevoltZombieDirectorActor::SaveGamePlaceholder()
{
	return false;
}

bool ARevoltZombieDirectorActor::LoadGamePlaceholder()
{
	return false;
}

ARevoltZombieWaveSpawner::ARevoltZombieWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Tags.AddUnique(TEXT("Revolt.Generated"));
	Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
}

void ARevoltZombieWaveSpawner::SpawnZombieWave()
{
	if (!WaveData || !GetWorld())
	{
		return;
	}

	int32 SpawnedCount = 0;
	for (const FRevoltZombieWaveEntry& Entry : WaveData->Waves)
	{
		URevoltZombieEnemyData* ZombieData = Entry.ZombieData.LoadSynchronous();
		UClass* SpawnClass = ZombieData && ZombieData->ZombieActorClass ? ZombieData->ZombieActorClass.Get() : ARevoltZombieEnemyCharacter::StaticClass();
		const int32 Count = FMath::Max(1, Entry.Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = (static_cast<float>(SpawnedCount) / static_cast<float>(Count)) * UE_TWO_PI;
			const FVector Offset(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 80.0f);
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnClass, GetActorLocation() + Offset, FRotator::ZeroRotator);
			if (SpawnedActor)
			{
				SpawnedActor->Tags.AddUnique(TEXT("Revolt.Generated"));
				SpawnedActor->Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
				SpawnedActor->Tags.AddUnique(TEXT("Revolt.Zombie"));
				++SpawnedCount;
			}
		}
	}
}

ARevoltArenaWaveSpawner::ARevoltArenaWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	Tags.AddUnique(TEXT("Revolt.Generated"));
}

void ARevoltArenaWaveSpawner::SpawnTestWave()
{
	if (!WaveData || !GetWorld())
	{
		return;
	}

	int32 SpawnedCount = 0;
	for (const FRevoltArenaWaveEntry& Entry : WaveData->Waves)
	{
		URevoltArenaEnemyData* EnemyData = Entry.EnemyData.LoadSynchronous();
		UClass* SpawnClass = EnemyData && EnemyData->EnemyActorClass ? EnemyData->EnemyActorClass.Get() : AActor::StaticClass();
		const int32 Count = FMath::Max(1, Entry.Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = (static_cast<float>(SpawnedCount) / static_cast<float>(Count)) * UE_TWO_PI;
			const FVector Offset(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 80.0f);
			AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnClass, GetActorLocation() + Offset, FRotator::ZeroRotator);
			if (SpawnedActor)
			{
				SpawnedActor->Tags.AddUnique(TEXT("Revolt.Generated"));
				SpawnedActor->Tags.AddUnique(TEXT("Revolt.Template.ArenaShooter"));
				++SpawnedCount;
			}
		}
	}
}
