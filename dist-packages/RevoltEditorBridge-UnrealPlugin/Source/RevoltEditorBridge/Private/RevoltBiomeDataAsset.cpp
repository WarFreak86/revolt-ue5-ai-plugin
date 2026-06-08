#include "RevoltBiomeDataAsset.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace RevoltBiomeJson
{
	static TSharedRef<FJsonObject> MakeRange(float Min, float Max)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("min"), Min);
		Object->SetNumberField(TEXT("max"), Max);
		return Object;
	}

	static FVector2D ReadRange(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, const FVector2D& DefaultValue)
	{
		const TSharedPtr<FJsonObject>* RangeObject = nullptr;
		if (Object.IsValid() && Object->TryGetObjectField(FieldName, RangeObject) && RangeObject && RangeObject->IsValid())
		{
			return FVector2D(
				static_cast<float>((*RangeObject)->GetNumberField(TEXT("min"))),
				static_cast<float>((*RangeObject)->GetNumberField(TEXT("max"))));
		}
		return DefaultValue;
	}

	static TSharedRef<FJsonObject> MakeVector2D(const FVector2D& Value)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("x"), Value.X);
		Object->SetNumberField(TEXT("y"), Value.Y);
		return Object;
	}

	static FVector2D ReadVector2D(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, const FVector2D& DefaultValue)
	{
		const TSharedPtr<FJsonObject>* VectorObject = nullptr;
		if (Object.IsValid() && Object->TryGetObjectField(FieldName, VectorObject) && VectorObject && VectorObject->IsValid())
		{
			return FVector2D(
				static_cast<float>((*VectorObject)->GetNumberField(TEXT("x"))),
				static_cast<float>((*VectorObject)->GetNumberField(TEXT("y"))));
		}
		return DefaultValue;
	}

	static TSharedRef<FJsonObject> MakeColor(const FLinearColor& Color)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("r"), Color.R);
		Object->SetNumberField(TEXT("g"), Color.G);
		Object->SetNumberField(TEXT("b"), Color.B);
		Object->SetNumberField(TEXT("a"), Color.A);
		return Object;
	}

	static FLinearColor ReadColor(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, const FLinearColor& DefaultValue)
	{
		const TSharedPtr<FJsonObject>* ColorObject = nullptr;
		if (Object.IsValid() && Object->TryGetObjectField(FieldName, ColorObject) && ColorObject && ColorObject->IsValid())
		{
			return FLinearColor(
				static_cast<float>((*ColorObject)->GetNumberField(TEXT("r"))),
				static_cast<float>((*ColorObject)->GetNumberField(TEXT("g"))),
				static_cast<float>((*ColorObject)->GetNumberField(TEXT("b"))),
				static_cast<float>((*ColorObject)->GetNumberField(TEXT("a"))));
		}
		return DefaultValue;
	}

	static bool IsValidRange(const FVector2D& Range, float MinAllowed, float MaxAllowed)
	{
		return FMath::IsFinite(Range.X) && FMath::IsFinite(Range.Y) && Range.X >= MinAllowed && Range.Y <= MaxAllowed && Range.X <= Range.Y;
	}
}

FString URevoltBiomeDataAsset::ToJsonString() const
{
	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(ToJsonObject(), Writer);
	return Output;
}

bool URevoltBiomeDataAsset::FromJsonString(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	return FJsonSerializer::Deserialize(Reader, JsonObject) && FromJsonObject(JsonObject);
}

TSharedRef<FJsonObject> URevoltBiomeDataAsset::ToJsonObject() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("biome_name"), BiomeName);
	Root->SetStringField(TEXT("description"), Description);

	TSharedRef<FJsonObject> Terrain = MakeShared<FJsonObject>();
	Terrain->SetNumberField(TEXT("terrain_resolution"), TerrainSettings.TerrainResolution);
	Terrain->SetNumberField(TEXT("seed"), TerrainSettings.Seed);
	Terrain->SetNumberField(TEXT("noise_scale"), TerrainSettings.NoiseScale);
	Terrain->SetObjectField(TEXT("height_range"), RevoltBiomeJson::MakeRange(TerrainSettings.MinHeight, TerrainSettings.MaxHeight));
	Terrain->SetObjectField(TEXT("slope_range"), RevoltBiomeJson::MakeRange(TerrainSettings.MinSlope, TerrainSettings.MaxSlope));
	Root->SetObjectField(TEXT("terrain_settings"), Terrain);

	TArray<TSharedPtr<FJsonValue>> TerrainStampValues;
	for (const FRevoltTerrainStamp& Stamp : TerrainStamps)
	{
		TSharedRef<FJsonObject> StampObject = MakeShared<FJsonObject>();
		StampObject->SetStringField(TEXT("name"), Stamp.Name);
		StampObject->SetObjectField(TEXT("location"), RevoltBiomeJson::MakeVector2D(Stamp.Location));
		StampObject->SetNumberField(TEXT("radius"), Stamp.Radius);
		StampObject->SetNumberField(TEXT("height_offset"), Stamp.HeightOffset);
		StampObject->SetNumberField(TEXT("falloff"), Stamp.Falloff);
		TerrainStampValues.Add(MakeShared<FJsonValueObject>(StampObject));
	}
	Root->SetArrayField(TEXT("terrain_stamps"), TerrainStampValues);

	TArray<TSharedPtr<FJsonValue>> FoliageValues;
	for (const FRevoltFoliageSpawnRule& Rule : FoliageSpawnRules)
	{
		TSharedRef<FJsonObject> RuleObject = MakeShared<FJsonObject>();
		RuleObject->SetStringField(TEXT("rule_name"), Rule.RuleName);
		RuleObject->SetStringField(TEXT("mesh"), Rule.Mesh.ToSoftObjectPath().ToString());
		RuleObject->SetNumberField(TEXT("density"), Rule.Density);
		RuleObject->SetObjectField(TEXT("scale_range"), RevoltBiomeJson::MakeRange(Rule.ScaleRange.X, Rule.ScaleRange.Y));
		RuleObject->SetObjectField(TEXT("slope_range"), RevoltBiomeJson::MakeRange(Rule.SlopeRange.X, Rule.SlopeRange.Y));
		RuleObject->SetObjectField(TEXT("height_range"), RevoltBiomeJson::MakeRange(Rule.HeightRange.X, Rule.HeightRange.Y));
		FoliageValues.Add(MakeShared<FJsonValueObject>(RuleObject));
	}
	Root->SetArrayField(TEXT("foliage_spawn_rules"), FoliageValues);

	TArray<TSharedPtr<FJsonValue>> StructureValues;
	for (const FRevoltStructureSpawnRule& Rule : StructureSpawnRules)
	{
		TSharedRef<FJsonObject> RuleObject = MakeShared<FJsonObject>();
		RuleObject->SetStringField(TEXT("rule_name"), Rule.RuleName);
		RuleObject->SetStringField(TEXT("actor_class"), Rule.ActorClass.ToSoftObjectPath().ToString());
		RuleObject->SetNumberField(TEXT("density"), Rule.Density);
		RuleObject->SetObjectField(TEXT("scale_range"), RevoltBiomeJson::MakeRange(Rule.ScaleRange.X, Rule.ScaleRange.Y));
		RuleObject->SetObjectField(TEXT("slope_range"), RevoltBiomeJson::MakeRange(Rule.SlopeRange.X, Rule.SlopeRange.Y));
		RuleObject->SetObjectField(TEXT("height_range"), RevoltBiomeJson::MakeRange(Rule.HeightRange.X, Rule.HeightRange.Y));
		StructureValues.Add(MakeShared<FJsonValueObject>(RuleObject));
	}
	Root->SetArrayField(TEXT("structure_spawn_rules"), StructureValues);

	TArray<TSharedPtr<FJsonValue>> ObjectiveValues;
	for (const FRevoltObjectiveSpawnRule& Rule : ObjectiveSpawnRules)
	{
		TSharedRef<FJsonObject> RuleObject = MakeShared<FJsonObject>();
		RuleObject->SetStringField(TEXT("rule_name"), Rule.RuleName);
		RuleObject->SetStringField(TEXT("actor_class"), Rule.ActorClass.ToSoftObjectPath().ToString());
		RuleObject->SetNumberField(TEXT("min_count"), Rule.MinCount);
		RuleObject->SetNumberField(TEXT("max_count"), Rule.MaxCount);
		RuleObject->SetObjectField(TEXT("slope_range"), RevoltBiomeJson::MakeRange(Rule.SlopeRange.X, Rule.SlopeRange.Y));
		RuleObject->SetObjectField(TEXT("height_range"), RevoltBiomeJson::MakeRange(Rule.HeightRange.X, Rule.HeightRange.Y));
		ObjectiveValues.Add(MakeShared<FJsonValueObject>(RuleObject));
	}
	Root->SetArrayField(TEXT("objective_spawn_rules"), ObjectiveValues);

	TSharedRef<FJsonObject> Navigation = MakeShared<FJsonObject>();
	Navigation->SetBoolField(TEXT("generate_navigation"), NavigationRules.bGenerateNavigation);
	Navigation->SetNumberField(TEXT("agent_radius"), NavigationRules.AgentRadius);
	Navigation->SetNumberField(TEXT("max_navigable_slope"), NavigationRules.MaxNavigableSlope);
	Navigation->SetBoolField(TEXT("avoid_water"), NavigationRules.bAvoidWater);
	Root->SetObjectField(TEXT("navigation_rules"), Navigation);

	TSharedRef<FJsonObject> Lighting = MakeShared<FJsonObject>();
	Lighting->SetNumberField(TEXT("directional_light_intensity"), LightingRules.DirectionalLightIntensity);
	Lighting->SetNumberField(TEXT("sky_light_intensity"), LightingRules.SkyLightIntensity);
	Lighting->SetNumberField(TEXT("fog_density"), LightingRules.FogDensity);
	Lighting->SetObjectField(TEXT("atmosphere_tint"), RevoltBiomeJson::MakeColor(LightingRules.AtmosphereTint));
	Root->SetObjectField(TEXT("lighting_rules"), Lighting);

	return Root;
}

bool URevoltBiomeDataAsset::FromJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	JsonObject->TryGetStringField(TEXT("biome_name"), BiomeName);
	JsonObject->TryGetStringField(TEXT("name"), BiomeName);
	JsonObject->TryGetStringField(TEXT("description"), Description);

	const TSharedPtr<FJsonObject>* Terrain = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("terrain_settings"), Terrain) && Terrain && Terrain->IsValid())
	{
		TerrainSettings.TerrainResolution = (*Terrain)->GetIntegerField(TEXT("terrain_resolution"));
		TerrainSettings.Seed = (*Terrain)->GetIntegerField(TEXT("seed"));
		TerrainSettings.NoiseScale = static_cast<float>((*Terrain)->GetNumberField(TEXT("noise_scale")));
		const FVector2D HeightRange = RevoltBiomeJson::ReadRange(*Terrain, TEXT("height_range"), FVector2D(TerrainSettings.MinHeight, TerrainSettings.MaxHeight));
		TerrainSettings.MinHeight = HeightRange.X;
		TerrainSettings.MaxHeight = HeightRange.Y;
		const FVector2D SlopeRange = RevoltBiomeJson::ReadRange(*Terrain, TEXT("slope_range"), FVector2D(TerrainSettings.MinSlope, TerrainSettings.MaxSlope));
		TerrainSettings.MinSlope = SlopeRange.X;
		TerrainSettings.MaxSlope = SlopeRange.Y;
	}

	TerrainStamps.Empty();
	const TArray<TSharedPtr<FJsonValue>>* TerrainStampValues = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("terrain_stamps"), TerrainStampValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *TerrainStampValues)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			if (!Object.IsValid())
			{
				continue;
			}
			FRevoltTerrainStamp& Stamp = TerrainStamps.AddDefaulted_GetRef();
			Object->TryGetStringField(TEXT("name"), Stamp.Name);
			Stamp.Location = RevoltBiomeJson::ReadVector2D(Object, TEXT("location"), Stamp.Location);
			Stamp.Radius = static_cast<float>(Object->GetNumberField(TEXT("radius")));
			Stamp.HeightOffset = static_cast<float>(Object->GetNumberField(TEXT("height_offset")));
			Stamp.Falloff = static_cast<float>(Object->GetNumberField(TEXT("falloff")));
		}
	}

	FoliageSpawnRules.Empty();
	const TArray<TSharedPtr<FJsonValue>>* FoliageValues = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("foliage_spawn_rules"), FoliageValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *FoliageValues)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			if (!Object.IsValid())
			{
				continue;
			}
			FRevoltFoliageSpawnRule& Rule = FoliageSpawnRules.AddDefaulted_GetRef();
			Object->TryGetStringField(TEXT("rule_name"), Rule.RuleName);
			FString MeshPath;
			if (Object->TryGetStringField(TEXT("mesh"), MeshPath))
			{
				Rule.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
			}
			Rule.Density = static_cast<float>(Object->GetNumberField(TEXT("density")));
			Rule.ScaleRange = RevoltBiomeJson::ReadRange(Object, TEXT("scale_range"), Rule.ScaleRange);
			Rule.SlopeRange = RevoltBiomeJson::ReadRange(Object, TEXT("slope_range"), Rule.SlopeRange);
			Rule.HeightRange = RevoltBiomeJson::ReadRange(Object, TEXT("height_range"), Rule.HeightRange);
		}
	}

	StructureSpawnRules.Empty();
	const TArray<TSharedPtr<FJsonValue>>* StructureValues = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("structure_spawn_rules"), StructureValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *StructureValues)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			if (!Object.IsValid())
			{
				continue;
			}
			FRevoltStructureSpawnRule& Rule = StructureSpawnRules.AddDefaulted_GetRef();
			Object->TryGetStringField(TEXT("rule_name"), Rule.RuleName);
			FString ClassPath;
			if (Object->TryGetStringField(TEXT("actor_class"), ClassPath))
			{
				Rule.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath));
			}
			Rule.Density = static_cast<float>(Object->GetNumberField(TEXT("density")));
			Rule.ScaleRange = RevoltBiomeJson::ReadRange(Object, TEXT("scale_range"), Rule.ScaleRange);
			Rule.SlopeRange = RevoltBiomeJson::ReadRange(Object, TEXT("slope_range"), Rule.SlopeRange);
			Rule.HeightRange = RevoltBiomeJson::ReadRange(Object, TEXT("height_range"), Rule.HeightRange);
		}
	}

	ObjectiveSpawnRules.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ObjectiveValues = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("objective_spawn_rules"), ObjectiveValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ObjectiveValues)
		{
			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			if (!Object.IsValid())
			{
				continue;
			}
			FRevoltObjectiveSpawnRule& Rule = ObjectiveSpawnRules.AddDefaulted_GetRef();
			Object->TryGetStringField(TEXT("rule_name"), Rule.RuleName);
			FString ClassPath;
			if (Object->TryGetStringField(TEXT("actor_class"), ClassPath))
			{
				Rule.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath));
			}
			Rule.MinCount = Object->GetIntegerField(TEXT("min_count"));
			Rule.MaxCount = Object->GetIntegerField(TEXT("max_count"));
			Rule.SlopeRange = RevoltBiomeJson::ReadRange(Object, TEXT("slope_range"), Rule.SlopeRange);
			Rule.HeightRange = RevoltBiomeJson::ReadRange(Object, TEXT("height_range"), Rule.HeightRange);
		}
	}

	const TSharedPtr<FJsonObject>* Navigation = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("navigation_rules"), Navigation) && Navigation && Navigation->IsValid())
	{
		NavigationRules.bGenerateNavigation = (*Navigation)->GetBoolField(TEXT("generate_navigation"));
		NavigationRules.AgentRadius = static_cast<float>((*Navigation)->GetNumberField(TEXT("agent_radius")));
		NavigationRules.MaxNavigableSlope = static_cast<float>((*Navigation)->GetNumberField(TEXT("max_navigable_slope")));
		NavigationRules.bAvoidWater = (*Navigation)->GetBoolField(TEXT("avoid_water"));
	}

	const TSharedPtr<FJsonObject>* Lighting = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("lighting_rules"), Lighting) && Lighting && Lighting->IsValid())
	{
		LightingRules.DirectionalLightIntensity = static_cast<float>((*Lighting)->GetNumberField(TEXT("directional_light_intensity")));
		LightingRules.SkyLightIntensity = static_cast<float>((*Lighting)->GetNumberField(TEXT("sky_light_intensity")));
		LightingRules.FogDensity = static_cast<float>((*Lighting)->GetNumberField(TEXT("fog_density")));
		LightingRules.AtmosphereTint = RevoltBiomeJson::ReadColor(*Lighting, TEXT("atmosphere_tint"), LightingRules.AtmosphereTint);
	}

	return true;
}

void URevoltBiomeDataAsset::ValidateBiome(TArray<FString>& OutErrors) const
{
	if (BiomeName.TrimStartAndEnd().IsEmpty())
	{
		OutErrors.Add(TEXT("Biome name cannot be empty."));
	}
	if (TerrainSettings.Seed < 0)
	{
		OutErrors.Add(TEXT("Seed must be zero or greater."));
	}
	if (TerrainSettings.TerrainResolution < 64 || TerrainSettings.TerrainResolution > 8192)
	{
		OutErrors.Add(TEXT("Terrain resolution must be between 64 and 8192."));
	}
	if (!FMath::IsPowerOfTwo(TerrainSettings.TerrainResolution))
	{
		OutErrors.Add(TEXT("Terrain resolution must be a power of two."));
	}
	if (!RevoltBiomeJson::IsValidRange(FVector2D(TerrainSettings.MinHeight, TerrainSettings.MaxHeight), -100000.0f, 100000.0f))
	{
		OutErrors.Add(TEXT("Terrain height range is invalid."));
	}
	if (!RevoltBiomeJson::IsValidRange(FVector2D(TerrainSettings.MinSlope, TerrainSettings.MaxSlope), 0.0f, 90.0f))
	{
		OutErrors.Add(TEXT("Terrain slope range must be between 0 and 90 degrees."));
	}
	if (!FMath::IsFinite(TerrainSettings.NoiseScale) || TerrainSettings.NoiseScale <= 0.0f)
	{
		OutErrors.Add(TEXT("Terrain noise scale must be greater than zero."));
	}

	for (int32 Index = 0; Index < FoliageSpawnRules.Num(); ++Index)
	{
		const FRevoltFoliageSpawnRule& Rule = FoliageSpawnRules[Index];
		if (Rule.Mesh.IsNull())
		{
			OutErrors.Add(FString::Printf(TEXT("Foliage rule %d is missing a mesh reference."), Index));
		}
		if (!FMath::IsFinite(Rule.Density) || Rule.Density < 0.0f || Rule.Density > 1000.0f)
		{
			OutErrors.Add(FString::Printf(TEXT("Foliage rule %d has invalid density."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.ScaleRange, 0.01f, 100.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Foliage rule %d has invalid scale range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.SlopeRange, 0.0f, 90.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Foliage rule %d has invalid slope range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.HeightRange, -100000.0f, 100000.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Foliage rule %d has invalid height range."), Index));
		}
	}

	for (int32 Index = 0; Index < StructureSpawnRules.Num(); ++Index)
	{
		const FRevoltStructureSpawnRule& Rule = StructureSpawnRules[Index];
		if (Rule.ActorClass.IsNull())
		{
			OutErrors.Add(FString::Printf(TEXT("Structure rule %d is missing an actor class reference."), Index));
		}
		if (!FMath::IsFinite(Rule.Density) || Rule.Density < 0.0f || Rule.Density > 1000.0f)
		{
			OutErrors.Add(FString::Printf(TEXT("Structure rule %d has invalid density."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.ScaleRange, 0.01f, 100.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Structure rule %d has invalid scale range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.SlopeRange, 0.0f, 90.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Structure rule %d has invalid slope range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.HeightRange, -100000.0f, 100000.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Structure rule %d has invalid height range."), Index));
		}
	}

	for (int32 Index = 0; Index < ObjectiveSpawnRules.Num(); ++Index)
	{
		const FRevoltObjectiveSpawnRule& Rule = ObjectiveSpawnRules[Index];
		if (Rule.ActorClass.IsNull())
		{
			OutErrors.Add(FString::Printf(TEXT("Objective rule %d is missing an actor class reference."), Index));
		}
		if (Rule.MinCount < 0 || Rule.MaxCount < Rule.MinCount)
		{
			OutErrors.Add(FString::Printf(TEXT("Objective rule %d has invalid count range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.SlopeRange, 0.0f, 90.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Objective rule %d has invalid slope range."), Index));
		}
		if (!RevoltBiomeJson::IsValidRange(Rule.HeightRange, -100000.0f, 100000.0f))
		{
			OutErrors.Add(FString::Printf(TEXT("Objective rule %d has invalid height range."), Index));
		}
	}

	if (!FMath::IsFinite(NavigationRules.AgentRadius) || NavigationRules.AgentRadius <= 0.0f)
	{
		OutErrors.Add(TEXT("Navigation agent radius must be greater than zero."));
	}
	if (!FMath::IsFinite(NavigationRules.MaxNavigableSlope) || NavigationRules.MaxNavigableSlope < 0.0f || NavigationRules.MaxNavigableSlope > 90.0f)
	{
		OutErrors.Add(TEXT("Navigation max slope must be between 0 and 90 degrees."));
	}
	if (!FMath::IsFinite(LightingRules.FogDensity) || LightingRules.FogDensity < 0.0f)
	{
		OutErrors.Add(TEXT("Fog density must be zero or greater."));
	}
}
