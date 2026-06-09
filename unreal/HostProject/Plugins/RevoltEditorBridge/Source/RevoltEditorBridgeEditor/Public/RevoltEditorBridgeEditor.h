#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"

class FJsonObject;
class FSocket;
class AActor;
class FProperty;
class UMaterialInstanceConstant;
class UMaterialInterface;
class UObject;
class UClass;
class UBlueprint;
class URevoltBiomeDataAsset;
class URevoltArenaEnemyData;
class URevoltArenaWeaponData;
class URevoltArenaWaveData;
class URevoltZombieEnemyData;
class URevoltZombieWaveData;
class ARevoltLandGenActor;
struct FEdGraphPinType;

struct FRevoltBridgeClient
{
	FSocket* Socket = nullptr;
	TArray<uint8> Buffer;
	double AcceptedTimeSeconds = 0.0;
};

struct FRevoltPendingMutation
{
	FString RequestId;
	FString Command;
	FString Target;
	FString Summary;
	TSharedPtr<FJsonObject> Params;
	TArray<TSharedPtr<FJsonObject>> Diff;
	FString RiskLevel = TEXT("Low");
	FString Timestamp;
	int32 HistoryIndex = INDEX_NONE;
};

struct FRevoltDryRunDiffItem
{
	FString CommandId;
	FString CommandName;
	FString TargetType;
	FString TargetPath;
	FString ChangeType;
	FString Property;
	FString OldValue;
	FString NewValue;
	FString RiskLevel = TEXT("Low");
	FString PermissionLevel = TEXT("SafeEdit");
	FString ApprovalStatus = TEXT("Pending");
};

struct FRevoltCommandHistoryEntry
{
	FString Timestamp;
	FString Command;
	FString Target;
	FString Status;
	FString ResultSummary;
};

class FRevoltEditorBridgeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OpenRevoltBridgeTab();
	TSharedRef<class SDockTab> SpawnRevoltBridgeTab(const class FSpawnTabArgs& SpawnTabArgs);

	FReply StartBridge();
	FReply StopBridge();
	bool TickBridge(float DeltaTime);
	void CloseClient(FRevoltBridgeClient& Client);
	void CloseAllClients();
	void PollClient(FRevoltBridgeClient& Client, bool& bShouldClose);
	bool TryHandleHttpRequest(FRevoltBridgeClient& Client, bool& bShouldClose);

	FString HandleCommandJson(const FString& RequestJson);
	TSharedPtr<FJsonObject> RouteCommand(const FString& RequestId, const FString& Command, const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandlePing() const;
	TSharedPtr<FJsonObject> HandleGetProjectSummary(FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleGetOpenLevelSummary(FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleGetSelectedActors(FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleFindAssets(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleRunProjectAudit(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleAuditCurrentLevel(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleAuditSelectedActors(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleAuditBlueprints(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleAuditAssets(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleAuditGeneratedContent(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleGenerateFixPlan(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleExportAuditReport(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleMutationCommand(const FString& RequestId, const FString& Command, const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> ApplyMutationCommand(const FString& Command, const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSpawnActor(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleDuplicateSelectedActors(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleDeleteGeneratedActorsOnly(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCreateFolder(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCreateDataAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleBulkEditDataAssets(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSaveAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSaveGeneratedAssets(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCreateBlueprintClass(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSetBlueprintDefaultValue(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleGetBlueprintCompileStatus(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleCreateBiomeAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleUpdateBiomeAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleReadBiomeAsset(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleValidateBiomeAsset(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleExportBiomeAssetJson(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleLandGenValidate(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleLandGenGeneratePreview(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenClearPreview(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenRandomizeSeed(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenSpawnBiomeContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenClearGeneratedContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenBakeGeneratedContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleLandGenGetGeneratedSummary(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> HandleCreateArenaShooterTemplate(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleConfigureWeaponData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleConfigureEnemyData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSpawnTestArena(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleCreateZombieShooterTemplate(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleConfigureZombieData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleConfigureWeaponRecoil(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleSpawnZombieTestArena(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	TSharedPtr<FJsonObject> HandleConfigureWaveData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage);
	bool IsMutationCommand(const FString& Command) const;
	bool HasMutationPermission(const TSharedPtr<FJsonObject>& Params) const;
	bool GetDryRunFlag(const TSharedPtr<FJsonObject>& Params) const;
	bool GetApprovedFlag(const TSharedPtr<FJsonObject>& Params) const;
	AActor* FindActorFromParams(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	AActor* FindActorByPathOrName(const FString& ActorIdentifier) const;
	UClass* ResolveSpawnClass(const FString& ClassName, FString& ErrorCode, FString& ErrorMessage) const;
	bool ReadVectorObject(const TSharedPtr<FJsonObject>& Params, const FString& FieldName, FVector& OutVector, bool& bHasField, FString& ErrorCode, FString& ErrorMessage) const;
	bool ReadRotatorObject(const TSharedPtr<FJsonObject>& Params, const FString& FieldName, FRotator& OutRotator, bool& bHasField, FString& ErrorCode, FString& ErrorMessage) const;
	bool AreTransformValuesFinite(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const;
	TSharedPtr<FJsonObject> MakeTransformObject(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const;
	FString GetActorTargetName(AActor* Actor) const;
	bool IsGeneratedActor(AActor* Actor) const;
	bool IsGeneratedAssetPath(const FString& AssetPath) const;
	bool NormalizeGeneratedPackagePath(const FString& InputPath, FString& OutPackagePath, FString& ErrorCode, FString& ErrorMessage) const;
	bool NormalizeGeneratedAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	bool EnsureGeneratedFolderExists(const FString& PackagePath, bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const;
	UClass* ResolveApprovedDataAssetClass(const FString& ClassName, FString& ErrorCode, FString& ErrorMessage) const;
	UObject* LoadAssetForMutation(const FString& AssetPath, UClass* ExpectedClass, FString& ErrorCode, FString& ErrorMessage) const;
	bool ApplyEditablePropertyValue(
		UObject* Object,
		const FString& PropertyName,
		const TSharedPtr<FJsonValue>& Value,
		FString& BeforeValue,
		FString& AfterValue,
		bool bDryRun,
		FString& ErrorCode,
		FString& ErrorMessage
	) const;
	bool SavePackageForAsset(UObject* Asset, FString& ErrorCode, FString& ErrorMessage) const;
	bool NormalizeGeneratedBlueprintAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	UClass* ResolveSafeBlueprintParentClass(const FString& ParentClassName, FString& ErrorCode, FString& ErrorMessage) const;
	UClass* ResolveAllowedBlueprintComponentClass(const FString& ComponentClassName, FString& ErrorCode, FString& ErrorMessage) const;
	UBlueprint* LoadBlueprintForMutation(const FString& BlueprintPath, bool bAllowUserBlueprintEdit, FString& ErrorCode, FString& ErrorMessage) const;
	bool NormalizeGeneratedBiomeAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	bool IsGeneratedBiomeAssetPath(const FString& AssetPath) const;
	URevoltBiomeDataAsset* LoadBiomeAsset(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	bool ReadBiomeJsonFromParams(const TSharedPtr<FJsonObject>& Params, TSharedPtr<FJsonObject>& OutBiomeJson, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> MakeBiomeValidationResult(const URevoltBiomeDataAsset* BiomeAsset) const;
	ARevoltLandGenActor* FindLandGenActorFromParams(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const;
	TSharedPtr<FJsonObject> MakeLandGenStatusObject(const ARevoltLandGenActor* LandGenActor) const;
	void AddSpawnPreviewToResult(const ARevoltLandGenActor* LandGenActor, TSharedPtr<FJsonObject> Result) const;
	bool IsValidBlueprintVariableName(const FString& VariableName, FString& ErrorCode, FString& ErrorMessage) const;
	bool MakeSupportedBlueprintPinType(const FString& VariableType, FEdGraphPinType& OutPinType, FString& ErrorCode, FString& ErrorMessage) const;
	FString GetBlueprintCompileStatusString(const UBlueprint* Blueprint) const;
	bool CompileBlueprintAndValidate(UBlueprint* Blueprint, FString& ErrorCode, FString& ErrorMessage) const;
	void AddAuditIssue(
		TArray<TSharedPtr<FJsonValue>>& Issues,
		const FString& Source,
		const FString& Severity,
		const FString& Code,
		const FString& AffectedType,
		const FString& Affected,
		const FString& Message,
		const FString& RecommendedFix,
		int32& IssueCounter
	) const;
	TSharedPtr<FJsonObject> MakeFixPlanItemFromIssue(const TSharedPtr<FJsonObject>& Issue) const;
	void AddActorAuditIssues(AActor* Actor, TArray<TSharedPtr<FJsonValue>>& Issues, const FString& Source, int32& IssueCounter) const;
	bool EnsureArenaTemplateFolders(bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const;
	UObject* CreateArenaDataAsset(const FString& PackagePath, const FString& AssetName, UClass* AssetClass, bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const;
	URevoltArenaWeaponData* LoadArenaWeaponData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	URevoltArenaEnemyData* LoadArenaEnemyData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	bool IsArenaTemplateAssetPath(const FString& AssetPath) const;
	bool EnsureZombieTemplateFolders(bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const;
	bool IsZombieTemplateAssetPath(const FString& AssetPath) const;
	URevoltZombieEnemyData* LoadZombieEnemyData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	URevoltZombieWaveData* LoadZombieWaveData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const;
	TArray<TSharedPtr<FJsonObject>> BuildDryRunDiff(
		const FString& RequestId,
		const FString& Command,
		const TSharedPtr<FJsonObject>& Params,
		const TSharedPtr<FJsonObject>& Preview,
		const FString& Target,
		FString& OutRiskLevel
	) const;
	TSharedPtr<FJsonObject> MakeDiffItemObject(const FRevoltDryRunDiffItem& Item) const;
	FString JsonValueToDisplayString(const TSharedPtr<FJsonValue>& Value) const;
	FString JsonObjectToDisplayString(const TSharedPtr<FJsonObject>& Object) const;
	FString InferRiskLevel(const FString& Command, const FString& TargetPath, int32 ChangeCount) const;
	FString InferPermissionLevel(const TSharedPtr<FJsonObject>& Params) const;
	void AttachDryRunDiff(TSharedPtr<FJsonObject> Result, const TArray<TSharedPtr<FJsonObject>>& Diff, const FString& RiskLevel, bool bDryRun) const;
	int32 AddHistoryEntry(const FString& Command, const FString& Target, const FString& Status, const FString& ResultSummary);
	void UpdateHistoryEntry(int32 HistoryIndex, const FString& Status, const FString& ResultSummary);
	void RefreshSelectedDiffIndex();
	FReply ApproveSelectedChange();
	FReply RejectSelectedChange();
	FReply ApproveSelectedCommand();
	FReply RejectSelectedCommand();
	FReply ClearRejectedDiffs();
	FReply CopyDiffAsJson();
	FReply AuditCurrentLevelFromUi();
	FReply AuditSelectedActorsFromUi();
	FReply AuditAssetsFromUi();
	FReply AuditBlueprintsFromUi();
	FReply AuditGeneratedContentFromUi();
	FReply RunFullProjectAuditFromUi();
	FReply GenerateFixPlanFromUi();
	FReply ExportAuditJsonFromUi();
	void StoreAuditResult(const FString& Command, const TSharedPtr<FJsonObject>& Result, const FString& ErrorCode, const FString& ErrorMessage);
	FReply ClearCommandHistory();
	FString BuildApprovalQueueText() const;
	FString BuildProjectAuditText() const;
	FString BuildDryRunDiffText() const;
	FString BuildCommandHistoryText() const;
	FString MakeSuccessResponse(const FString& RequestId, const TSharedPtr<FJsonObject>& Result) const;
	FString MakeErrorResponse(const FString& RequestId, const FString& ErrorCode, const FString& ErrorMessage) const;
	void SetLastCommand(const FString& Command);
	void SetLastResponseSummary(const FString& Summary);

	bool bBridgeRunning = false;
	uint16 BridgePort = 8765;
	FSocket* ListenSocket = nullptr;
	FTSTicker::FDelegateHandle BridgeTickerHandle;
	TArray<FRevoltBridgeClient> Clients;
	FString BridgeStatus = TEXT("Stopped");
	FString LocalEndpoint = TEXT("http://127.0.0.1:8765/");
	FString LastCommand = TEXT("None");
	FString LastResponseSummary = TEXT("No responses yet");
	FString OfflineModeStatus = TEXT("Enabled - localhost only");
	TSharedPtr<FJsonObject> LastAuditReport;
	FString LastAuditSummary = TEXT("No audit has been run yet.");
	TArray<FRevoltPendingMutation> ApprovalQueue;
	TArray<FRevoltCommandHistoryEntry> CommandHistory;
	int32 SelectedApprovalIndex = INDEX_NONE;
	int32 SelectedDiffIndex = INDEX_NONE;
	TArray<TSharedPtr<FJsonObject>> RejectedDiffItems;
};
