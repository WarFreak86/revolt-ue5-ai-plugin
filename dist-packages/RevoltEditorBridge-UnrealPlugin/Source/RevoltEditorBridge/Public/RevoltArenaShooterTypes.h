#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "RevoltArenaShooterTypes.generated.h"

class UBoxComponent;
class UAIPerceptionComponent;

UENUM(BlueprintType)
enum class ERevoltArenaPickupType : uint8
{
	Health,
	Ammo,
	Score
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltArenaWaveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	TSoftObjectPtr<class URevoltArenaEnemyData> EnemyData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "1"))
	int32 Count = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float SpawnDelaySeconds = 0.25f;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltWeaponRecoilData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float VerticalKick = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float HorizontalKick = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float RecoverySpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float SpreadDegrees = 1.5f;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltAmmoReloadData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0"))
	int32 ReserveAmmo = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float ReloadSeconds = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bAllowPartialReload = false;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltArenaWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.01"))
	float FireRate = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "1"))
	int32 PelletCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float Range = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	FRevoltWeaponRecoilData Recoil;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	FRevoltAmmoReloadData Ammo;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltArenaEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	TSubclassOf<AActor> EnemyActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float MoveSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float ContactDamage = 10.0f;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltArenaWaveData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	TArray<FRevoltArenaWaveEntry> Waves;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltZombieEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TSubclassOf<AActor> ZombieActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float MoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float BiteDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float SightRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float HearingRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bUsesSimpleCover = false;
};

USTRUCT(BlueprintType)
struct REVOLTEDITORBRIDGE_API FRevoltZombieWaveEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TSoftObjectPtr<URevoltZombieEnemyData> ZombieData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "1"))
	int32 Count = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter", meta = (ClampMin = "0.0"))
	float SpawnDelaySeconds = 0.35f;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltZombieWaveData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TArray<FRevoltZombieWaveEntry> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bEnableDayNightScaling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bEnableWeatherScaling = true;
};

UCLASS(BlueprintType)
class REVOLTEDITORBRIDGE_API URevoltZombieObjectiveData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	FText ObjectiveText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bRequiresExtraction = true;
};

UCLASS(ClassGroup = (Revolt), meta = (BlueprintSpawnableComponent))
class REVOLTEDITORBRIDGE_API URevoltArenaHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URevoltArenaHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Arena Shooter")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bEnableReplicationSupport = false;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Arena Shooter")
	void ResetHealth();

	UFUNCTION(BlueprintCallable, Category = "Revolt|Arena Shooter")
	bool ApplyArenaDamage(float DamageAmount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS(ClassGroup = (Revolt), meta = (BlueprintSpawnableComponent))
class REVOLTEDITORBRIDGE_API URevoltArenaDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URevoltArenaDamageComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bEnableReplicationSupport = false;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Arena Shooter")
	bool ApplyDamageToActor(AActor* TargetActor) const;
};

UCLASS(ClassGroup = (Revolt), meta = (BlueprintSpawnableComponent))
class REVOLTEDITORBRIDGE_API URevoltInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URevoltInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TArray<TObjectPtr<URevoltArenaWeaponData>> Weapons;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	int32 ActiveWeaponIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	URevoltArenaWeaponData* GetActiveWeapon() const;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltArenaPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARevoltArenaPlayerCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Arena Shooter")
	TObjectPtr<URevoltArenaHealthComponent> HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	TObjectPtr<URevoltArenaWeaponData> StartingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltInventoryComponent> InventoryComponent;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltZombieEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ARevoltZombieEnemyCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltArenaHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltArenaDamageComponent> DamageComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltZombieEnemyData> ZombieData;

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	FVector FindSimpleCoverLocation(const FVector& ThreatLocation) const;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltArenaPickupActor : public AActor
{
	GENERATED_BODY()

public:
	ARevoltArenaPickupActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Arena Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	ERevoltArenaPickupType PickupType = ERevoltArenaPickupType::Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	float Amount = 25.0f;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltArenaObjectiveActor : public AActor
{
	GENERATED_BODY()

public:
	ARevoltArenaObjectiveActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Arena Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	FText ObjectiveText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	bool bCompleted = false;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltZombieExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	ARevoltZombieExtractionZone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float Radius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	bool bExtractionActive = true;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltZombieObjectiveActor : public ARevoltArenaObjectiveActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltZombieObjectiveData> ObjectiveData;
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltZombieDirectorActor : public AActor
{
	GENERATED_BODY()

public:
	ARevoltZombieDirectorActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float TimeOfDayHours = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	FName WeatherState = TEXT("Clear");

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	void ApplyDayNightCycleHook(float NewTimeOfDayHours);

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	void ApplyWeatherHook(FName NewWeatherState);

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	bool SaveGamePlaceholder();

	UFUNCTION(BlueprintCallable, Category = "Revolt|Zombie Shooter")
	bool LoadGamePlaceholder();
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltZombieWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARevoltZombieWaveSpawner();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Zombie Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	TObjectPtr<URevoltZombieWaveData> WaveData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Zombie Shooter")
	float SpawnRadius = 900.0f;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Zombie Shooter")
	void SpawnZombieWave();
};

UCLASS(BlueprintType, Blueprintable)
class REVOLTEDITORBRIDGE_API ARevoltArenaWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ARevoltArenaWaveSpawner();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolt|Arena Shooter")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	TObjectPtr<URevoltArenaWaveData> WaveData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revolt|Arena Shooter")
	float SpawnRadius = 700.0f;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Revolt|Arena Shooter")
	void SpawnTestWave();
};
