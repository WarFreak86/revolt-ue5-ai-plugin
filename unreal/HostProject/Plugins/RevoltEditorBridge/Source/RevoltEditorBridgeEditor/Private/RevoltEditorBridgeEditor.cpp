#include "RevoltEditorBridgeEditor.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/DataAsset.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "EdGraphSchema_K2.h"
#include "Factories/DataAssetFactory.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/WorldFactory.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Networking.h"
#include "ObjectTools.h"
#include "RevoltArenaShooterTypes.h"
#include "RevoltBiomeDataAsset.h"
#include "RevoltLandGenActor.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "RevoltEditorBridge.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Styling/CoreStyle.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "ToolMenus.h"
#include "Components/ActorComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FRevoltEditorBridgeEditorModule"

namespace RevoltEditorBridgeEditor
{
	static const FName RevoltBridgeTabName(TEXT("RevoltEditorBridge.RevoltBridge"));
}

void FRevoltEditorBridgeEditorModule::StartupModule()
{
	UE_LOG(LogRevoltEditorBridge, Log, TEXT("RevoltEditorBridge editor module started."));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		RevoltEditorBridgeEditor::RevoltBridgeTabName,
		FOnSpawnTab::CreateRaw(this, &FRevoltEditorBridgeEditorModule::SpawnRevoltBridgeTab))
		.SetDisplayName(LOCTEXT("RevoltBridgeTabTitle", "Revolt Bridge"))
		.SetTooltipText(LOCTEXT("RevoltBridgeTabTooltip", "Open the Revolt Bridge editor panel."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Editor-only menu registration is deferred until ToolMenus is available.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRevoltEditorBridgeEditorModule::RegisterMenus));
}

void FRevoltEditorBridgeEditorModule::ShutdownModule()
{
	UE_LOG(LogRevoltEditorBridge, Log, TEXT("RevoltEditorBridge editor module shut down."));

	StopBridge();

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RevoltEditorBridgeEditor::RevoltBridgeTabName);
}

void FRevoltEditorBridgeEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(
		TEXT("RevoltEditorBridge"),
		LOCTEXT("RevoltEditorBridgeToolsSection", "Revolt"));

	Section.AddMenuEntry(
		TEXT("OpenRevoltBridge"),
		LOCTEXT("OpenRevoltBridgeLabel", "Revolt Bridge"),
		LOCTEXT("OpenRevoltBridgeTooltip", "Open the Revolt Bridge tab."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FRevoltEditorBridgeEditorModule::OpenRevoltBridgeTab)));
}

void FRevoltEditorBridgeEditorModule::OpenRevoltBridgeTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(RevoltEditorBridgeEditor::RevoltBridgeTabName);
}

TSharedRef<SDockTab> FRevoltEditorBridgeEditorModule::SpawnRevoltBridgeTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder)
			.Padding(16.0f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PluginTitle", "Revolt Editor Bridge"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Bridge Status: %s"), *BridgeStatus));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Local Endpoint: %s"), *LocalEndpoint));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Last Command: %s"), *LastCommand));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Last Response Summary: %s"), *LastResponseSummary));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(FString::Printf(TEXT("Offline Mode: %s"), *OfflineModeStatus));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f)
					[
						SNew(SSeparator)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("StartBridgeButton", "Start Bridge"))
							.IsEnabled_Lambda([this]() { return !bBridgeRunning; })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::StartBridge)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("StopBridgeButton", "Stop Bridge"))
							.IsEnabled_Lambda([this]() { return bBridgeRunning; })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::StopBridge)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ProjectAuditTitle", "Project Audit"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(BuildProjectAuditText());
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AuditCurrentLevelButton", "Audit Current Level"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::AuditCurrentLevelFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AuditSelectedActorsButton", "Audit Selected Actors"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::AuditSelectedActorsFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AuditAssetsButton", "Audit Assets"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::AuditAssetsFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("AuditBlueprintsButton", "Audit Blueprints"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::AuditBlueprintsFromUi)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AuditGeneratedContentButton", "Audit Generated Content"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::AuditGeneratedContentFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("RunFullProjectAuditButton", "Run Full Project Audit"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::RunFullProjectAuditFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("GenerateFixPlanButton", "Generate Fix Plan"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::GenerateFixPlanFromUi)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("ExportAuditJsonButton", "Export Audit JSON"))
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ExportAuditJsonFromUi)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ApprovalQueueTitle", "Approval Queue"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(BuildApprovalQueueText());
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ApproveSelectedCommandButton", "Approve Selected Command"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ApproveSelectedCommand)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("RejectSelectedCommandButton", "Reject Selected Command"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::RejectSelectedCommand)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DryRunDiffViewerTitle", "Dry-Run Diff Viewer"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(BuildDryRunDiffText());
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ApproveSelectedChangeButton", "Approve Selected Change"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex) && SelectedDiffIndex != INDEX_NONE; })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ApproveSelectedChange)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("RejectSelectedChangeButton", "Reject Selected Change"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex) && SelectedDiffIndex != INDEX_NONE; })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::RejectSelectedChange)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ApproveEntireCommandButton", "Approve Entire Command"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ApproveSelectedCommand)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("RejectEntireCommandButton", "Reject Entire Command"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::RejectSelectedCommand)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 8.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ClearRejectedDiffsButton", "Clear Rejected"))
							.IsEnabled_Lambda([this]() { return !RejectedDiffItems.IsEmpty(); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ClearRejectedDiffs)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("CopyDiffAsJsonButton", "Copy Diff as JSON"))
							.IsEnabled_Lambda([this]() { return ApprovalQueue.IsValidIndex(SelectedApprovalIndex); })
							.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::CopyDiffAsJson)
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CommandHistoryTitle", "Command History"))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(BuildCommandHistoryText());
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("ClearCommandHistoryButton", "Clear Command History"))
						.OnClicked_Raw(this, &FRevoltEditorBridgeEditorModule::ClearCommandHistory)
					]
				]
			]
		];
}

FReply FRevoltEditorBridgeEditorModule::StartBridge()
{
	if (bBridgeRunning)
	{
		BridgeStatus = TEXT("Already running");
		SetLastResponseSummary(TEXT("Error BRIDGE_ALREADY_RUNNING"));
		return FReply::Handled();
	}

	FIPv4Address LoopbackAddress;
	FIPv4Address::Parse(TEXT("127.0.0.1"), LoopbackAddress);
	const FIPv4Endpoint LoopbackEndpoint(LoopbackAddress, BridgePort);

	ListenSocket = FTcpSocketBuilder(TEXT("RevoltEditorBridgeListener"))
		.AsReusable()
		.BoundToEndpoint(LoopbackEndpoint)
		.Listening(8);

	if (!ListenSocket)
	{
		BridgeStatus = TEXT("Failed to start");
		SetLastResponseSummary(TEXT("Error BRIDGE_START_FAILED"));
		UE_LOG(LogRevoltEditorBridge, Error, TEXT("Failed to start Revolt bridge on 127.0.0.1:%d."), BridgePort);
		return FReply::Handled();
	}

	ListenSocket->SetNonBlocking(true);
	bBridgeRunning = true;
	BridgeStatus = TEXT("Running");
	LocalEndpoint = FString::Printf(TEXT("http://127.0.0.1:%d/"), BridgePort);
	SetLastResponseSummary(TEXT("Bridge started on localhost"));

	// The socket is bound to 127.0.0.1 and polled lightly on the editor ticker.
	BridgeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FRevoltEditorBridgeEditorModule::TickBridge));

	UE_LOG(LogRevoltEditorBridge, Log, TEXT("Revolt bridge started on %s."), *LocalEndpoint);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::StopBridge()
{
	if (!bBridgeRunning && !ListenSocket)
	{
		BridgeStatus = TEXT("Stopped");
		SetLastResponseSummary(TEXT("Error BRIDGE_NOT_RUNNING"));
		return FReply::Handled();
	}

	if (BridgeTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(BridgeTickerHandle);
		BridgeTickerHandle.Reset();
	}

	CloseAllClients();

	if (ListenSocket)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;
	}

	bBridgeRunning = false;
	BridgeStatus = TEXT("Stopped");
	SetLastResponseSummary(TEXT("Bridge stopped"));
	UE_LOG(LogRevoltEditorBridge, Log, TEXT("Revolt bridge stopped."));

	return FReply::Handled();
}

bool FRevoltEditorBridgeEditorModule::TickBridge(float DeltaTime)
{
	if (!bBridgeRunning || !ListenSocket)
	{
		return false;
	}

	bool bHasPendingConnection = false;
	while (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
	{
		FSocket* ClientSocket = ListenSocket->Accept(TEXT("RevoltEditorBridgeClient"));
		if (ClientSocket)
		{
			ClientSocket->SetNonBlocking(true);
			FRevoltBridgeClient& Client = Clients.AddDefaulted_GetRef();
			Client.Socket = ClientSocket;
			Client.AcceptedTimeSeconds = FPlatformTime::Seconds();
		}
	}

	for (int32 ClientIndex = Clients.Num() - 1; ClientIndex >= 0; --ClientIndex)
	{
		bool bShouldClose = false;
		PollClient(Clients[ClientIndex], bShouldClose);

		const double ClientAgeSeconds = FPlatformTime::Seconds() - Clients[ClientIndex].AcceptedTimeSeconds;
		if (bShouldClose || ClientAgeSeconds > 5.0)
		{
			CloseClient(Clients[ClientIndex]);
			Clients.RemoveAtSwap(ClientIndex);
		}
	}

	return true;
}

void FRevoltEditorBridgeEditorModule::CloseClient(FRevoltBridgeClient& Client)
{
	if (Client.Socket)
	{
		Client.Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Client.Socket);
		Client.Socket = nullptr;
	}
}

void FRevoltEditorBridgeEditorModule::CloseAllClients()
{
	for (FRevoltBridgeClient& Client : Clients)
	{
		CloseClient(Client);
	}

	Clients.Empty();
}

void FRevoltEditorBridgeEditorModule::PollClient(FRevoltBridgeClient& Client, bool& bShouldClose)
{
	if (!Client.Socket)
	{
		bShouldClose = true;
		return;
	}

	uint32 PendingDataSize = 0;
	while (Client.Socket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
	{
		const int32 ReadSize = FMath::Min<int32>(PendingDataSize, 64 * 1024);
		const int32 PreviousSize = Client.Buffer.Num();
		Client.Buffer.AddUninitialized(ReadSize);

		int32 BytesRead = 0;
		if (!Client.Socket->Recv(Client.Buffer.GetData() + PreviousSize, ReadSize, BytesRead) || BytesRead <= 0)
		{
			Client.Buffer.SetNum(PreviousSize);
			break;
		}

		Client.Buffer.SetNum(PreviousSize + BytesRead);
	}

	TryHandleHttpRequest(Client, bShouldClose);
}

bool FRevoltEditorBridgeEditorModule::TryHandleHttpRequest(FRevoltBridgeClient& Client, bool& bShouldClose)
{
	int32 HeaderEndIndex = INDEX_NONE;
	for (int32 BufferIndex = 0; BufferIndex <= Client.Buffer.Num() - 4; ++BufferIndex)
	{
		if (Client.Buffer[BufferIndex] == '\r' &&
			Client.Buffer[BufferIndex + 1] == '\n' &&
			Client.Buffer[BufferIndex + 2] == '\r' &&
			Client.Buffer[BufferIndex + 3] == '\n')
		{
			HeaderEndIndex = BufferIndex;
			break;
		}
	}

	if (HeaderEndIndex == INDEX_NONE)
	{
		return false;
	}

	FUTF8ToTCHAR HeaderChars(reinterpret_cast<const ANSICHAR*>(Client.Buffer.GetData()), HeaderEndIndex);
	const FString HeaderString(HeaderChars.Length(), HeaderChars.Get());
	int32 ContentLength = 0;
	TArray<FString> HeaderLines;
	HeaderString.ParseIntoArrayLines(HeaderLines);
	for (const FString& HeaderLine : HeaderLines)
	{
		FString HeaderName;
		FString HeaderValue;
		if (HeaderLine.Split(TEXT(":"), &HeaderName, &HeaderValue) && HeaderName.Equals(TEXT("Content-Length"), ESearchCase::IgnoreCase))
		{
			ContentLength = FCString::Atoi(*HeaderValue.TrimStartAndEnd());
			break;
		}
	}

	const int32 BodyStartIndex = HeaderEndIndex + 4;
	if (Client.Buffer.Num() < BodyStartIndex + ContentLength)
	{
		return false;
	}

	FString BodyJson;
	if (ContentLength > 0)
	{
		FUTF8ToTCHAR BodyChars(reinterpret_cast<const ANSICHAR*>(Client.Buffer.GetData() + BodyStartIndex), ContentLength);
		BodyJson = FString(BodyChars.Length(), BodyChars.Get());
	}

	const FString JsonResponse = HandleCommandJson(BodyJson);
	FTCHARToUTF8 ResponseBodyUtf8(*JsonResponse);
	const FString ResponseHeader = FString::Printf(
		TEXT("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"),
		ResponseBodyUtf8.Length());
	FTCHARToUTF8 ResponseHeaderUtf8(*ResponseHeader);

	int32 BytesSent = 0;
	Client.Socket->Send(reinterpret_cast<const uint8*>(ResponseHeaderUtf8.Get()), ResponseHeaderUtf8.Length(), BytesSent);
	Client.Socket->Send(reinterpret_cast<const uint8*>(ResponseBodyUtf8.Get()), ResponseBodyUtf8.Length(), BytesSent);

	bShouldClose = true;
	return true;
}

FString FRevoltEditorBridgeEditorModule::HandleCommandJson(const FString& RequestJson)
{
	TSharedPtr<FJsonObject> RequestObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RequestJson);
	if (!FJsonSerializer::Deserialize(Reader, RequestObject) || !RequestObject.IsValid())
	{
		SetLastCommand(TEXT("invalid_json"));
		SetLastResponseSummary(TEXT("Error INVALID_JSON"));
		return MakeErrorResponse(TEXT(""), TEXT("INVALID_JSON"), TEXT("Request body must be valid JSON."));
	}

	FString RequestId;
	if (!RequestObject->TryGetStringField(TEXT("id"), RequestId))
	{
		RequestId = TEXT("");
	}

	FString Command;
	if (!RequestObject->TryGetStringField(TEXT("command"), Command) || Command.IsEmpty())
	{
		SetLastCommand(TEXT("missing_command"));
		SetLastResponseSummary(TEXT("Error INVALID_PARAMETERS"));
		return MakeErrorResponse(RequestId, TEXT("INVALID_PARAMETERS"), TEXT("Request must include a non-empty command string."));
	}

	const TSharedPtr<FJsonObject>* ParamsObject = nullptr;
	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	if (RequestObject->TryGetObjectField(TEXT("params"), ParamsObject))
	{
		Params = *ParamsObject;
	}
	else if (RequestObject->HasField(TEXT("params")))
	{
		SetLastCommand(Command);
		SetLastResponseSummary(TEXT("Error INVALID_PARAMETERS"));
		return MakeErrorResponse(RequestId, TEXT("INVALID_PARAMETERS"), TEXT("params must be a JSON object."));
	}

	SetLastCommand(Command);

	FString ErrorCode;
	FString ErrorMessage;
	TSharedPtr<FJsonObject> Result = RouteCommand(RequestId, Command, Params, ErrorCode, ErrorMessage);
	if (!Result.IsValid())
	{
		SetLastResponseSummary(FString::Printf(TEXT("Error %s"), *ErrorCode));
		return MakeErrorResponse(RequestId, ErrorCode, ErrorMessage);
	}

	SetLastResponseSummary(FString::Printf(TEXT("%s returned ok: true"), *Command));
	return MakeSuccessResponse(RequestId, Result);
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::RouteCommand(const FString& RequestId, const FString& Command, const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage)
{
	if (Command == TEXT("ping"))
	{
		return HandlePing();
	}

	if (Command == TEXT("get_project_summary"))
	{
		return HandleGetProjectSummary(ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("get_open_level_summary"))
	{
		return HandleGetOpenLevelSummary(ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("get_selected_actors"))
	{
		return HandleGetSelectedActors(ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("find_assets"))
	{
		return HandleFindAssets(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("run_project_audit") || Command == TEXT("run_full_project_audit"))
	{
		return HandleRunProjectAudit(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("audit_current_level"))
	{
		return HandleAuditCurrentLevel(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("audit_selected_actors"))
	{
		return HandleAuditSelectedActors(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("audit_blueprints"))
	{
		return HandleAuditBlueprints(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("audit_assets"))
	{
		return HandleAuditAssets(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("audit_generated_content"))
	{
		return HandleAuditGeneratedContent(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("generate_fix_plan"))
	{
		return HandleGenerateFixPlan(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("export_audit_report"))
	{
		return HandleExportAuditReport(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("get_blueprint_compile_status"))
	{
		return HandleGetBlueprintCompileStatus(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("read_biome_asset"))
	{
		return HandleReadBiomeAsset(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("validate_biome_asset"))
	{
		return HandleValidateBiomeAsset(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("export_biome_asset_json"))
	{
		return HandleExportBiomeAssetJson(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_validate"))
	{
		return HandleLandGenValidate(Params, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_get_generated_summary"))
	{
		return HandleLandGenGetGeneratedSummary(Params, ErrorCode, ErrorMessage);
	}

	if (IsMutationCommand(Command))
	{
		return HandleMutationCommand(RequestId, Command, Params, ErrorCode, ErrorMessage);
	}

	ErrorCode = TEXT("UNKNOWN_COMMAND");
	ErrorMessage = FString::Printf(TEXT("Unknown command '%s'."), *Command);
	return nullptr;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandlePing() const
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("message"), TEXT("pong"));
	Result->SetStringField(TEXT("offline_mode"), TEXT("enabled"));
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleGetProjectSummary(FString& ErrorCode, FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("project_name"), FApp::GetProjectName());
	Result->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
	Result->SetNumberField(TEXT("enabled_plugin_count"), IPluginManager::Get().GetEnabledPlugins().Num());

	TArray<FString> ContentPaths;
	FPackageName::QueryRootContentPaths(ContentPaths);
	TArray<TSharedPtr<FJsonValue>> ContentPathValues;
	for (const FString& ContentPath : ContentPaths)
	{
		ContentPathValues.Add(MakeShared<FJsonValueString>(ContentPath));
	}
	Result->SetArrayField(TEXT("content_paths"), ContentPathValues);

	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleGetOpenLevelSummary(FString& ErrorCode, FString& ErrorMessage) const
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	int32 ActorCount = 0;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		++ActorCount;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("current_map_name"), World->GetMapName());
	Result->SetNumberField(TEXT("actor_count"), ActorCount);
	FString WorldTypeString = TEXT("Unknown");
	switch (World->WorldType)
	{
	case EWorldType::Game:
		WorldTypeString = TEXT("Game");
		break;
	case EWorldType::Editor:
		WorldTypeString = TEXT("Editor");
		break;
	case EWorldType::PIE:
		WorldTypeString = TEXT("PIE");
		break;
	case EWorldType::EditorPreview:
		WorldTypeString = TEXT("EditorPreview");
		break;
	case EWorldType::GamePreview:
		WorldTypeString = TEXT("GamePreview");
		break;
	case EWorldType::GameRPC:
		WorldTypeString = TEXT("GameRPC");
		break;
	case EWorldType::Inactive:
		WorldTypeString = TEXT("Inactive");
		break;
	default:
		break;
	}
	Result->SetStringField(TEXT("world_type"), WorldTypeString);
	Result->SetStringField(TEXT("persistent_level"), World->PersistentLevel ? World->PersistentLevel->GetPathName() : TEXT(""));
	Result->SetNumberField(TEXT("loaded_level_count"), World->GetNumLevels());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleGetSelectedActors(FString& ErrorCode, FString& ErrorMessage) const
{
	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		ErrorCode = TEXT("NO_SELECTED_ACTORS");
		ErrorMessage = TEXT("No actors are currently selected.");
		return nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> ActorValues;
	for (FSelectionIterator SelectionIterator(*SelectedActors); SelectionIterator; ++SelectionIterator)
	{
		AActor* Actor = Cast<AActor>(*SelectionIterator);
		if (!Actor)
		{
			continue;
		}

		TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
		ActorObject->SetStringField(TEXT("actor_name"), Actor->GetActorNameOrLabel());
		ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		ActorObject->SetStringField(TEXT("path"), Actor->GetPathName());

		const FVector Location = Actor->GetActorLocation();
		const FRotator Rotation = Actor->GetActorRotation();
		const FVector Scale = Actor->GetActorScale3D();
		TSharedPtr<FJsonObject> LocationObject = MakeShared<FJsonObject>();
		LocationObject->SetNumberField(TEXT("x"), Location.X);
		LocationObject->SetNumberField(TEXT("y"), Location.Y);
		LocationObject->SetNumberField(TEXT("z"), Location.Z);
		ActorObject->SetObjectField(TEXT("location"), LocationObject);

		TSharedPtr<FJsonObject> RotationObject = MakeShared<FJsonObject>();
		RotationObject->SetNumberField(TEXT("pitch"), Rotation.Pitch);
		RotationObject->SetNumberField(TEXT("yaw"), Rotation.Yaw);
		RotationObject->SetNumberField(TEXT("roll"), Rotation.Roll);
		ActorObject->SetObjectField(TEXT("rotation"), RotationObject);

		TSharedPtr<FJsonObject> ScaleObject = MakeShared<FJsonObject>();
		ScaleObject->SetNumberField(TEXT("x"), Scale.X);
		ScaleObject->SetNumberField(TEXT("y"), Scale.Y);
		ScaleObject->SetNumberField(TEXT("z"), Scale.Z);
		ActorObject->SetObjectField(TEXT("scale"), ScaleObject);

		TArray<TSharedPtr<FJsonValue>> TagValues;
		for (const FName& Tag : Actor->Tags)
		{
			TagValues.Add(MakeShared<FJsonValueString>(Tag.ToString()));
		}
		ActorObject->SetArrayField(TEXT("tags"), TagValues);
		ActorObject->SetNumberField(TEXT("component_count"), Actor->GetComponents().Num());

		ActorValues.Add(MakeShared<FJsonValueObject>(ActorObject));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("actors"), ActorValues);
	Result->SetNumberField(TEXT("count"), ActorValues.Num());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleFindAssets(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("find_assets params must be a JSON object.");
		return nullptr;
	}

	FString PathFilter = TEXT("/Game");
	FString ClassFilter;
	FString NameSubstring;
	double MaxResultCountNumber = 50.0;
	Params->TryGetStringField(TEXT("path"), PathFilter);
	Params->TryGetStringField(TEXT("path_filter"), PathFilter);
	Params->TryGetStringField(TEXT("class"), ClassFilter);
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);
	Params->TryGetStringField(TEXT("type"), ClassFilter);
	Params->TryGetStringField(TEXT("type_filter"), ClassFilter);
	Params->TryGetStringField(TEXT("name_substring"), NameSubstring);
	Params->TryGetStringField(TEXT("name_filter"), NameSubstring);
	Params->TryGetNumberField(TEXT("max_results"), MaxResultCountNumber);
	Params->TryGetNumberField(TEXT("max_result_count"), MaxResultCountNumber);

	const int32 MaxResultCount = FMath::Clamp(FMath::FloorToInt(MaxResultCountNumber), 1, 500);
	if (!PathFilter.StartsWith(TEXT("/")))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("find_assets path must be a package path such as /Game.");
		return nullptr;
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(*PathFilter);
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TArray<TSharedPtr<FJsonValue>> AssetValues;
	for (const FAssetData& Asset : AssetData)
	{
		if (!NameSubstring.IsEmpty() && !Asset.AssetName.ToString().Contains(NameSubstring, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (!ClassFilter.IsEmpty() &&
			!Asset.AssetClassPath.ToString().Contains(ClassFilter, ESearchCase::IgnoreCase) &&
			!Asset.AssetClassPath.GetAssetName().ToString().Contains(ClassFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		TSharedPtr<FJsonObject> AssetObject = MakeShared<FJsonObject>();
		AssetObject->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		AssetObject->SetStringField(TEXT("class"), Asset.AssetClassPath.ToString());
		AssetObject->SetStringField(TEXT("package_name"), Asset.PackageName.ToString());
		AssetObject->SetStringField(TEXT("object_path"), Asset.GetObjectPathString());
		AssetObject->SetStringField(TEXT("package_path"), Asset.PackagePath.ToString());
		AssetValues.Add(MakeShared<FJsonValueObject>(AssetObject));

		if (AssetValues.Num() >= MaxResultCount)
		{
			break;
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("assets"), AssetValues);
	Result->SetNumberField(TEXT("count"), AssetValues.Num());
	Result->SetNumberField(TEXT("max_results"), MaxResultCount);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleRunProjectAudit(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> LevelAudit = HandleAuditCurrentLevel(Params, ErrorCode, ErrorMessage);
	if (!LevelAudit.IsValid())
	{
		return nullptr;
	}

	FString BlueprintErrorCode;
	FString BlueprintErrorMessage;
	TSharedPtr<FJsonObject> BlueprintAudit = HandleAuditBlueprints(Params, BlueprintErrorCode, BlueprintErrorMessage);
	FString AssetErrorCode;
	FString AssetErrorMessage;
	TSharedPtr<FJsonObject> AssetAudit = HandleAuditAssets(Params, AssetErrorCode, AssetErrorMessage);
	FString GeneratedErrorCode;
	FString GeneratedErrorMessage;
	TSharedPtr<FJsonObject> GeneratedAudit = HandleAuditGeneratedContent(Params, GeneratedErrorCode, GeneratedErrorMessage);

	TArray<TSharedPtr<FJsonValue>> Issues;
	auto AppendIssues = [&Issues](const TSharedPtr<FJsonObject>& AuditSection)
	{
		const TArray<TSharedPtr<FJsonValue>>* SectionIssues = nullptr;
		if (AuditSection.IsValid() && AuditSection->TryGetArrayField(TEXT("issues"), SectionIssues))
		{
			Issues.Append(*SectionIssues);
		}
	};

	AppendIssues(LevelAudit);
	AppendIssues(BlueprintAudit);
	AppendIssues(AssetAudit);
	AppendIssues(GeneratedAudit);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("full_project"));
	Result->SetObjectField(TEXT("current_level"), LevelAudit);
	if (BlueprintAudit.IsValid())
	{
		Result->SetObjectField(TEXT("blueprints"), BlueprintAudit);
	}
	if (AssetAudit.IsValid())
	{
		Result->SetObjectField(TEXT("assets"), AssetAudit);
	}
	if (GeneratedAudit.IsValid())
	{
		Result->SetObjectField(TEXT("generated_content"), GeneratedAudit);
	}
	if (!BlueprintAudit.IsValid())
	{
		Result->SetStringField(TEXT("blueprint_audit_warning"), BlueprintErrorMessage);
	}
	if (!AssetAudit.IsValid())
	{
		Result->SetStringField(TEXT("asset_audit_warning"), AssetErrorMessage);
	}
	if (!GeneratedAudit.IsValid())
	{
		Result->SetStringField(TEXT("generated_content_audit_warning"), GeneratedErrorMessage);
	}
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAuditCurrentLevel(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	TArray<TSharedPtr<FJsonValue>> Issues;
	int32 IssueCounter = 1;
	int32 ActorCount = 0;
	bool bHasPlayerStart = false;
	bool bHasNavMeshBounds = false;
	bool bHasLikelyAiActor = false;
	bool bHasBasicLighting = false;
	bool bHasCameraSetup = false;
	bool bHasObjective = false;
	bool bHasPlayablePawnOrCharacter = false;

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor || Actor->IsPendingKillPending())
		{
			continue;
		}

		++ActorCount;
		bHasPlayerStart = bHasPlayerStart || Actor->IsA<APlayerStart>();
		bHasNavMeshBounds = bHasNavMeshBounds || Actor->GetClass()->GetName().Contains(TEXT("NavMeshBoundsVolume"));
		bHasCameraSetup = bHasCameraSetup ||
			Actor->GetClass()->GetName().Contains(TEXT("Camera"), ESearchCase::IgnoreCase) ||
			Actor->GetName().Contains(TEXT("Camera"), ESearchCase::IgnoreCase);
		bHasObjective = bHasObjective ||
			Actor->GetClass()->GetName().Contains(TEXT("Objective"), ESearchCase::IgnoreCase) ||
			Actor->GetName().Contains(TEXT("Objective"), ESearchCase::IgnoreCase) ||
			Actor->Tags.Contains(FName(TEXT("Revolt.Objective")));
		bHasLikelyAiActor = bHasLikelyAiActor ||
			(Actor->IsA<ACharacter>() && Actor->GetName().Contains(TEXT("AI"), ESearchCase::IgnoreCase)) ||
			(Actor->IsA<APawn>() && Actor->GetClass()->GetName().Contains(TEXT("AI"), ESearchCase::IgnoreCase));
		bHasPlayablePawnOrCharacter = bHasPlayablePawnOrCharacter ||
			((Actor->IsA<ACharacter>() || Actor->IsA<APawn>()) && !Actor->GetName().Contains(TEXT("AI"), ESearchCase::IgnoreCase) && !Actor->GetClass()->GetName().Contains(TEXT("AI"), ESearchCase::IgnoreCase));

		TArray<ULightComponent*> LightComponents;
		Actor->GetComponents<ULightComponent>(LightComponents);
		bHasBasicLighting = bHasBasicLighting || LightComponents.Num() > 0;

		AddActorAuditIssues(Actor, Issues, TEXT("current_level"), IssueCounter);
	}

	const bool bHasGameMode = World->GetWorldSettings() && World->GetWorldSettings()->DefaultGameMode != nullptr;

	if (!bHasPlayerStart)
	{
		AddAuditIssue(Issues, TEXT("current_level"), TEXT("warning"), TEXT("map_without_player_start"), TEXT("level"), World->GetMapName(), TEXT("Current map has no PlayerStart actor."), TEXT("Add a PlayerStart if this map is intended for playtesting or gameplay."), IssueCounter);
	}
	if (bHasLikelyAiActor && !bHasNavMeshBounds)
	{
		AddAuditIssue(Issues, TEXT("current_level"), TEXT("warning"), TEXT("missing_navmesh_in_ai_map"), TEXT("level"), World->GetMapName(), TEXT("Likely AI actors were found but no NavMeshBoundsVolume was detected."), TEXT("Add a NavMeshBoundsVolume if AI navigation is required."), IssueCounter);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("current_level"));
	Result->SetStringField(TEXT("map_name"), World->GetMapName());
	Result->SetNumberField(TEXT("actor_count"), ActorCount);
	Result->SetBoolField(TEXT("has_player_start"), bHasPlayerStart);
	Result->SetBoolField(TEXT("has_navmesh_bounds_volume"), bHasNavMeshBounds);
	Result->SetBoolField(TEXT("has_likely_ai_actor"), bHasLikelyAiActor);
	Result->SetBoolField(TEXT("has_basic_lighting"), bHasBasicLighting);
	Result->SetBoolField(TEXT("has_camera_setup"), bHasCameraSetup);
	Result->SetBoolField(TEXT("has_objective"), bHasObjective);
	Result->SetBoolField(TEXT("has_playable_pawn_or_character"), bHasPlayablePawnOrCharacter);
	Result->SetBoolField(TEXT("has_game_mode"), bHasGameMode);
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAuditSelectedActors(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;

	TArray<TSharedPtr<FJsonValue>> Issues;
	TArray<TSharedPtr<FJsonValue>> ActorValues;
	int32 IssueCounter = 1;

	if (SelectedActors)
	{
		for (FSelectionIterator SelectionIterator(*SelectedActors); SelectionIterator; ++SelectionIterator)
		{
			AActor* Actor = Cast<AActor>(*SelectionIterator);
			if (!Actor)
			{
				continue;
			}

			TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
			ActorObject->SetStringField(TEXT("name"), Actor->GetActorNameOrLabel());
			ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
			ActorObject->SetStringField(TEXT("path"), Actor->GetPathName());
			ActorValues.Add(MakeShared<FJsonValueObject>(ActorObject));
			AddActorAuditIssues(Actor, Issues, TEXT("selected_actors"), IssueCounter);
		}
	}

	if (ActorValues.IsEmpty())
	{
		AddAuditIssue(Issues, TEXT("selected_actors"), TEXT("info"), TEXT("no_selected_actors"), TEXT("selection"), TEXT("Editor selection"), TEXT("No actors are selected for selected-actor audit."), TEXT("Select one or more actors, then run the selected actor audit again."), IssueCounter);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("selected_actors"));
	Result->SetArrayField(TEXT("actors"), ActorValues);
	Result->SetNumberField(TEXT("selected_count"), ActorValues.Num());
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAuditBlueprints(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	double MaxBlueprintsNumber = 200.0;
	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("max_results"), MaxBlueprintsNumber);
		Params->TryGetNumberField(TEXT("max_blueprints"), MaxBlueprintsNumber);
	}
	const int32 MaxBlueprints = FMath::Clamp(FMath::FloorToInt(MaxBlueprintsNumber), 1, 1000);

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TArray<TSharedPtr<FJsonValue>> Issues;
	TArray<TSharedPtr<FJsonValue>> BlueprintValues;
	int32 IssueCounter = 1;
	int32 CheckedCount = 0;

	for (const FAssetData& Asset : AssetData)
	{
		if (!Asset.AssetClassPath.ToString().Contains(TEXT("Blueprint"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (CheckedCount >= MaxBlueprints)
		{
			break;
		}

		++CheckedCount;
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		const FString Status = GetBlueprintCompileStatusString(Blueprint);

		TSharedPtr<FJsonObject> BlueprintObject = MakeShared<FJsonObject>();
		BlueprintObject->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		BlueprintObject->SetStringField(TEXT("object_path"), Asset.GetObjectPathString());
		BlueprintObject->SetStringField(TEXT("package_path"), Asset.PackagePath.ToString());
		BlueprintObject->SetStringField(TEXT("compile_status"), Status);
		BlueprintValues.Add(MakeShared<FJsonValueObject>(BlueprintObject));

		if (!Blueprint)
		{
			AddAuditIssue(Issues, TEXT("blueprints"), TEXT("warning"), TEXT("missing_reference_or_load_failed"), TEXT("asset"), Asset.GetObjectPathString(), TEXT("Blueprint asset could not be loaded for read-only inspection."), TEXT("Open the Blueprint in Unreal and resolve missing references or load errors."), IssueCounter);
		}
		else if (Status == TEXT("error"))
		{
			AddAuditIssue(Issues, TEXT("blueprints"), TEXT("error"), TEXT("blueprint_compile_error"), TEXT("asset"), Asset.GetObjectPathString(), TEXT("Blueprint reports a compile error."), TEXT("Open the Blueprint compiler results and fix the reported graph or reference errors."), IssueCounter);
		}
		else if (Status == TEXT("dirty") || Status == TEXT("unknown"))
		{
			AddAuditIssue(Issues, TEXT("blueprints"), TEXT("info"), TEXT("blueprint_compile_status_uncertain"), TEXT("asset"), Asset.GetObjectPathString(), TEXT("Blueprint compile status is dirty or unknown."), TEXT("Compile the Blueprint to confirm whether issues exist."), IssueCounter);
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("blueprints"));
	Result->SetArrayField(TEXT("blueprints"), BlueprintValues);
	Result->SetNumberField(TEXT("checked_count"), CheckedCount);
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAuditAssets(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	double MaxAssetsNumber = 500.0;
	if (Params.IsValid())
	{
		Params->TryGetNumberField(TEXT("max_results"), MaxAssetsNumber);
		Params->TryGetNumberField(TEXT("max_assets"), MaxAssetsNumber);
	}
	const int32 MaxAssets = FMath::Clamp(FMath::FloorToInt(MaxAssetsNumber), 1, 2000);

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TArray<TSharedPtr<FJsonValue>> Issues;
	int32 IssueCounter = 1;
	int32 CheckedCount = 0;

	for (const FAssetData& Asset : AssetData)
	{
		if (CheckedCount >= MaxAssets)
		{
			break;
		}
		++CheckedCount;

		const FString ObjectPath = Asset.GetObjectPathString();
		const FString PackagePath = Asset.PackagePath.ToString();
		const FString AssetName = Asset.AssetName.ToString();
		const FString ClassName = Asset.AssetClassPath.GetAssetName().ToString();

		if (!PackagePath.StartsWith(TEXT("/Game/RevoltGenerated")) &&
			(AssetName.StartsWith(TEXT("Revolt")) || ObjectPath.Contains(TEXT("RevoltGenerated"))))
		{
			AddAuditIssue(Issues, TEXT("assets"), TEXT("warning"), TEXT("generated_asset_outside_generated_root"), TEXT("asset"), ObjectPath, TEXT("Asset appears to be generated content outside /Game/RevoltGenerated."), TEXT("Move future generated assets under /Game/RevoltGenerated; review this asset manually before changing it."), IssueCounter);
		}

		if (ClassName.Contains(TEXT("MaterialInstance"), ESearchCase::IgnoreCase))
		{
			UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Asset.GetAsset());
			if (MaterialInstance && !MaterialInstance->Parent)
			{
				AddAuditIssue(Issues, TEXT("assets"), TEXT("warning"), TEXT("material_missing_parent"), TEXT("asset"), ObjectPath, TEXT("Material instance has no parent material assigned."), TEXT("Assign a valid parent material or recreate the generated material instance."), IssueCounter);
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("assets"));
	Result->SetNumberField(TEXT("checked_count"), CheckedCount);
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAuditGeneratedContent(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	TArray<TSharedPtr<FJsonValue>> Issues;
	TArray<TSharedPtr<FJsonValue>> GeneratedAssetValues;
	int32 IssueCounter = 1;

	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/RevoltGenerated"));
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	for (const FAssetData& Asset : AssetData)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShared<FJsonObject>();
		AssetObject->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		AssetObject->SetStringField(TEXT("class"), Asset.AssetClassPath.ToString());
		AssetObject->SetStringField(TEXT("object_path"), Asset.GetObjectPathString());
		AssetObject->SetStringField(TEXT("package_path"), Asset.PackagePath.ToString());
		GeneratedAssetValues.Add(MakeShared<FJsonValueObject>(AssetObject));
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	int32 GeneratedActorCount = 0;
	if (World)
	{
		for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
		{
			AActor* Actor = *ActorIterator;
			if (!Actor)
			{
				continue;
			}

			bool bHasBiomeTag = false;
			for (const FName& Tag : Actor->Tags)
			{
				if (Tag.ToString().StartsWith(TEXT("Revolt.Biome.")))
				{
					bHasBiomeTag = true;
					break;
				}
			}
			if (Actor->ActorHasTag(TEXT("Revolt.Generated")))
			{
				++GeneratedActorCount;
			}
			else if (bHasBiomeTag)
			{
				AddAuditIssue(Issues, TEXT("generated_content"), TEXT("warning"), TEXT("generated_actor_missing_tag"), TEXT("actor"), Actor->GetPathName(), TEXT("Actor has a Revolt biome tag but is missing Revolt.Generated."), TEXT("Review whether this actor is generated content before adding tags or baking."), IssueCounter);
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("generated_content"));
	Result->SetNumberField(TEXT("generated_asset_count"), GeneratedAssetValues.Num());
	Result->SetNumberField(TEXT("generated_actor_count"), GeneratedActorCount);
	Result->SetArrayField(TEXT("generated_assets"), GeneratedAssetValues);
	Result->SetArrayField(TEXT("issues"), Issues);
	Result->SetNumberField(TEXT("issue_count"), Issues.Num());
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleGenerateFixPlan(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	TArray<TSharedPtr<FJsonValue>> FixPlanValues;
	const TArray<TSharedPtr<FJsonValue>>* IssueValues = nullptr;
	if (Params.IsValid() && Params->TryGetArrayField(TEXT("issues"), IssueValues))
	{
		for (const TSharedPtr<FJsonValue>& IssueValue : *IssueValues)
		{
			TSharedPtr<FJsonObject> IssueObject = IssueValue.IsValid() ? IssueValue->AsObject() : nullptr;
			if (IssueObject.IsValid())
			{
				FixPlanValues.Add(MakeShared<FJsonValueObject>(MakeFixPlanItemFromIssue(IssueObject)));
			}
		}
	}
	else
	{
		TSharedPtr<FJsonObject> Audit = HandleRunProjectAudit(Params, ErrorCode, ErrorMessage);
		if (!Audit.IsValid())
		{
			return nullptr;
		}

		const TArray<TSharedPtr<FJsonValue>>* AuditIssues = nullptr;
		if (Audit->TryGetArrayField(TEXT("issues"), AuditIssues))
		{
			for (const TSharedPtr<FJsonValue>& IssueValue : *AuditIssues)
			{
				TSharedPtr<FJsonObject> IssueObject = IssueValue.IsValid() ? IssueValue->AsObject() : nullptr;
				if (IssueObject.IsValid())
				{
					FixPlanValues.Add(MakeShared<FJsonValueObject>(MakeFixPlanItemFromIssue(IssueObject)));
				}
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("plan_type"), TEXT("read_only_fix_plan"));
	Result->SetArrayField(TEXT("fix_plan"), FixPlanValues);
	Result->SetNumberField(TEXT("item_count"), FixPlanValues.Num());
	Result->SetBoolField(TEXT("executed"), false);
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleExportAuditReport(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Audit = HandleRunProjectAudit(Params, ErrorCode, ErrorMessage);
	if (!Audit.IsValid())
	{
		return nullptr;
	}

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Audit.ToSharedRef(), Writer);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("audit_type"), TEXT("export"));
	Result->SetObjectField(TEXT("audit"), Audit);
	Result->SetStringField(TEXT("json"), JsonString);
	Result->SetBoolField(TEXT("executed"), false);
	Result->SetBoolField(TEXT("read_only"), true);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleMutationCommand(const FString& RequestId, const FString& Command, const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage)
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Mutating commands require params to be a JSON object.");
		AddHistoryEntry(Command, TEXT("Unknown"), TEXT("rejected"), ErrorMessage);
		return nullptr;
	}

	if (!HasMutationPermission(Params))
	{
		ErrorCode = TEXT("INSUFFICIENT_PERMISSION");
		ErrorMessage = TEXT("Mutating commands require permission_level to be 'editor_mutation', 'asset_mutation', or 'blueprint_mutation'.");
		AddHistoryEntry(Command, TEXT("Unknown"), TEXT("rejected"), ErrorMessage);
		return nullptr;
	}

	FString Target;
	FString ResultSummary;
	const bool bDryRun = GetDryRunFlag(Params);
	const bool bApproved = GetApprovedFlag(Params);

	if (bDryRun)
	{
		TSharedPtr<FJsonObject> Result = ApplyMutationCommand(Command, Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
		if (Result.IsValid())
		{
			FString RiskLevel;
			const TArray<TSharedPtr<FJsonObject>> Diff = BuildDryRunDiff(RequestId, Command, Params, Result, Target, RiskLevel);
			if (Diff.IsEmpty())
			{
				ErrorCode = TEXT("DRY_RUN_DIFF_UNAVAILABLE");
				ErrorMessage = TEXT("Mutating commands must produce a reliable dry-run diff before approval.");
				AddHistoryEntry(Command, Target.IsEmpty() ? TEXT("Unknown") : Target, TEXT("rejected"), ErrorMessage);
				return nullptr;
			}

			AttachDryRunDiff(Result, Diff, RiskLevel, true);
			AddHistoryEntry(Command, Target, TEXT("dry-run"), FString::Printf(TEXT("%s | changes: %d | risk: %s"), *ResultSummary, Diff.Num(), *RiskLevel));
		}
		else
		{
			AddHistoryEntry(Command, Target.IsEmpty() ? TEXT("Unknown") : Target, TEXT("rejected"), ErrorMessage);
		}
		return Result;
	}

	TSharedPtr<FJsonObject> Preview = ApplyMutationCommand(Command, Params, true, Target, ResultSummary, ErrorCode, ErrorMessage);
	if (!Preview.IsValid())
	{
		AddHistoryEntry(Command, Target.IsEmpty() ? TEXT("Unknown") : Target, TEXT("rejected"), ErrorMessage);
		return nullptr;
	}

	FString RiskLevel;
	const TArray<TSharedPtr<FJsonObject>> Diff = BuildDryRunDiff(RequestId, Command, Params, Preview, Target, RiskLevel);
	if (Diff.IsEmpty())
	{
		ErrorCode = TEXT("DRY_RUN_DIFF_UNAVAILABLE");
		ErrorMessage = TEXT("Mutating commands must produce a reliable dry-run diff before approval.");
		AddHistoryEntry(Command, Target.IsEmpty() ? TEXT("Unknown") : Target, TEXT("rejected"), ErrorMessage);
		return nullptr;
	}
	AttachDryRunDiff(Preview, Diff, RiskLevel, true);

	if (bApproved)
	{
		TSharedPtr<FJsonObject> Result = ApplyMutationCommand(Command, Params, false, Target, ResultSummary, ErrorCode, ErrorMessage);
		if (!Result.IsValid())
		{
			AddHistoryEntry(Command, Target.IsEmpty() ? TEXT("Unknown") : Target, TEXT("rejected"), ErrorMessage);
			return nullptr;
		}
		AttachDryRunDiff(Result, Diff, RiskLevel, false);
		AddHistoryEntry(Command, Target, TEXT("applied"), FString::Printf(TEXT("Approved by request | changes: %d | risk: %s | %s"), Diff.Num(), *RiskLevel, *ResultSummary));
		return Result;
	}

	const int32 HistoryIndex = AddHistoryEntry(Command, Target, TEXT("pending"), FString::Printf(TEXT("%s | changes: %d | risk: %s"), *ResultSummary, Diff.Num(), *RiskLevel));

	FRevoltPendingMutation& PendingMutation = ApprovalQueue.AddDefaulted_GetRef();
	PendingMutation.RequestId = RequestId;
	PendingMutation.Command = Command;
	PendingMutation.Target = Target;
	PendingMutation.Summary = ResultSummary;
	PendingMutation.Params = Params;
	PendingMutation.Diff = Diff;
	PendingMutation.RiskLevel = RiskLevel;
	PendingMutation.Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
	PendingMutation.HistoryIndex = HistoryIndex;
	SelectedApprovalIndex = ApprovalQueue.Num() - 1;
	RefreshSelectedDiffIndex();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("status"), TEXT("pending_approval"));
	Result->SetStringField(TEXT("approval_id"), FString::FromInt(SelectedApprovalIndex));
	Result->SetStringField(TEXT("summary"), ResultSummary);
	AttachDryRunDiff(Result, Diff, RiskLevel, true);
	Result->SetObjectField(TEXT("preview"), Preview);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::ApplyMutationCommand(const FString& Command, const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	if (Command == TEXT("set_actor_transform"))
	{
		return HandleSetActorTransform(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("set_actor_property"))
	{
		return HandleSetActorProperty(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("spawn_actor"))
	{
		return HandleSpawnActor(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("duplicate_selected_actors"))
	{
		return HandleDuplicateSelectedActors(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("delete_generated_actors_only"))
	{
		return HandleDeleteGeneratedActorsOnly(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_folder"))
	{
		return HandleCreateFolder(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_data_asset"))
	{
		return HandleCreateDataAsset(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_material_instance"))
	{
		return HandleCreateMaterialInstance(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("set_material_instance_parameter"))
	{
		return HandleSetMaterialInstanceParameter(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("bulk_edit_data_assets"))
	{
		return HandleBulkEditDataAssets(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("save_asset"))
	{
		return HandleSaveAsset(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("save_generated_assets"))
	{
		return HandleSaveGeneratedAssets(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_blueprint_class"))
	{
		return HandleCreateBlueprintClass(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("add_blueprint_variable"))
	{
		return HandleAddBlueprintVariable(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("set_blueprint_default_value"))
	{
		return HandleSetBlueprintDefaultValue(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("add_component_to_blueprint"))
	{
		return HandleAddComponentToBlueprint(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("compile_blueprint"))
	{
		return HandleCompileBlueprint(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_biome_asset"))
	{
		return HandleCreateBiomeAsset(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("update_biome_asset"))
	{
		return HandleUpdateBiomeAsset(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_generate_preview"))
	{
		return HandleLandGenGeneratePreview(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_clear_preview"))
	{
		return HandleLandGenClearPreview(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_randomize_seed"))
	{
		return HandleLandGenRandomizeSeed(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_spawn_biome_content"))
	{
		return HandleLandGenSpawnBiomeContent(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_clear_generated_content"))
	{
		return HandleLandGenClearGeneratedContent(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("landgen_bake_generated_content"))
	{
		return HandleLandGenBakeGeneratedContent(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_arena_shooter_template"))
	{
		return HandleCreateArenaShooterTemplate(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("configure_weapon_data"))
	{
		return HandleConfigureWeaponData(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("configure_enemy_data"))
	{
		return HandleConfigureEnemyData(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("spawn_test_arena"))
	{
		return HandleSpawnTestArena(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("create_zombie_shooter_template"))
	{
		return HandleCreateZombieShooterTemplate(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("configure_zombie_data"))
	{
		return HandleConfigureZombieData(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("configure_weapon_recoil"))
	{
		return HandleConfigureWeaponRecoil(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("spawn_zombie_test_arena"))
	{
		return HandleSpawnZombieTestArena(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	if (Command == TEXT("configure_wave_data"))
	{
		return HandleConfigureWaveData(Params, bDryRun, Target, ResultSummary, ErrorCode, ErrorMessage);
	}

	ErrorCode = TEXT("UNKNOWN_COMMAND");
	ErrorMessage = FString::Printf(TEXT("Unknown mutating command '%s'."), *Command);
	return nullptr;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	AActor* Actor = FindActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!Actor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(Actor);
	FVector NewLocation = Actor->GetActorLocation();
	FRotator NewRotation = Actor->GetActorRotation();
	FVector NewScale = Actor->GetActorScale3D();
	bool bHasLocation = false;
	bool bHasRotation = false;
	bool bHasScale = false;

	if (!ReadVectorObject(Params, TEXT("location"), NewLocation, bHasLocation, ErrorCode, ErrorMessage) ||
		!ReadRotatorObject(Params, TEXT("rotation"), NewRotation, bHasRotation, ErrorCode, ErrorMessage) ||
		!ReadVectorObject(Params, TEXT("scale"), NewScale, bHasScale, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	if (!bHasLocation && !bHasRotation && !bHasScale)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_actor_transform requires location, rotation, or scale.");
		return nullptr;
	}

	if (!AreTransformValuesFinite(NewLocation, NewRotation, NewScale))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Transform values must be finite.");
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("actor"), Target);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetObjectField(TEXT("before"), MakeTransformObject(Actor->GetActorLocation(), Actor->GetActorRotation(), Actor->GetActorScale3D()));
	Result->SetObjectField(TEXT("after"), MakeTransformObject(NewLocation, NewRotation, NewScale));

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SetActorTransformTransaction", "Revolt Set Actor Transform"));
		Actor->Modify();
		Actor->SetActorLocationAndRotation(NewLocation, NewRotation);
		Actor->SetActorScale3D(NewScale);
	}

	ResultSummary = bDryRun ? TEXT("Transform preview generated") : TEXT("Actor transform applied");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	AActor* Actor = FindActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!Actor)
	{
		return nullptr;
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property"), PropertyName) || PropertyName.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_actor_property requires a property name.");
		return nullptr;
	}

	if (!Params->HasField(TEXT("value")))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_actor_property requires a value field.");
		return nullptr;
	}

	FProperty* Property = Actor->GetClass()->FindPropertyByName(*PropertyName);
	if (!Property)
	{
		ErrorCode = TEXT("PROPERTY_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Property '%s' was not found on actor class '%s'."), *PropertyName, *Actor->GetClass()->GetName());
		return nullptr;
	}

	if (!Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
	{
		ErrorCode = TEXT("PROPERTY_NOT_EDITABLE");
		ErrorMessage = FString::Printf(TEXT("Property '%s' is not editable on this actor instance."), *PropertyName);
		return nullptr;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);
	FString BeforeValue;
	Property->ExportTextItem_Direct(BeforeValue, ValuePtr, ValuePtr, Actor, PPF_None);

	FString NewValueString;
	const TSharedPtr<FJsonValue> Value = Params->TryGetField(TEXT("value"));
	if (!Value.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_actor_property value could not be read.");
		return nullptr;
	}

	if (Value->Type == EJson::String)
	{
		NewValueString = Value->AsString();
	}
	else if (Value->Type == EJson::Number)
	{
		NewValueString = FString::SanitizeFloat(Value->AsNumber());
	}
	else if (Value->Type == EJson::Boolean)
	{
		NewValueString = Value->AsBool() ? TEXT("true") : TEXT("false");
	}
	else
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_actor_property supports string, number, and boolean values in Phase 3.");
		return nullptr;
	}

	Target = FString::Printf(TEXT("%s.%s"), *GetActorTargetName(Actor), *PropertyName);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("target"), Target);
	Result->SetStringField(TEXT("before"), BeforeValue);
	Result->SetStringField(TEXT("after"), NewValueString);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SetActorPropertyTransaction", "Revolt Set Actor Property"));
		Actor->Modify();
		if (!Property->ImportText_Direct(*NewValueString, ValuePtr, Actor, PPF_None))
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = FString::Printf(TEXT("Value could not be imported for property '%s'."), *PropertyName);
			return nullptr;
		}
		Actor->PostEditChange();
	}

	ResultSummary = bDryRun ? TEXT("Property change preview generated") : TEXT("Actor property applied");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class"), ClassName) && !Params->TryGetStringField(TEXT("class_path"), ClassName))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("spawn_actor requires class or class_path.");
		return nullptr;
	}

	UClass* SpawnClass = ResolveSpawnClass(ClassName, ErrorCode, ErrorMessage);
	if (!SpawnClass)
	{
		return nullptr;
	}

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Scale = FVector::OneVector;
	bool bHasLocation = false;
	bool bHasRotation = false;
	bool bHasScale = false;
	if (!ReadVectorObject(Params, TEXT("location"), Location, bHasLocation, ErrorCode, ErrorMessage) ||
		!ReadRotatorObject(Params, TEXT("rotation"), Rotation, bHasRotation, ErrorCode, ErrorMessage) ||
		!ReadVectorObject(Params, TEXT("scale"), Scale, bHasScale, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	if (!AreTransformValuesFinite(Location, Rotation, Scale))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Spawn transform values must be finite.");
		return nullptr;
	}

	Target = SpawnClass->GetName();
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("class"), SpawnClass->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetObjectField(TEXT("transform"), MakeTransformObject(Location, Rotation, Scale));

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SpawnActorTransaction", "Revolt Spawn Actor"));
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->GetCurrentLevel();
		SpawnParameters.ObjectFlags = RF_Transactional;
		AActor* NewActor = World->SpawnActor<AActor>(SpawnClass, Location, Rotation, SpawnParameters);
		if (!NewActor)
		{
			ErrorCode = TEXT("SPAWN_FAILED");
			ErrorMessage = TEXT("Unreal failed to spawn the requested actor.");
			return nullptr;
		}
		NewActor->Modify();
		NewActor->Tags.AddUnique(FName(TEXT("Revolt.Generated")));
		NewActor->SetActorScale3D(Scale);
		Result->SetStringField(TEXT("spawned_actor"), NewActor->GetPathName());
		Target = GetActorTargetName(NewActor);
	}

	ResultSummary = bDryRun ? TEXT("Spawn preview generated") : TEXT("Generated actor spawned and tagged Revolt.Generated");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleDuplicateSelectedActors(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		ErrorCode = TEXT("NO_SELECTED_ACTORS");
		ErrorMessage = TEXT("No actors are currently selected.");
		return nullptr;
	}

	TArray<AActor*> ActorsToDuplicate;
	for (FSelectionIterator SelectionIterator(*SelectedActors); SelectionIterator; ++SelectionIterator)
	{
		if (AActor* Actor = Cast<AActor>(*SelectionIterator))
		{
			ActorsToDuplicate.Add(Actor);
		}
	}

	if (ActorsToDuplicate.IsEmpty())
	{
		ErrorCode = TEXT("NO_SELECTED_ACTORS");
		ErrorMessage = TEXT("No valid actors are currently selected.");
		return nullptr;
	}

	Target = FString::Printf(TEXT("%d selected actor(s)"), ActorsToDuplicate.Num());
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("selected_count"), ActorsToDuplicate.Num());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (!EditorActorSubsystem)
		{
			ErrorCode = TEXT("EDITOR_SUBSYSTEM_UNAVAILABLE");
			ErrorMessage = TEXT("Editor actor subsystem is unavailable.");
			return nullptr;
		}

		FScopedTransaction Transaction(LOCTEXT("DuplicateSelectedActorsTransaction", "Revolt Duplicate Selected Actors"));
		TArray<AActor*> DuplicatedActors = EditorActorSubsystem->DuplicateActors(ActorsToDuplicate, World, FVector::ZeroVector);
		for (AActor* DuplicatedActor : DuplicatedActors)
		{
			if (DuplicatedActor)
			{
				DuplicatedActor->Modify();
				DuplicatedActor->Tags.AddUnique(FName(TEXT("Revolt.Generated")));
			}
		}
		Result->SetNumberField(TEXT("duplicated_count"), DuplicatedActors.Num());
	}

	ResultSummary = bDryRun ? TEXT("Duplicate selected actors preview generated") : TEXT("Selected actors duplicated and tagged Revolt.Generated");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleDeleteGeneratedActorsOnly(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	TArray<AActor*> ActorsToDelete;
	const TArray<TSharedPtr<FJsonValue>>* ActorValues = nullptr;
	if (Params->TryGetArrayField(TEXT("actors"), ActorValues) || Params->TryGetArrayField(TEXT("actor_paths"), ActorValues))
	{
		for (const TSharedPtr<FJsonValue>& ActorValue : *ActorValues)
		{
			const FString ActorIdentifier = ActorValue->AsString();
			AActor* Actor = FindActorByPathOrName(ActorIdentifier);
			if (!Actor)
			{
				ErrorCode = TEXT("ACTOR_NOT_FOUND");
				ErrorMessage = FString::Printf(TEXT("Actor '%s' was not found."), *ActorIdentifier);
				return nullptr;
			}
			ActorsToDelete.Add(Actor);
		}
	}
	else
	{
		USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
		if (!SelectedActors || SelectedActors->Num() == 0)
		{
			ErrorCode = TEXT("NO_SELECTED_ACTORS");
			ErrorMessage = TEXT("No actors were specified and no actors are selected.");
			return nullptr;
		}

		for (FSelectionIterator SelectionIterator(*SelectedActors); SelectionIterator; ++SelectionIterator)
		{
			if (AActor* Actor = Cast<AActor>(*SelectionIterator))
			{
				ActorsToDelete.Add(Actor);
			}
		}
	}

	if (ActorsToDelete.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("delete_generated_actors_only requires at least one actor.");
		return nullptr;
	}

	for (AActor* Actor : ActorsToDelete)
	{
		if (!IsGeneratedActor(Actor))
		{
			ErrorCode = TEXT("UNSAFE_DELETE_REFUSED");
			ErrorMessage = FString::Printf(TEXT("Actor '%s' is not tagged Revolt.Generated and will not be deleted."), *GetActorTargetName(Actor));
			return nullptr;
		}
	}

	Target = FString::Printf(TEXT("%d generated actor(s)"), ActorsToDelete.Num());
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("delete_count"), ActorsToDelete.Num());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (!EditorActorSubsystem)
		{
			ErrorCode = TEXT("EDITOR_SUBSYSTEM_UNAVAILABLE");
			ErrorMessage = TEXT("Editor actor subsystem is unavailable.");
			return nullptr;
		}

		FScopedTransaction Transaction(LOCTEXT("DeleteGeneratedActorsTransaction", "Revolt Delete Generated Actors"));
		int32 DeletedCount = 0;
		for (AActor* Actor : ActorsToDelete)
		{
			if (Actor)
			{
				Actor->Modify();
				if (EditorActorSubsystem->DestroyActor(Actor))
				{
					++DeletedCount;
				}
			}
		}
		Result->SetNumberField(TEXT("deleted_count"), DeletedCount);
	}

	ResultSummary = bDryRun ? TEXT("Generated actor delete preview generated") : TEXT("Generated actors deleted");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateFolder(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString FolderPath;
	if (!Params->TryGetStringField(TEXT("path"), FolderPath) && !Params->TryGetStringField(TEXT("folder_path"), FolderPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("create_folder requires path or folder_path.");
		return nullptr;
	}

	FString PackagePath;
	if (!NormalizeGeneratedPackagePath(FolderPath, PackagePath, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	Target = PackagePath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("folder_path"), PackagePath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetBoolField(TEXT("already_exists"), FPackageName::DoesPackageExist(PackagePath) || IFileManager::Get().DirectoryExists(*FPackageName::LongPackageNameToFilename(PackagePath)));

	if (!EnsureGeneratedFolderExists(PackagePath, bDryRun, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	ResultSummary = bDryRun ? TEXT("Generated folder creation preview generated") : TEXT("Generated folder created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateDataAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString PackagePath;
	FString AssetName;
	FString AssetPath;
	if (!NormalizeGeneratedAssetPath(Params, PackagePath, AssetName, AssetPath, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	FString ClassName = TEXT("/Script/Engine.DataAsset");
	Params->TryGetStringField(TEXT("class"), ClassName);
	Params->TryGetStringField(TEXT("class_path"), ClassName);
	UClass* DataAssetClass = ResolveApprovedDataAssetClass(ClassName, ErrorCode, ErrorMessage);
	if (!DataAssetClass)
	{
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("class"), DataAssetClass->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (FindObject<UObject>(nullptr, *AssetPath))
	{
		ErrorCode = TEXT("ASSET_ALREADY_EXISTS");
		ErrorMessage = FString::Printf(TEXT("Asset '%s' already exists."), *AssetPath);
		return nullptr;
	}

	if (!bDryRun)
	{
		if (!EnsureGeneratedFolderExists(PackagePath, false, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}

		FScopedTransaction Transaction(LOCTEXT("CreateDataAssetTransaction", "Revolt Create Data Asset"));
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = DataAssetClass;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, DataAssetClass, Factory);
		if (!NewAsset)
		{
			ErrorCode = TEXT("ASSET_CREATE_FAILED");
			ErrorMessage = TEXT("Unreal failed to create the data asset.");
			return nullptr;
		}
		NewAsset->MarkPackageDirty();
		Result->SetStringField(TEXT("created_asset"), NewAsset->GetPathName());
	}

	ResultSummary = bDryRun ? TEXT("Data asset creation preview generated") : TEXT("Generated data asset created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString PackagePath;
	FString AssetName;
	FString AssetPath;
	if (!NormalizeGeneratedAssetPath(Params, PackagePath, AssetName, AssetPath, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	FString ParentMaterialPath;
	if (!Params->TryGetStringField(TEXT("parent_material"), ParentMaterialPath) && !Params->TryGetStringField(TEXT("parent"), ParentMaterialPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("create_material_instance requires parent_material.");
		return nullptr;
	}

	UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *ParentMaterialPath));
	if (!ParentMaterial)
	{
		ErrorCode = TEXT("PARENT_MATERIAL_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Parent material '%s' was not found."), *ParentMaterialPath);
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("parent_material"), ParentMaterial->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (FindObject<UObject>(nullptr, *AssetPath))
	{
		ErrorCode = TEXT("ASSET_ALREADY_EXISTS");
		ErrorMessage = FString::Printf(TEXT("Asset '%s' already exists."), *AssetPath);
		return nullptr;
	}

	if (!bDryRun)
	{
		if (!EnsureGeneratedFolderExists(PackagePath, false, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}

		FScopedTransaction Transaction(LOCTEXT("CreateMaterialInstanceTransaction", "Revolt Create Material Instance"));
		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = ParentMaterial;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UMaterialInstanceConstant::StaticClass(), Factory);
		if (!NewAsset)
		{
			ErrorCode = TEXT("ASSET_CREATE_FAILED");
			ErrorMessage = TEXT("Unreal failed to create the material instance.");
			return nullptr;
		}
		NewAsset->MarkPackageDirty();
		Result->SetStringField(TEXT("created_asset"), NewAsset->GetPathName());
	}

	ResultSummary = bDryRun ? TEXT("Material instance creation preview generated") : TEXT("Generated material instance created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset"), AssetPath) && !Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_material_instance_parameter requires asset or asset_path.");
		return nullptr;
	}
	if (!IsGeneratedAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Material instance parameter edits are limited to /Game/RevoltGenerated assets.");
		return nullptr;
	}

	UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(LoadAssetForMutation(AssetPath, UMaterialInstanceConstant::StaticClass(), ErrorCode, ErrorMessage));
	if (!MaterialInstance)
	{
		return nullptr;
	}

	FString ParameterName;
	FString ParameterType;
	if (!Params->TryGetStringField(TEXT("parameter"), ParameterName) && !Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_material_instance_parameter requires parameter.");
		return nullptr;
	}
	Params->TryGetStringField(TEXT("type"), ParameterType);
	Params->TryGetStringField(TEXT("parameter_type"), ParameterType);
	ParameterType = ParameterType.IsEmpty() ? TEXT("scalar") : ParameterType.ToLower();

	Target = FString::Printf(TEXT("%s.%s"), *AssetPath, *ParameterName);
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("target"), Target);
	Result->SetStringField(TEXT("parameter_type"), ParameterType);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	const FMaterialParameterInfo ParameterInfo{ FName(*ParameterName) };
	if (ParameterType == TEXT("scalar"))
	{
		double ScalarValue = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), ScalarValue))
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = TEXT("Scalar material parameter requires numeric value.");
			return nullptr;
		}
		Result->SetNumberField(TEXT("after"), ScalarValue);
		if (!bDryRun)
		{
			FScopedTransaction Transaction(LOCTEXT("SetMaterialScalarTransaction", "Revolt Set Material Scalar Parameter"));
			MaterialInstance->Modify();
			MaterialInstance->SetScalarParameterValueEditorOnly(ParameterInfo, static_cast<float>(ScalarValue));
			MaterialInstance->MarkPackageDirty();
		}
	}
	else if (ParameterType == TEXT("vector"))
	{
		const TSharedPtr<FJsonObject>* ValueObject = nullptr;
		if (!Params->TryGetObjectField(TEXT("value"), ValueObject))
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = TEXT("Vector material parameter requires value object.");
			return nullptr;
		}
		double R = 0.0;
		double G = 0.0;
		double B = 0.0;
		double A = 1.0;
		if (!(*ValueObject)->TryGetNumberField(TEXT("r"), R)) { (*ValueObject)->TryGetNumberField(TEXT("x"), R); }
		if (!(*ValueObject)->TryGetNumberField(TEXT("g"), G)) { (*ValueObject)->TryGetNumberField(TEXT("y"), G); }
		if (!(*ValueObject)->TryGetNumberField(TEXT("b"), B)) { (*ValueObject)->TryGetNumberField(TEXT("z"), B); }
		if (!(*ValueObject)->TryGetNumberField(TEXT("a"), A)) { (*ValueObject)->TryGetNumberField(TEXT("w"), A); }
		const FLinearColor Color(static_cast<float>(R), static_cast<float>(G), static_cast<float>(B), static_cast<float>(A));
		TSharedPtr<FJsonObject> ColorObject = MakeShared<FJsonObject>();
		ColorObject->SetNumberField(TEXT("r"), Color.R);
		ColorObject->SetNumberField(TEXT("g"), Color.G);
		ColorObject->SetNumberField(TEXT("b"), Color.B);
		ColorObject->SetNumberField(TEXT("a"), Color.A);
		Result->SetObjectField(TEXT("after"), ColorObject);
		if (!bDryRun)
		{
			FScopedTransaction Transaction(LOCTEXT("SetMaterialVectorTransaction", "Revolt Set Material Vector Parameter"));
			MaterialInstance->Modify();
			MaterialInstance->SetVectorParameterValueEditorOnly(ParameterInfo, Color);
			MaterialInstance->MarkPackageDirty();
		}
	}
	else
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Only scalar and vector material instance parameters are supported in Phase 4.");
		return nullptr;
	}

	ResultSummary = bDryRun ? TEXT("Material parameter change preview generated") : TEXT("Generated material instance parameter changed");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleBulkEditDataAssets(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	const TArray<TSharedPtr<FJsonValue>>* AssetValues = nullptr;
	const TSharedPtr<FJsonObject>* ChangesObject = nullptr;
	if (!Params->TryGetArrayField(TEXT("assets"), AssetValues) || !Params->TryGetObjectField(TEXT("changes"), ChangesObject))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("bulk_edit_data_assets requires assets array and changes object.");
		return nullptr;
	}

	TUniquePtr<FScopedTransaction> Transaction;
	if (!bDryRun)
	{
		Transaction = MakeUnique<FScopedTransaction>(LOCTEXT("BulkEditDataAssetsTransaction", "Revolt Bulk Edit Data Assets"));
	}

	TArray<TSharedPtr<FJsonValue>> PreviewValues;
	for (const TSharedPtr<FJsonValue>& AssetValue : *AssetValues)
	{
		const FString AssetPath = AssetValue->AsString();
		if (!IsGeneratedAssetPath(AssetPath))
		{
			ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
			ErrorMessage = TEXT("bulk_edit_data_assets is limited to /Game/RevoltGenerated assets.");
			return nullptr;
		}

		UObject* Asset = LoadAssetForMutation(AssetPath, UDataAsset::StaticClass(), ErrorCode, ErrorMessage);
		if (!Asset)
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> AssetPreview = MakeShared<FJsonObject>();
		AssetPreview->SetStringField(TEXT("asset"), AssetPath);
		TArray<TSharedPtr<FJsonValue>> ChangeValues;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Change : (*ChangesObject)->Values)
		{
			FString BeforeValue;
			FString AfterValue;
			if (!ApplyEditablePropertyValue(Asset, Change.Key, Change.Value, BeforeValue, AfterValue, bDryRun, ErrorCode, ErrorMessage))
			{
				return nullptr;
			}
			TSharedPtr<FJsonObject> ChangePreview = MakeShared<FJsonObject>();
			ChangePreview->SetStringField(TEXT("property"), Change.Key);
			ChangePreview->SetStringField(TEXT("before"), BeforeValue);
			ChangePreview->SetStringField(TEXT("after"), AfterValue);
			ChangeValues.Add(MakeShared<FJsonValueObject>(ChangePreview));
		}
		AssetPreview->SetArrayField(TEXT("changes"), ChangeValues);
		PreviewValues.Add(MakeShared<FJsonValueObject>(AssetPreview));
	}

	Target = FString::Printf(TEXT("%d generated data asset(s)"), AssetValues->Num());
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("preview"), PreviewValues);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetNumberField(TEXT("asset_count"), AssetValues->Num());
	ResultSummary = bDryRun ? TEXT("Bulk data asset edit preview generated") : TEXT("Generated data assets edited");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSaveAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset"), AssetPath) && !Params->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("save_asset requires asset or asset_path.");
		return nullptr;
	}

	bool bAllowUserAssetSave = false;
	Params->TryGetBoolField(TEXT("allow_user_asset_save"), bAllowUserAssetSave);
	if (!IsGeneratedAssetPath(AssetPath) && !bAllowUserAssetSave)
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("save_asset refuses user assets unless allow_user_asset_save is explicitly true and the command is approved.");
		return nullptr;
	}

	UObject* Asset = LoadAssetForMutation(AssetPath, UObject::StaticClass(), ErrorCode, ErrorMessage);
	if (!Asset)
	{
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetBoolField(TEXT("is_generated_asset"), IsGeneratedAssetPath(AssetPath));

	if (!bDryRun && !SavePackageForAsset(Asset, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	ResultSummary = bDryRun ? TEXT("Asset save preview generated") : TEXT("Asset package saved by explicit command");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSaveGeneratedAssets(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FARFilter Filter;
	Filter.PackagePaths.Add(TEXT("/Game/RevoltGenerated"));
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TArray<TSharedPtr<FJsonValue>> AssetValues;
	int32 SavedCount = 0;
	for (const FAssetData& AssetDatum : AssetData)
	{
		const FString AssetPath = AssetDatum.GetObjectPathString();
		AssetValues.Add(MakeShared<FJsonValueString>(AssetPath));
		if (!bDryRun)
		{
			if (UObject* Asset = AssetDatum.GetAsset())
			{
				if (SavePackageForAsset(Asset, ErrorCode, ErrorMessage))
				{
					++SavedCount;
				}
				else
				{
					return nullptr;
				}
			}
		}
	}

	Target = TEXT("/Game/RevoltGenerated");
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("assets"), AssetValues);
	Result->SetNumberField(TEXT("asset_count"), AssetValues.Num());
	Result->SetNumberField(TEXT("saved_count"), SavedCount);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	ResultSummary = bDryRun ? TEXT("Generated asset save preview generated") : TEXT("Generated assets saved by explicit command");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateBlueprintClass(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString PackagePath;
	FString AssetName;
	FString AssetPath;
	if (!NormalizeGeneratedBlueprintAssetPath(Params, PackagePath, AssetName, AssetPath, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	FString ParentClassName = TEXT("Actor");
	Params->TryGetStringField(TEXT("parent_class"), ParentClassName);
	Params->TryGetStringField(TEXT("parent"), ParentClassName);
	UClass* ParentClass = ResolveSafeBlueprintParentClass(ParentClassName, ErrorCode, ErrorMessage);
	if (!ParentClass)
	{
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("parent_class"), ParentClass->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (FindObject<UObject>(nullptr, *AssetPath) || StaticLoadObject(UBlueprint::StaticClass(), nullptr, *AssetPath))
	{
		ErrorCode = TEXT("ASSET_ALREADY_EXISTS");
		ErrorMessage = FString::Printf(TEXT("Blueprint '%s' already exists."), *AssetPath);
		return nullptr;
	}

	if (!bDryRun)
	{
		if (!EnsureGeneratedFolderExists(PackagePath, false, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}

		FScopedTransaction Transaction(LOCTEXT("CreateBlueprintClassTransaction", "Revolt Create Blueprint Class"));
		UPackage* Package = CreatePackage(*AssetPath);
		UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
			ParentClass,
			Package,
			FName(*AssetName),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass(),
			FName(TEXT("RevoltEditorBridge")));

		if (!NewBlueprint)
		{
			ErrorCode = TEXT("BLUEPRINT_CREATE_FAILED");
			ErrorMessage = TEXT("Unreal failed to create the Blueprint class.");
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
		FAssetRegistryModule::AssetCreated(NewBlueprint);
		NewBlueprint->MarkPackageDirty();
		Result->SetStringField(TEXT("created_blueprint"), NewBlueprint->GetPathName());
		Result->SetStringField(TEXT("compile_status"), GetBlueprintCompileStatusString(NewBlueprint));
	}

	ResultSummary = bDryRun ? TEXT("Blueprint class creation preview generated") : TEXT("Generated Blueprint class created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint"), BlueprintPath) && !Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("add_blueprint_variable requires blueprint or blueprint_path.");
		return nullptr;
	}

	bool bAllowUserBlueprintEdit = false;
	Params->TryGetBoolField(TEXT("allow_user_blueprint_edit"), bAllowUserBlueprintEdit);
	UBlueprint* Blueprint = LoadBlueprintForMutation(BlueprintPath, bAllowUserBlueprintEdit, ErrorCode, ErrorMessage);
	if (!Blueprint)
	{
		return nullptr;
	}

	FString VariableName;
	FString VariableType;
	if (!Params->TryGetStringField(TEXT("name"), VariableName) && !Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("add_blueprint_variable requires name or variable_name.");
		return nullptr;
	}
	if (!Params->TryGetStringField(TEXT("type"), VariableType) && !Params->TryGetStringField(TEXT("variable_type"), VariableType))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("add_blueprint_variable requires type or variable_type.");
		return nullptr;
	}

	if (!IsValidBlueprintVariableName(VariableName, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	FEdGraphPinType PinType;
	if (!MakeSupportedBlueprintPinType(VariableType, PinType, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(*VariableName)) != INDEX_NONE)
	{
		ErrorCode = TEXT("BLUEPRINT_VARIABLE_EXISTS");
		ErrorMessage = FString::Printf(TEXT("Blueprint variable '%s' already exists."), *VariableName);
		return nullptr;
	}

	Target = FString::Printf(TEXT("%s.%s"), *BlueprintPath, *VariableName);
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Result->SetStringField(TEXT("variable_name"), VariableName);
	Result->SetStringField(TEXT("variable_type"), VariableType);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("AddBlueprintVariableTransaction", "Revolt Add Blueprint Variable"));
		Blueprint->Modify();
		FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);
		if (!CompileBlueprintAndValidate(Blueprint, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}
		Blueprint->MarkPackageDirty();
		Result->SetStringField(TEXT("compile_status"), GetBlueprintCompileStatusString(Blueprint));
	}

	ResultSummary = bDryRun ? TEXT("Blueprint variable add preview generated") : TEXT("Blueprint variable added and compiled");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSetBlueprintDefaultValue(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint"), BlueprintPath) && !Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_blueprint_default_value requires blueprint or blueprint_path.");
		return nullptr;
	}

	bool bAllowUserBlueprintEdit = false;
	Params->TryGetBoolField(TEXT("allow_user_blueprint_edit"), bAllowUserBlueprintEdit);
	UBlueprint* Blueprint = LoadBlueprintForMutation(BlueprintPath, bAllowUserBlueprintEdit, ErrorCode, ErrorMessage);
	if (!Blueprint)
	{
		return nullptr;
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("name"), VariableName) && !Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_blueprint_default_value requires name or variable_name.");
		return nullptr;
	}

	if (!Params->HasField(TEXT("value")))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("set_blueprint_default_value requires value.");
		return nullptr;
	}

	if (!Blueprint->GeneratedClass)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}

	UObject* Defaults = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject() : nullptr;
	if (!Defaults)
	{
		ErrorCode = TEXT("BLUEPRINT_COMPILE_FAILED");
		ErrorMessage = TEXT("Blueprint default object is unavailable.");
		return nullptr;
	}

	FString BeforeValue;
	FString AfterValue;
	if (!ApplyEditablePropertyValue(Defaults, VariableName, Params->TryGetField(TEXT("value")), BeforeValue, AfterValue, bDryRun, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	Target = FString::Printf(TEXT("%s.%s"), *BlueprintPath, *VariableName);
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("target"), Target);
	Result->SetStringField(TEXT("before"), BeforeValue);
	Result->SetStringField(TEXT("after"), AfterValue);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SetBlueprintDefaultValueTransaction", "Revolt Set Blueprint Default Value"));
		Blueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		if (!CompileBlueprintAndValidate(Blueprint, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}
		Blueprint->MarkPackageDirty();
		Result->SetStringField(TEXT("compile_status"), GetBlueprintCompileStatusString(Blueprint));
	}

	ResultSummary = bDryRun ? TEXT("Blueprint default value preview generated") : TEXT("Blueprint default value set and compiled");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint"), BlueprintPath) && !Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("add_component_to_blueprint requires blueprint or blueprint_path.");
		return nullptr;
	}

	bool bAllowUserBlueprintEdit = false;
	Params->TryGetBoolField(TEXT("allow_user_blueprint_edit"), bAllowUserBlueprintEdit);
	UBlueprint* Blueprint = LoadBlueprintForMutation(BlueprintPath, bAllowUserBlueprintEdit, ErrorCode, ErrorMessage);
	if (!Blueprint)
	{
		return nullptr;
	}

	if (!Blueprint->SimpleConstructionScript)
	{
		ErrorCode = TEXT("BLUEPRINT_COMPONENTS_NOT_SUPPORTED");
		ErrorMessage = TEXT("This Blueprint type does not support SimpleConstructionScript components.");
		return nullptr;
	}

	FString ComponentClassName;
	if (!Params->TryGetStringField(TEXT("component_class"), ComponentClassName) && !Params->TryGetStringField(TEXT("class"), ComponentClassName))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("add_component_to_blueprint requires component_class.");
		return nullptr;
	}

	UClass* ComponentClass = ResolveAllowedBlueprintComponentClass(ComponentClassName, ErrorCode, ErrorMessage);
	if (!ComponentClass)
	{
		return nullptr;
	}

	FString ComponentName = ComponentClass->GetName();
	Params->TryGetStringField(TEXT("component_name"), ComponentName);
	Params->TryGetStringField(TEXT("name"), ComponentName);
	if (!IsValidBlueprintVariableName(ComponentName, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	Target = FString::Printf(TEXT("%s.%s"), *BlueprintPath, *ComponentName);
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Result->SetStringField(TEXT("component_name"), ComponentName);
	Result->SetStringField(TEXT("component_class"), ComponentClass->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("AddComponentToBlueprintTransaction", "Revolt Add Component To Blueprint"));
		Blueprint->Modify();
		Blueprint->SimpleConstructionScript->Modify();
		USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, FName(*ComponentName));
		if (!NewNode)
		{
			ErrorCode = TEXT("COMPONENT_CREATE_FAILED");
			ErrorMessage = TEXT("Unreal failed to create the Blueprint component node.");
			return nullptr;
		}
		Blueprint->SimpleConstructionScript->AddNode(NewNode);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		if (!CompileBlueprintAndValidate(Blueprint, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}
		Blueprint->MarkPackageDirty();
		Result->SetStringField(TEXT("compile_status"), GetBlueprintCompileStatusString(Blueprint));
	}

	ResultSummary = bDryRun ? TEXT("Blueprint component add preview generated") : TEXT("Blueprint component added and compiled");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint"), BlueprintPath) && !Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("compile_blueprint requires blueprint or blueprint_path.");
		return nullptr;
	}

	bool bAllowUserBlueprintEdit = false;
	Params->TryGetBoolField(TEXT("allow_user_blueprint_edit"), bAllowUserBlueprintEdit);
	UBlueprint* Blueprint = LoadBlueprintForMutation(BlueprintPath, bAllowUserBlueprintEdit, ErrorCode, ErrorMessage);
	if (!Blueprint)
	{
		return nullptr;
	}

	Target = BlueprintPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetStringField(TEXT("before_status"), GetBlueprintCompileStatusString(Blueprint));

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("CompileBlueprintTransaction", "Revolt Compile Blueprint"));
		if (!CompileBlueprintAndValidate(Blueprint, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}
		Result->SetStringField(TEXT("after_status"), GetBlueprintCompileStatusString(Blueprint));
	}

	ResultSummary = bDryRun ? TEXT("Blueprint compile preview generated") : TEXT("Blueprint compiled");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleGetBlueprintCompileStatus(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	FString BlueprintPath;
	if (!Params.IsValid() || (!Params->TryGetStringField(TEXT("blueprint"), BlueprintPath) && !Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath)))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("get_blueprint_compile_status requires blueprint or blueprint_path.");
		return nullptr;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath));
	if (!Blueprint)
	{
		ErrorCode = TEXT("BLUEPRINT_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Blueprint '%s' was not found."), *BlueprintPath);
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Result->SetStringField(TEXT("compile_status"), GetBlueprintCompileStatusString(Blueprint));
	Result->SetBoolField(TEXT("has_generated_class"), Blueprint->GeneratedClass != nullptr);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateBiomeAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("create_biome_asset requires params.");
		return nullptr;
	}

	TSharedPtr<FJsonObject> BiomeJson;
	if (!ReadBiomeJsonFromParams(Params, BiomeJson, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	URevoltBiomeDataAsset* PreviewBiome = NewObject<URevoltBiomeDataAsset>(GetTransientPackage());
	PreviewBiome->FromJsonObject(BiomeJson);
	TArray<FString> ValidationErrors;
	PreviewBiome->ValidateBiome(ValidationErrors);
	if (!ValidationErrors.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_BIOME");
		ErrorMessage = FString::Join(ValidationErrors, TEXT(" "));
		return nullptr;
	}

	FString PackagePath;
	FString AssetName;
	FString AssetPath;
	if (!Params->HasField(TEXT("asset_name")) && !Params->HasField(TEXT("name")) && !Params->HasField(TEXT("asset_path")) && !Params->HasField(TEXT("asset")))
	{
		PackagePath = TEXT("/Game/RevoltGenerated/Biomes");
		AssetName = ObjectTools::SanitizeObjectName(PreviewBiome->BiomeName);
		AssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
	}
	else if (!NormalizeGeneratedBiomeAssetPath(Params, PackagePath, AssetName, AssetPath, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}
	Target = AssetPath;

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetObjectField(TEXT("biome"), PreviewBiome->ToJsonObject());
	Result->SetObjectField(TEXT("validation"), MakeBiomeValidationResult(PreviewBiome));

	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Biome asset create preview generated for %s"), *AssetPath);
		return Result;
	}

	if (!EnsureGeneratedFolderExists(PackagePath, false, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	FScopedTransaction Transaction(LOCTEXT("CreateBiomeAssetTransaction", "Revolt Create Biome Asset"));
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = URevoltBiomeDataAsset::StaticClass();
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, URevoltBiomeDataAsset::StaticClass(), Factory);
	URevoltBiomeDataAsset* BiomeAsset = Cast<URevoltBiomeDataAsset>(NewAsset);
	if (!BiomeAsset)
	{
		ErrorCode = TEXT("CREATE_FAILED");
		ErrorMessage = TEXT("Unreal failed to create the biome data asset.");
		return nullptr;
	}

	BiomeAsset->Modify();
	BiomeAsset->FromJsonObject(BiomeJson);
	BiomeAsset->MarkPackageDirty();
	Result->SetStringField(TEXT("status"), TEXT("created"));
	Result->SetStringField(TEXT("created_asset"), BiomeAsset->GetPathName());
	ResultSummary = FString::Printf(TEXT("Biome asset created at %s"), *BiomeAsset->GetPathName());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleUpdateBiomeAsset(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("update_biome_asset requires params.");
		return nullptr;
	}

	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("update_biome_asset requires asset_path.");
		return nullptr;
	}
	Target = AssetPath;

	if (!IsGeneratedBiomeAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Biome assets may only be updated under /Game/RevoltGenerated/Biomes.");
		return nullptr;
	}

	TSharedPtr<FJsonObject> BiomeJson;
	if (!ReadBiomeJsonFromParams(Params, BiomeJson, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	URevoltBiomeDataAsset* PreviewBiome = NewObject<URevoltBiomeDataAsset>(GetTransientPackage());
	PreviewBiome->FromJsonObject(BiomeJson);
	TArray<FString> ValidationErrors;
	PreviewBiome->ValidateBiome(ValidationErrors);
	if (!ValidationErrors.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_BIOME");
		ErrorMessage = FString::Join(ValidationErrors, TEXT(" "));
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetObjectField(TEXT("biome"), PreviewBiome->ToJsonObject());
	Result->SetObjectField(TEXT("validation"), MakeBiomeValidationResult(PreviewBiome));

	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Biome asset update preview generated for %s"), *AssetPath);
		return Result;
	}

	URevoltBiomeDataAsset* BiomeAsset = LoadBiomeAsset(AssetPath, ErrorCode, ErrorMessage);
	if (!BiomeAsset)
	{
		return nullptr;
	}

	FScopedTransaction Transaction(LOCTEXT("UpdateBiomeAssetTransaction", "Revolt Update Biome Asset"));
	BiomeAsset->Modify();
	BiomeAsset->FromJsonObject(BiomeJson);
	BiomeAsset->MarkPackageDirty();
	Result->SetStringField(TEXT("status"), TEXT("updated"));
	ResultSummary = FString::Printf(TEXT("Biome asset updated at %s"), *BiomeAsset->GetPathName());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleReadBiomeAsset(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("read_biome_asset requires params.");
		return nullptr;
	}

	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("read_biome_asset requires asset_path.");
		return nullptr;
	}

	URevoltBiomeDataAsset* BiomeAsset = LoadBiomeAsset(AssetPath, ErrorCode, ErrorMessage);
	if (!BiomeAsset)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), BiomeAsset->GetPathName());
	Result->SetObjectField(TEXT("biome"), BiomeAsset->ToJsonObject());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleValidateBiomeAsset(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Params.IsValid())
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("validate_biome_asset requires params.");
		return nullptr;
	}

	URevoltBiomeDataAsset* BiomeToValidate = nullptr;
	URevoltBiomeDataAsset* TransientBiome = nullptr;
	FString AssetPath;
	if (Params->TryGetStringField(TEXT("asset_path"), AssetPath) || Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		BiomeToValidate = LoadBiomeAsset(AssetPath, ErrorCode, ErrorMessage);
		if (!BiomeToValidate)
		{
			return nullptr;
		}
	}
	else
	{
		TSharedPtr<FJsonObject> BiomeJson;
		if (!ReadBiomeJsonFromParams(Params, BiomeJson, ErrorCode, ErrorMessage))
		{
			return nullptr;
		}
		TransientBiome = NewObject<URevoltBiomeDataAsset>(GetTransientPackage());
		TransientBiome->FromJsonObject(BiomeJson);
		BiomeToValidate = TransientBiome;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetObjectField(TEXT("validation"), MakeBiomeValidationResult(BiomeToValidate));
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleExportBiomeAssetJson(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> ReadResult = HandleReadBiomeAsset(Params, ErrorCode, ErrorMessage);
	if (!ReadResult.IsValid())
	{
		return nullptr;
	}

	const TSharedPtr<FJsonObject>* BiomeObject = nullptr;
	if (!ReadResult->TryGetObjectField(TEXT("biome"), BiomeObject) || !BiomeObject || !BiomeObject->IsValid())
	{
		ErrorCode = TEXT("EXPORT_FAILED");
		ErrorMessage = TEXT("Unable to export biome JSON.");
		return nullptr;
	}

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(BiomeObject->ToSharedRef(), Writer);
	ReadResult->SetStringField(TEXT("json"), JsonString);
	return ReadResult;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenValidate(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	TArray<FString> Errors = LandGenActor->GetValidationErrors();
	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}

	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetBoolField(TEXT("valid"), Errors.IsEmpty());
	Result->SetNumberField(TEXT("error_count"), Errors.Num());
	Result->SetArrayField(TEXT("errors"), ErrorValues);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenGeneratePreview(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TArray<FString> Errors = LandGenActor->GetValidationErrors();
	if (!Errors.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_LANDGEN");
		ErrorMessage = FString::Join(Errors, TEXT(" "));
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Land generation preview dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenGeneratePreviewTransaction", "Revolt Generate Terrain Preview"));
	LandGenActor->Modify();
	LandGenActor->GenerateTerrainPreview();
	Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("status"), TEXT("generated_preview"));
	ResultSummary = FString::Printf(TEXT("Generated %d land preview zones"), LandGenActor->PreviewZones.Num());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenClearPreview(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		Result->SetNumberField(TEXT("preview_zones_to_clear"), LandGenActor->PreviewZones.Num());
		ResultSummary = FString::Printf(TEXT("Land generation clear preview dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenClearPreviewTransaction", "Revolt Clear Terrain Preview"));
	LandGenActor->Modify();
	LandGenActor->ClearTerrainPreview();
	Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("status"), TEXT("cleared_preview"));
	ResultSummary = TEXT("Generated land preview content cleared");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenRandomizeSeed(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Land generation randomize seed dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenRandomizeSeedTransaction", "Revolt Randomize LandGen Seed"));
	LandGenActor->Modify();
	LandGenActor->RandomizeSeed();
	Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("status"), TEXT("randomized_seed"));
	ResultSummary = FString::Printf(TEXT("Land generation seed randomized to %d"), LandGenActor->SeedOverride);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenSpawnBiomeContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TArray<FString> Errors = LandGenActor->GetValidationErrors();
	if (!Errors.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_LANDGEN");
		ErrorMessage = FString::Join(Errors, TEXT(" "));
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	AddSpawnPreviewToResult(LandGenActor, Result);
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Biome content spawn dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenSpawnBiomeContentTransaction", "Revolt Spawn Biome Content"));
	LandGenActor->Modify();
	LandGenActor->SpawnBiomeContent();
	Result = MakeLandGenStatusObject(LandGenActor);
	AddSpawnPreviewToResult(LandGenActor, Result);
	Result->SetStringField(TEXT("status"), TEXT("spawned_biome_content"));
	ResultSummary = FString::Printf(TEXT("Spawned biome content; generated actor count is %d"), LandGenActor->GetGeneratedActorCount());
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenClearGeneratedContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetNumberField(TEXT("generated_actors_to_clear"), LandGenActor->GetGeneratedActorCount());
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Generated biome content clear dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenClearGeneratedContentTransaction", "Revolt Clear Generated Biome Content"));
	LandGenActor->Modify();
	LandGenActor->ClearGeneratedContent();
	Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("status"), TEXT("cleared_generated_content"));
	ResultSummary = TEXT("Generated biome actors cleared");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenBakeGeneratedContent(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	Target = GetActorTargetName(LandGenActor);
	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetNumberField(TEXT("generated_actors_to_bake"), LandGenActor->GetGeneratedActorCount());
	if (bDryRun)
	{
		Result->SetStringField(TEXT("status"), TEXT("dry_run"));
		ResultSummary = FString::Printf(TEXT("Generated biome content bake dry-run for %s"), *LandGenActor->GetActorNameOrLabel());
		return Result;
	}

	FScopedTransaction Transaction(LOCTEXT("LandGenBakeGeneratedContentTransaction", "Revolt Bake Generated Biome Content"));
	LandGenActor->Modify();
	LandGenActor->BakeGeneratedContent();
	Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("status"), TEXT("baked_generated_content"));
	ResultSummary = TEXT("Generated biome actors marked as baked");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleLandGenGetGeneratedSummary(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	ARevoltLandGenActor* LandGenActor = FindLandGenActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!LandGenActor)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeLandGenStatusObject(LandGenActor);
	Result->SetStringField(TEXT("summary"), LandGenActor->GetGeneratedSummary());
	AddSpawnPreviewToResult(LandGenActor, Result);
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateArenaShooterTemplate(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	Target = TEXT("/Game/RevoltGenerated/Templates/ArenaShooter");
	TArray<TSharedPtr<FJsonValue>> AssetValues;
	const TArray<FString> PlannedAssets = {
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Rifle"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Shotgun"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies/DA_Enemy_Basic"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies/DA_Enemy_Fast"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves/DA_Wave_Test"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Maps/L_ArenaShooter_Test")
	};

	if (!EnsureArenaTemplateFolders(bDryRun, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("template_path"), Target);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("CreateArenaShooterTemplateTransaction", "Revolt Create Arena Shooter Template"));

		const bool bRifleExists = StaticLoadObject(URevoltArenaWeaponData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Rifle")) != nullptr;
		URevoltArenaWeaponData* Rifle = Cast<URevoltArenaWeaponData>(CreateArenaDataAsset(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons"), TEXT("DA_Weapon_Rifle"), URevoltArenaWeaponData::StaticClass(), false, ErrorCode, ErrorMessage));
		if (Rifle && !bRifleExists)
		{
			Rifle->DisplayName = FText::FromString(TEXT("Rifle"));
			Rifle->Damage = 22.0f;
			Rifle->FireRate = 9.0f;
			Rifle->PelletCount = 1;
			Rifle->Range = 3500.0f;
			Rifle->MarkPackageDirty();
		}

		const bool bShotgunExists = StaticLoadObject(URevoltArenaWeaponData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons/DA_Weapon_Shotgun")) != nullptr;
		URevoltArenaWeaponData* Shotgun = Cast<URevoltArenaWeaponData>(CreateArenaDataAsset(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons"), TEXT("DA_Weapon_Shotgun"), URevoltArenaWeaponData::StaticClass(), false, ErrorCode, ErrorMessage));
		if (Shotgun && !bShotgunExists)
		{
			Shotgun->DisplayName = FText::FromString(TEXT("Shotgun"));
			Shotgun->Damage = 11.0f;
			Shotgun->FireRate = 1.4f;
			Shotgun->PelletCount = 8;
			Shotgun->Range = 1600.0f;
			Shotgun->MarkPackageDirty();
		}

		const bool bBasicEnemyExists = StaticLoadObject(URevoltArenaEnemyData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies/DA_Enemy_Basic")) != nullptr;
		URevoltArenaEnemyData* BasicEnemy = Cast<URevoltArenaEnemyData>(CreateArenaDataAsset(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies"), TEXT("DA_Enemy_Basic"), URevoltArenaEnemyData::StaticClass(), false, ErrorCode, ErrorMessage));
		if (BasicEnemy && !bBasicEnemyExists)
		{
			BasicEnemy->DisplayName = FText::FromString(TEXT("Basic Enemy"));
			BasicEnemy->EnemyActorClass = AActor::StaticClass();
			BasicEnemy->MaxHealth = 80.0f;
			BasicEnemy->MoveSpeed = 420.0f;
			BasicEnemy->ContactDamage = 10.0f;
			BasicEnemy->MarkPackageDirty();
		}

		const bool bFastEnemyExists = StaticLoadObject(URevoltArenaEnemyData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies/DA_Enemy_Fast")) != nullptr;
		URevoltArenaEnemyData* FastEnemy = Cast<URevoltArenaEnemyData>(CreateArenaDataAsset(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies"), TEXT("DA_Enemy_Fast"), URevoltArenaEnemyData::StaticClass(), false, ErrorCode, ErrorMessage));
		if (FastEnemy && !bFastEnemyExists)
		{
			FastEnemy->DisplayName = FText::FromString(TEXT("Fast Enemy"));
			FastEnemy->EnemyActorClass = AActor::StaticClass();
			FastEnemy->MaxHealth = 45.0f;
			FastEnemy->MoveSpeed = 720.0f;
			FastEnemy->ContactDamage = 8.0f;
			FastEnemy->MarkPackageDirty();
		}

		const bool bWaveExists = StaticLoadObject(URevoltArenaWaveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves/DA_Wave_Test")) != nullptr;
		URevoltArenaWaveData* WaveData = Cast<URevoltArenaWaveData>(CreateArenaDataAsset(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves"), TEXT("DA_Wave_Test"), URevoltArenaWaveData::StaticClass(), false, ErrorCode, ErrorMessage));
		if (WaveData && !bWaveExists)
		{
			WaveData->Waves.Empty();
			FRevoltArenaWaveEntry BasicWave;
			BasicWave.EnemyData = BasicEnemy;
			BasicWave.Count = 5;
			BasicWave.SpawnDelaySeconds = 0.25f;
			WaveData->Waves.Add(BasicWave);
			FRevoltArenaWaveEntry FastWave;
			FastWave.EnemyData = FastEnemy;
			FastWave.Count = 3;
			FastWave.SpawnDelaySeconds = 0.15f;
			WaveData->Waves.Add(FastWave);
			WaveData->MarkPackageDirty();
		}

		if (!StaticLoadObject(UWorld::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Maps/L_ArenaShooter_Test")))
		{
			UWorldFactory* WorldFactory = NewObject<UWorldFactory>();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* NewMap = AssetToolsModule.Get().CreateAsset(TEXT("L_ArenaShooter_Test"), TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Maps"), UWorld::StaticClass(), WorldFactory);
			if (NewMap)
			{
				NewMap->MarkPackageDirty();
			}
		}
	}

	for (const FString& AssetPath : PlannedAssets)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShared<FJsonObject>();
		AssetObject->SetStringField(TEXT("asset_path"), AssetPath);
		AssetObject->SetBoolField(TEXT("already_exists"), StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath) != nullptr);
		AssetValues.Add(MakeShared<FJsonValueObject>(AssetObject));
	}

	Result->SetArrayField(TEXT("assets"), AssetValues);
	Result->SetNumberField(TEXT("asset_count"), AssetValues.Num());
	ResultSummary = bDryRun ? TEXT("Arena shooter template creation preview generated") : TEXT("Arena shooter generated template assets created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleConfigureWeaponData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("configure_weapon_data requires asset_path.");
		return nullptr;
	}

	URevoltArenaWeaponData* WeaponData = LoadArenaWeaponData(AssetPath, ErrorCode, ErrorMessage);
	if (!WeaponData)
	{
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetStringField(TEXT("before_display_name"), WeaponData->DisplayName.ToString());
	Result->SetNumberField(TEXT("before_damage"), WeaponData->Damage);
	Result->SetNumberField(TEXT("before_fire_rate"), WeaponData->FireRate);
	Result->SetNumberField(TEXT("before_pellet_count"), WeaponData->PelletCount);
	Result->SetNumberField(TEXT("before_range"), WeaponData->Range);

	FString DisplayName;
	double Damage = WeaponData->Damage;
	double FireRate = WeaponData->FireRate;
	double PelletCount = WeaponData->PelletCount;
	double Range = WeaponData->Range;
	Params->TryGetStringField(TEXT("display_name"), DisplayName);
	Params->TryGetNumberField(TEXT("damage"), Damage);
	Params->TryGetNumberField(TEXT("fire_rate"), FireRate);
	Params->TryGetNumberField(TEXT("pellet_count"), PelletCount);
	Params->TryGetNumberField(TEXT("range"), Range);

	if (Damage < 0.0 || FireRate <= 0.0 || PelletCount < 1.0 || Range < 0.0)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Weapon damage/range must be non-negative, fire_rate must be positive, and pellet_count must be at least 1.");
		return nullptr;
	}

	Result->SetStringField(TEXT("after_display_name"), DisplayName.IsEmpty() ? WeaponData->DisplayName.ToString() : DisplayName);
	Result->SetNumberField(TEXT("after_damage"), Damage);
	Result->SetNumberField(TEXT("after_fire_rate"), FireRate);
	Result->SetNumberField(TEXT("after_pellet_count"), FMath::FloorToInt(PelletCount));
	Result->SetNumberField(TEXT("after_range"), Range);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("ConfigureWeaponDataTransaction", "Revolt Configure Weapon Data"));
		WeaponData->Modify();
		if (!DisplayName.IsEmpty())
		{
			WeaponData->DisplayName = FText::FromString(DisplayName);
		}
		WeaponData->Damage = static_cast<float>(Damage);
		WeaponData->FireRate = static_cast<float>(FireRate);
		WeaponData->PelletCount = FMath::FloorToInt(PelletCount);
		WeaponData->Range = static_cast<float>(Range);
		WeaponData->MarkPackageDirty();
	}

	ResultSummary = bDryRun ? TEXT("Weapon data configuration preview generated") : TEXT("Generated weapon data configured");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleConfigureEnemyData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("configure_enemy_data requires asset_path.");
		return nullptr;
	}

	URevoltArenaEnemyData* EnemyData = LoadArenaEnemyData(AssetPath, ErrorCode, ErrorMessage);
	if (!EnemyData)
	{
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetStringField(TEXT("before_display_name"), EnemyData->DisplayName.ToString());
	Result->SetNumberField(TEXT("before_max_health"), EnemyData->MaxHealth);
	Result->SetNumberField(TEXT("before_move_speed"), EnemyData->MoveSpeed);
	Result->SetNumberField(TEXT("before_contact_damage"), EnemyData->ContactDamage);

	FString DisplayName;
	FString EnemyClassPath;
	double MaxHealth = EnemyData->MaxHealth;
	double MoveSpeed = EnemyData->MoveSpeed;
	double ContactDamage = EnemyData->ContactDamage;
	Params->TryGetStringField(TEXT("display_name"), DisplayName);
	Params->TryGetStringField(TEXT("enemy_actor_class"), EnemyClassPath);
	Params->TryGetNumberField(TEXT("max_health"), MaxHealth);
	Params->TryGetNumberField(TEXT("move_speed"), MoveSpeed);
	Params->TryGetNumberField(TEXT("contact_damage"), ContactDamage);

	UClass* EnemyClass = EnemyData->EnemyActorClass ? EnemyData->EnemyActorClass.Get() : AActor::StaticClass();
	if (!EnemyClassPath.IsEmpty())
	{
		EnemyClass = ResolveSpawnClass(EnemyClassPath, ErrorCode, ErrorMessage);
		if (!EnemyClass)
		{
			return nullptr;
		}
	}

	if (MaxHealth <= 0.0 || MoveSpeed < 0.0 || ContactDamage < 0.0)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Enemy max_health must be positive; move_speed and contact_damage must be non-negative.");
		return nullptr;
	}

	Result->SetStringField(TEXT("after_display_name"), DisplayName.IsEmpty() ? EnemyData->DisplayName.ToString() : DisplayName);
	Result->SetStringField(TEXT("after_enemy_actor_class"), EnemyClass->GetPathName());
	Result->SetNumberField(TEXT("after_max_health"), MaxHealth);
	Result->SetNumberField(TEXT("after_move_speed"), MoveSpeed);
	Result->SetNumberField(TEXT("after_contact_damage"), ContactDamage);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("ConfigureEnemyDataTransaction", "Revolt Configure Enemy Data"));
		EnemyData->Modify();
		if (!DisplayName.IsEmpty())
		{
			EnemyData->DisplayName = FText::FromString(DisplayName);
		}
		EnemyData->EnemyActorClass = EnemyClass;
		EnemyData->MaxHealth = static_cast<float>(MaxHealth);
		EnemyData->MoveSpeed = static_cast<float>(MoveSpeed);
		EnemyData->ContactDamage = static_cast<float>(ContactDamage);
		EnemyData->MarkPackageDirty();
	}

	ResultSummary = bDryRun ? TEXT("Enemy data configuration preview generated") : TEXT("Generated enemy data configured");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSpawnTestArena(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	Target = World->GetMapName();
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("map_name"), World->GetMapName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetNumberField(TEXT("planned_actor_count"), 3);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SpawnTestArenaTransaction", "Revolt Spawn Arena Shooter Test Actors"));
		const FVector BaseLocation(0.0f, 0.0f, 100.0f);
		ARevoltArenaWaveSpawner* WaveSpawner = World->SpawnActor<ARevoltArenaWaveSpawner>(ARevoltArenaWaveSpawner::StaticClass(), BaseLocation, FRotator::ZeroRotator);
		ARevoltArenaPickupActor* Pickup = World->SpawnActor<ARevoltArenaPickupActor>(ARevoltArenaPickupActor::StaticClass(), BaseLocation + FVector(350.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		ARevoltArenaObjectiveActor* Objective = World->SpawnActor<ARevoltArenaObjectiveActor>(ARevoltArenaObjectiveActor::StaticClass(), BaseLocation + FVector(-350.0f, 0.0f, 0.0f), FRotator::ZeroRotator);

		URevoltArenaWaveData* WaveData = Cast<URevoltArenaWaveData>(StaticLoadObject(URevoltArenaWaveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves/DA_Wave_Test")));
		if (WaveSpawner)
		{
			WaveSpawner->Tags.AddUnique(TEXT("Revolt.Generated"));
			WaveSpawner->Tags.AddUnique(TEXT("Revolt.Template.ArenaShooter"));
			WaveSpawner->WaveData = WaveData;
		}
		if (Pickup)
		{
			Pickup->Tags.AddUnique(TEXT("Revolt.Generated"));
			Pickup->Tags.AddUnique(TEXT("Revolt.Template.ArenaShooter"));
		}
		if (Objective)
		{
			Objective->Tags.AddUnique(TEXT("Revolt.Generated"));
			Objective->Tags.AddUnique(TEXT("Revolt.Template.ArenaShooter"));
		}
	}

	ResultSummary = bDryRun ? TEXT("Test arena spawn preview generated") : TEXT("Generated arena shooter test actors spawned");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleCreateZombieShooterTemplate(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	Target = TEXT("/Game/RevoltGenerated/Templates/ZombieShooter");
	const TArray<FString> PlannedAssets = {
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons/DA_Weapon_Carbine"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons/DA_Weapon_ZombieShotgun"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Shambler"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Runner"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Brute"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves/DA_Zombie_Wave_Test"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Objectives/DA_Objective_Extract"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Maps/L_ZombieShooter_Test")
	};

	if (!EnsureZombieTemplateFolders(bDryRun, ErrorCode, ErrorMessage))
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("template_path"), Target);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);

	auto CreateTemplateAsset = [this, &ErrorCode, &ErrorMessage](const FString& PackagePath, const FString& AssetName, UClass* AssetClass) -> UObject*
	{
		const FString AssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
		UObject* ExistingAsset = StaticLoadObject(AssetClass, nullptr, *AssetPath);
		if (ExistingAsset)
		{
			return ExistingAsset;
		}

		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = AssetClass;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, AssetClass, Factory);
		if (!NewAsset)
		{
			ErrorCode = TEXT("ASSET_CREATE_FAILED");
			ErrorMessage = FString::Printf(TEXT("Failed to create zombie shooter asset '%s'."), *AssetPath);
			return nullptr;
		}
		NewAsset->MarkPackageDirty();
		return NewAsset;
	};

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("CreateZombieShooterTemplateTransaction", "Revolt Create Zombie Shooter Template"));

		const bool bCarbineExists = StaticLoadObject(URevoltArenaWeaponData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons/DA_Weapon_Carbine")) != nullptr;
		URevoltArenaWeaponData* Carbine = Cast<URevoltArenaWeaponData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons"), TEXT("DA_Weapon_Carbine"), URevoltArenaWeaponData::StaticClass()));
		if (Carbine && !bCarbineExists)
		{
			Carbine->DisplayName = FText::FromString(TEXT("Carbine"));
			Carbine->Damage = 28.0f;
			Carbine->FireRate = 7.5f;
			Carbine->Range = 4200.0f;
			Carbine->Ammo.MagazineSize = 30;
			Carbine->Ammo.ReserveAmmo = 150;
			Carbine->Ammo.ReloadSeconds = 1.9f;
			Carbine->Recoil.VerticalKick = 1.15f;
			Carbine->Recoil.HorizontalKick = 0.3f;
			Carbine->MarkPackageDirty();
		}

		const bool bShotgunExists = StaticLoadObject(URevoltArenaWeaponData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons/DA_Weapon_ZombieShotgun")) != nullptr;
		URevoltArenaWeaponData* Shotgun = Cast<URevoltArenaWeaponData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons"), TEXT("DA_Weapon_ZombieShotgun"), URevoltArenaWeaponData::StaticClass()));
		if (Shotgun && !bShotgunExists)
		{
			Shotgun->DisplayName = FText::FromString(TEXT("Zombie Shotgun"));
			Shotgun->Damage = 12.0f;
			Shotgun->FireRate = 1.2f;
			Shotgun->PelletCount = 10;
			Shotgun->Range = 1500.0f;
			Shotgun->Ammo.MagazineSize = 6;
			Shotgun->Ammo.ReserveAmmo = 48;
			Shotgun->Ammo.ReloadSeconds = 2.4f;
			Shotgun->Recoil.VerticalKick = 3.0f;
			Shotgun->Recoil.HorizontalKick = 1.1f;
			Shotgun->MarkPackageDirty();
		}

		const bool bShamblerExists = StaticLoadObject(URevoltZombieEnemyData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Shambler")) != nullptr;
		URevoltZombieEnemyData* Shambler = Cast<URevoltZombieEnemyData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies"), TEXT("DA_Zombie_Shambler"), URevoltZombieEnemyData::StaticClass()));
		if (Shambler && !bShamblerExists)
		{
			Shambler->DisplayName = FText::FromString(TEXT("Shambler"));
			Shambler->ZombieActorClass = ARevoltZombieEnemyCharacter::StaticClass();
			Shambler->MaxHealth = 85.0f;
			Shambler->MoveSpeed = 260.0f;
			Shambler->BiteDamage = 12.0f;
			Shambler->MarkPackageDirty();
		}

		const bool bRunnerExists = StaticLoadObject(URevoltZombieEnemyData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Runner")) != nullptr;
		URevoltZombieEnemyData* Runner = Cast<URevoltZombieEnemyData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies"), TEXT("DA_Zombie_Runner"), URevoltZombieEnemyData::StaticClass()));
		if (Runner && !bRunnerExists)
		{
			Runner->DisplayName = FText::FromString(TEXT("Runner"));
			Runner->ZombieActorClass = ARevoltZombieEnemyCharacter::StaticClass();
			Runner->MaxHealth = 55.0f;
			Runner->MoveSpeed = 620.0f;
			Runner->BiteDamage = 8.0f;
			Runner->SightRadius = 2200.0f;
			Runner->MarkPackageDirty();
		}

		const bool bBruteExists = StaticLoadObject(URevoltZombieEnemyData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Brute")) != nullptr;
		URevoltZombieEnemyData* Brute = Cast<URevoltZombieEnemyData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies"), TEXT("DA_Zombie_Brute"), URevoltZombieEnemyData::StaticClass()));
		if (Brute && !bBruteExists)
		{
			Brute->DisplayName = FText::FromString(TEXT("Brute"));
			Brute->ZombieActorClass = ARevoltZombieEnemyCharacter::StaticClass();
			Brute->MaxHealth = 240.0f;
			Brute->MoveSpeed = 190.0f;
			Brute->BiteDamage = 28.0f;
			Brute->bUsesSimpleCover = true;
			Brute->MarkPackageDirty();
		}

		const bool bWaveExists = StaticLoadObject(URevoltZombieWaveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves/DA_Zombie_Wave_Test")) != nullptr;
		URevoltZombieWaveData* Wave = Cast<URevoltZombieWaveData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves"), TEXT("DA_Zombie_Wave_Test"), URevoltZombieWaveData::StaticClass()));
		if (Wave && !bWaveExists)
		{
			Wave->Waves.Empty();
			FRevoltZombieWaveEntry ShamblerEntry;
			ShamblerEntry.ZombieData = Shambler;
			ShamblerEntry.Count = 8;
			Wave->Waves.Add(ShamblerEntry);
			FRevoltZombieWaveEntry RunnerEntry;
			RunnerEntry.ZombieData = Runner;
			RunnerEntry.Count = 3;
			RunnerEntry.SpawnDelaySeconds = 0.2f;
			Wave->Waves.Add(RunnerEntry);
			Wave->MarkPackageDirty();
		}

		const bool bObjectiveExists = StaticLoadObject(URevoltZombieObjectiveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Objectives/DA_Objective_Extract")) != nullptr;
		URevoltZombieObjectiveData* Objective = Cast<URevoltZombieObjectiveData>(CreateTemplateAsset(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Objectives"), TEXT("DA_Objective_Extract"), URevoltZombieObjectiveData::StaticClass()));
		if (Objective && !bObjectiveExists)
		{
			Objective->ObjectiveText = FText::FromString(TEXT("Survive the horde and reach extraction"));
			Objective->bRequiresExtraction = true;
			Objective->MarkPackageDirty();
		}

		if (!StaticLoadObject(UWorld::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Maps/L_ZombieShooter_Test")))
		{
			UWorldFactory* WorldFactory = NewObject<UWorldFactory>();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* NewMap = AssetToolsModule.Get().CreateAsset(TEXT("L_ZombieShooter_Test"), TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Maps"), UWorld::StaticClass(), WorldFactory);
			if (NewMap)
			{
				NewMap->MarkPackageDirty();
			}
		}
	}

	TArray<TSharedPtr<FJsonValue>> AssetValues;
	for (const FString& AssetPath : PlannedAssets)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShared<FJsonObject>();
		AssetObject->SetStringField(TEXT("asset_path"), AssetPath);
		AssetObject->SetBoolField(TEXT("already_exists"), StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath) != nullptr);
		AssetValues.Add(MakeShared<FJsonValueObject>(AssetObject));
	}
	Result->SetArrayField(TEXT("assets"), AssetValues);
	Result->SetNumberField(TEXT("asset_count"), AssetValues.Num());
	ResultSummary = bDryRun ? TEXT("Zombie shooter template creation preview generated") : TEXT("Zombie shooter generated template assets created");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleConfigureZombieData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("configure_zombie_data requires asset_path.");
		return nullptr;
	}

	URevoltZombieEnemyData* ZombieData = LoadZombieEnemyData(AssetPath, ErrorCode, ErrorMessage);
	if (!ZombieData)
	{
		return nullptr;
	}

	FString DisplayName;
	FString ZombieClassPath;
	double MaxHealth = ZombieData->MaxHealth;
	double MoveSpeed = ZombieData->MoveSpeed;
	double BiteDamage = ZombieData->BiteDamage;
	double SightRadius = ZombieData->SightRadius;
	double HearingRadius = ZombieData->HearingRadius;
	bool bUsesCover = ZombieData->bUsesSimpleCover;
	Params->TryGetStringField(TEXT("display_name"), DisplayName);
	Params->TryGetStringField(TEXT("zombie_actor_class"), ZombieClassPath);
	Params->TryGetNumberField(TEXT("max_health"), MaxHealth);
	Params->TryGetNumberField(TEXT("move_speed"), MoveSpeed);
	Params->TryGetNumberField(TEXT("bite_damage"), BiteDamage);
	Params->TryGetNumberField(TEXT("sight_radius"), SightRadius);
	Params->TryGetNumberField(TEXT("hearing_radius"), HearingRadius);
	Params->TryGetBoolField(TEXT("uses_simple_cover"), bUsesCover);

	UClass* ZombieClass = ZombieData->ZombieActorClass ? ZombieData->ZombieActorClass.Get() : ARevoltZombieEnemyCharacter::StaticClass();
	if (!ZombieClassPath.IsEmpty())
	{
		ZombieClass = ResolveSpawnClass(ZombieClassPath, ErrorCode, ErrorMessage);
		if (!ZombieClass)
		{
			return nullptr;
		}
	}

	if (MaxHealth <= 0.0 || MoveSpeed < 0.0 || BiteDamage < 0.0 || SightRadius < 0.0 || HearingRadius < 0.0)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Zombie health must be positive; movement, damage, sight, and hearing values must be non-negative.");
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetStringField(TEXT("after_display_name"), DisplayName.IsEmpty() ? ZombieData->DisplayName.ToString() : DisplayName);
	Result->SetStringField(TEXT("after_zombie_actor_class"), ZombieClass->GetPathName());
	Result->SetNumberField(TEXT("after_max_health"), MaxHealth);
	Result->SetNumberField(TEXT("after_move_speed"), MoveSpeed);
	Result->SetNumberField(TEXT("after_bite_damage"), BiteDamage);
	Result->SetNumberField(TEXT("after_sight_radius"), SightRadius);
	Result->SetNumberField(TEXT("after_hearing_radius"), HearingRadius);
	Result->SetBoolField(TEXT("after_uses_simple_cover"), bUsesCover);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("ConfigureZombieDataTransaction", "Revolt Configure Zombie Data"));
		ZombieData->Modify();
		if (!DisplayName.IsEmpty())
		{
			ZombieData->DisplayName = FText::FromString(DisplayName);
		}
		ZombieData->ZombieActorClass = ZombieClass;
		ZombieData->MaxHealth = static_cast<float>(MaxHealth);
		ZombieData->MoveSpeed = static_cast<float>(MoveSpeed);
		ZombieData->BiteDamage = static_cast<float>(BiteDamage);
		ZombieData->SightRadius = static_cast<float>(SightRadius);
		ZombieData->HearingRadius = static_cast<float>(HearingRadius);
		ZombieData->bUsesSimpleCover = bUsesCover;
		ZombieData->MarkPackageDirty();
	}

	ResultSummary = bDryRun ? TEXT("Zombie data configuration preview generated") : TEXT("Generated zombie data configured");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleConfigureWeaponRecoil(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("configure_weapon_recoil requires asset_path.");
		return nullptr;
	}
	if (!IsZombieTemplateAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Weapon recoil tuning for Phase 16 is restricted to /Game/RevoltGenerated/Templates/ZombieShooter.");
		return nullptr;
	}

	URevoltArenaWeaponData* WeaponData = Cast<URevoltArenaWeaponData>(LoadAssetForMutation(AssetPath, URevoltArenaWeaponData::StaticClass(), ErrorCode, ErrorMessage));
	if (!WeaponData)
	{
		return nullptr;
	}

	double VerticalKick = WeaponData->Recoil.VerticalKick;
	double HorizontalKick = WeaponData->Recoil.HorizontalKick;
	double RecoverySpeed = WeaponData->Recoil.RecoverySpeed;
	double SpreadDegrees = WeaponData->Recoil.SpreadDegrees;
	double MagazineSize = WeaponData->Ammo.MagazineSize;
	double ReserveAmmo = WeaponData->Ammo.ReserveAmmo;
	double ReloadSeconds = WeaponData->Ammo.ReloadSeconds;
	bool bAllowPartialReload = WeaponData->Ammo.bAllowPartialReload;
	Params->TryGetNumberField(TEXT("vertical_kick"), VerticalKick);
	Params->TryGetNumberField(TEXT("horizontal_kick"), HorizontalKick);
	Params->TryGetNumberField(TEXT("recovery_speed"), RecoverySpeed);
	Params->TryGetNumberField(TEXT("spread_degrees"), SpreadDegrees);
	Params->TryGetNumberField(TEXT("magazine_size"), MagazineSize);
	Params->TryGetNumberField(TEXT("reserve_ammo"), ReserveAmmo);
	Params->TryGetNumberField(TEXT("reload_seconds"), ReloadSeconds);
	Params->TryGetBoolField(TEXT("allow_partial_reload"), bAllowPartialReload);

	if (VerticalKick < 0.0 || HorizontalKick < 0.0 || RecoverySpeed < 0.0 || SpreadDegrees < 0.0 || MagazineSize < 1.0 || ReserveAmmo < 0.0 || ReloadSeconds < 0.0)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Recoil, spread, reserve ammo, and reload values must be non-negative; magazine_size must be at least 1.");
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetNumberField(TEXT("after_vertical_kick"), VerticalKick);
	Result->SetNumberField(TEXT("after_horizontal_kick"), HorizontalKick);
	Result->SetNumberField(TEXT("after_recovery_speed"), RecoverySpeed);
	Result->SetNumberField(TEXT("after_spread_degrees"), SpreadDegrees);
	Result->SetNumberField(TEXT("after_magazine_size"), FMath::FloorToInt(MagazineSize));
	Result->SetNumberField(TEXT("after_reserve_ammo"), FMath::FloorToInt(ReserveAmmo));
	Result->SetNumberField(TEXT("after_reload_seconds"), ReloadSeconds);
	Result->SetBoolField(TEXT("after_allow_partial_reload"), bAllowPartialReload);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("ConfigureWeaponRecoilTransaction", "Revolt Configure Weapon Recoil"));
		WeaponData->Modify();
		WeaponData->Recoil.VerticalKick = static_cast<float>(VerticalKick);
		WeaponData->Recoil.HorizontalKick = static_cast<float>(HorizontalKick);
		WeaponData->Recoil.RecoverySpeed = static_cast<float>(RecoverySpeed);
		WeaponData->Recoil.SpreadDegrees = static_cast<float>(SpreadDegrees);
		WeaponData->Ammo.MagazineSize = FMath::FloorToInt(MagazineSize);
		WeaponData->Ammo.ReserveAmmo = FMath::FloorToInt(ReserveAmmo);
		WeaponData->Ammo.ReloadSeconds = static_cast<float>(ReloadSeconds);
		WeaponData->Ammo.bAllowPartialReload = bAllowPartialReload;
		WeaponData->MarkPackageDirty();
	}

	ResultSummary = bDryRun ? TEXT("Weapon recoil configuration preview generated") : TEXT("Generated zombie weapon recoil configured");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleSpawnZombieTestArena(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		ErrorCode = TEXT("NO_OPEN_LEVEL");
		ErrorMessage = TEXT("No open editor level is available.");
		return nullptr;
	}

	Target = World->GetMapName();
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("map_name"), World->GetMapName());
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetNumberField(TEXT("planned_actor_count"), 4);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("SpawnZombieTestArenaTransaction", "Revolt Spawn Zombie Shooter Test Arena"));
		const FVector BaseLocation(0.0f, 0.0f, 100.0f);
		ARevoltZombieWaveSpawner* WaveSpawner = World->SpawnActor<ARevoltZombieWaveSpawner>(ARevoltZombieWaveSpawner::StaticClass(), BaseLocation, FRotator::ZeroRotator);
		ARevoltZombieExtractionZone* ExtractionZone = World->SpawnActor<ARevoltZombieExtractionZone>(ARevoltZombieExtractionZone::StaticClass(), BaseLocation + FVector(900.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		ARevoltZombieDirectorActor* Director = World->SpawnActor<ARevoltZombieDirectorActor>(ARevoltZombieDirectorActor::StaticClass(), BaseLocation + FVector(0.0f, 700.0f, 0.0f), FRotator::ZeroRotator);
		ARevoltZombieObjectiveActor* Objective = World->SpawnActor<ARevoltZombieObjectiveActor>(ARevoltZombieObjectiveActor::StaticClass(), BaseLocation + FVector(-600.0f, 0.0f, 0.0f), FRotator::ZeroRotator);

		URevoltZombieWaveData* WaveData = Cast<URevoltZombieWaveData>(StaticLoadObject(URevoltZombieWaveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves/DA_Zombie_Wave_Test")));
		URevoltZombieObjectiveData* ObjectiveData = Cast<URevoltZombieObjectiveData>(StaticLoadObject(URevoltZombieObjectiveData::StaticClass(), nullptr, TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Objectives/DA_Objective_Extract")));
		if (WaveSpawner)
		{
			WaveSpawner->WaveData = WaveData;
			WaveSpawner->Tags.AddUnique(TEXT("Revolt.Generated"));
			WaveSpawner->Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
		}
		if (ExtractionZone)
		{
			ExtractionZone->Tags.AddUnique(TEXT("Revolt.Generated"));
			ExtractionZone->Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
		}
		if (Director)
		{
			Director->Tags.AddUnique(TEXT("Revolt.Generated"));
			Director->Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
		}
		if (Objective)
		{
			Objective->ObjectiveData = ObjectiveData;
			Objective->Tags.AddUnique(TEXT("Revolt.Generated"));
			Objective->Tags.AddUnique(TEXT("Revolt.Template.ZombieShooter"));
		}
	}

	ResultSummary = bDryRun ? TEXT("Zombie test arena spawn preview generated") : TEXT("Generated zombie test arena actors spawned");
	return Result;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::HandleConfigureWaveData(const TSharedPtr<FJsonObject>& Params, bool bDryRun, FString& Target, FString& ResultSummary, FString& ErrorCode, FString& ErrorMessage)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) && !Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("configure_wave_data requires asset_path.");
		return nullptr;
	}

	URevoltZombieWaveData* WaveData = LoadZombieWaveData(AssetPath, ErrorCode, ErrorMessage);
	if (!WaveData)
	{
		return nullptr;
	}

	double ShamblerCount = 8.0;
	double RunnerCount = 3.0;
	double BruteCount = 0.0;
	bool bEnableDayNightScaling = WaveData->bEnableDayNightScaling;
	bool bEnableWeatherScaling = WaveData->bEnableWeatherScaling;
	Params->TryGetNumberField(TEXT("shambler_count"), ShamblerCount);
	Params->TryGetNumberField(TEXT("runner_count"), RunnerCount);
	Params->TryGetNumberField(TEXT("brute_count"), BruteCount);
	Params->TryGetBoolField(TEXT("enable_day_night_scaling"), bEnableDayNightScaling);
	Params->TryGetBoolField(TEXT("enable_weather_scaling"), bEnableWeatherScaling);

	if (ShamblerCount < 0.0 || RunnerCount < 0.0 || BruteCount < 0.0)
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Zombie wave counts must be non-negative.");
		return nullptr;
	}

	Target = AssetPath;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetNumberField(TEXT("after_shambler_count"), FMath::FloorToInt(ShamblerCount));
	Result->SetNumberField(TEXT("after_runner_count"), FMath::FloorToInt(RunnerCount));
	Result->SetNumberField(TEXT("after_brute_count"), FMath::FloorToInt(BruteCount));
	Result->SetBoolField(TEXT("after_enable_day_night_scaling"), bEnableDayNightScaling);
	Result->SetBoolField(TEXT("after_enable_weather_scaling"), bEnableWeatherScaling);

	if (!bDryRun)
	{
		FScopedTransaction Transaction(LOCTEXT("ConfigureZombieWaveDataTransaction", "Revolt Configure Zombie Wave Data"));
		WaveData->Modify();
		WaveData->Waves.Empty();
		auto AddWaveEntry = [WaveData](const FString& ZombieAssetPath, int32 Count)
		{
			if (Count <= 0)
			{
				return;
			}
			URevoltZombieEnemyData* ZombieData = Cast<URevoltZombieEnemyData>(StaticLoadObject(URevoltZombieEnemyData::StaticClass(), nullptr, *ZombieAssetPath));
			if (ZombieData)
			{
				FRevoltZombieWaveEntry Entry;
				Entry.ZombieData = ZombieData;
				Entry.Count = Count;
				WaveData->Waves.Add(Entry);
			}
		};
		AddWaveEntry(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Shambler"), FMath::FloorToInt(ShamblerCount));
		AddWaveEntry(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Runner"), FMath::FloorToInt(RunnerCount));
		AddWaveEntry(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies/DA_Zombie_Brute"), FMath::FloorToInt(BruteCount));
		WaveData->bEnableDayNightScaling = bEnableDayNightScaling;
		WaveData->bEnableWeatherScaling = bEnableWeatherScaling;
		WaveData->MarkPackageDirty();
	}

	ResultSummary = bDryRun ? TEXT("Zombie wave data configuration preview generated") : TEXT("Generated zombie wave data configured");
	return Result;
}

bool FRevoltEditorBridgeEditorModule::IsMutationCommand(const FString& Command) const
{
	return Command == TEXT("set_actor_transform") ||
		Command == TEXT("set_actor_property") ||
		Command == TEXT("spawn_actor") ||
		Command == TEXT("duplicate_selected_actors") ||
		Command == TEXT("delete_generated_actors_only") ||
		Command == TEXT("create_folder") ||
		Command == TEXT("create_data_asset") ||
		Command == TEXT("create_material_instance") ||
		Command == TEXT("set_material_instance_parameter") ||
		Command == TEXT("bulk_edit_data_assets") ||
		Command == TEXT("save_asset") ||
		Command == TEXT("save_generated_assets") ||
		Command == TEXT("create_blueprint_class") ||
		Command == TEXT("add_blueprint_variable") ||
		Command == TEXT("set_blueprint_default_value") ||
		Command == TEXT("add_component_to_blueprint") ||
		Command == TEXT("compile_blueprint") ||
		Command == TEXT("create_biome_asset") ||
		Command == TEXT("update_biome_asset") ||
		Command == TEXT("landgen_generate_preview") ||
		Command == TEXT("landgen_clear_preview") ||
		Command == TEXT("landgen_randomize_seed") ||
		Command == TEXT("landgen_spawn_biome_content") ||
		Command == TEXT("landgen_clear_generated_content") ||
		Command == TEXT("landgen_bake_generated_content") ||
		Command == TEXT("create_arena_shooter_template") ||
		Command == TEXT("configure_weapon_data") ||
		Command == TEXT("configure_enemy_data") ||
		Command == TEXT("spawn_test_arena") ||
		Command == TEXT("create_zombie_shooter_template") ||
		Command == TEXT("configure_zombie_data") ||
		Command == TEXT("configure_weapon_recoil") ||
		Command == TEXT("spawn_zombie_test_arena") ||
		Command == TEXT("configure_wave_data");
}

bool FRevoltEditorBridgeEditorModule::HasMutationPermission(const TSharedPtr<FJsonObject>& Params) const
{
	FString PermissionLevel;
	return Params.IsValid() &&
		Params->TryGetStringField(TEXT("permission_level"), PermissionLevel) &&
		(PermissionLevel == TEXT("editor_mutation") || PermissionLevel == TEXT("asset_mutation") || PermissionLevel == TEXT("blueprint_mutation"));
}

bool FRevoltEditorBridgeEditorModule::GetDryRunFlag(const TSharedPtr<FJsonObject>& Params) const
{
	bool bDryRun = false;
	return Params.IsValid() && Params->TryGetBoolField(TEXT("dry_run"), bDryRun) && bDryRun;
}

bool FRevoltEditorBridgeEditorModule::GetApprovedFlag(const TSharedPtr<FJsonObject>& Params) const
{
	bool bApproved = false;
	return Params.IsValid() && Params->TryGetBoolField(TEXT("approved"), bApproved) && bApproved;
}

AActor* FRevoltEditorBridgeEditorModule::FindActorFromParams(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	FString ActorIdentifier;
	if (!Params->TryGetStringField(TEXT("actor"), ActorIdentifier) &&
		!Params->TryGetStringField(TEXT("actor_path"), ActorIdentifier) &&
		!Params->TryGetStringField(TEXT("actor_name"), ActorIdentifier))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Command requires actor, actor_path, or actor_name.");
		return nullptr;
	}

	AActor* Actor = FindActorByPathOrName(ActorIdentifier);
	if (!Actor)
	{
		ErrorCode = TEXT("ACTOR_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Actor '%s' was not found."), *ActorIdentifier);
		return nullptr;
	}

	return Actor;
}

AActor* FRevoltEditorBridgeEditorModule::FindActorByPathOrName(const FString& ActorIdentifier) const
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (Actor &&
			(Actor->GetPathName() == ActorIdentifier ||
			 Actor->GetName() == ActorIdentifier ||
			 Actor->GetActorNameOrLabel() == ActorIdentifier))
		{
			return Actor;
		}
	}

	return nullptr;
}

ARevoltLandGenActor* FRevoltEditorBridgeEditorModule::FindLandGenActorFromParams(const TSharedPtr<FJsonObject>& Params, FString& ErrorCode, FString& ErrorMessage) const
{
	AActor* Actor = FindActorFromParams(Params, ErrorCode, ErrorMessage);
	if (!Actor)
	{
		return nullptr;
	}

	ARevoltLandGenActor* LandGenActor = Cast<ARevoltLandGenActor>(Actor);
	if (!LandGenActor)
	{
		ErrorCode = TEXT("INVALID_ACTOR_TYPE");
		ErrorMessage = FString::Printf(TEXT("Actor '%s' is not an ARevoltLandGenActor."), *Actor->GetActorNameOrLabel());
		return nullptr;
	}

	return LandGenActor;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::MakeLandGenStatusObject(const ARevoltLandGenActor* LandGenActor) const
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	if (!LandGenActor)
	{
		return Result;
	}

	Result->SetStringField(TEXT("actor"), LandGenActor->GetPathName());
	Result->SetStringField(TEXT("actor_name"), LandGenActor->GetActorNameOrLabel());
	Result->SetStringField(TEXT("biome"), LandGenActor->Biome ? LandGenActor->Biome->GetPathName() : TEXT(""));
	Result->SetNumberField(TEXT("seed"), LandGenActor->GetEffectiveSeed());
	Result->SetNumberField(TEXT("seed_override"), LandGenActor->SeedOverride);
	Result->SetBoolField(TEXT("preview_mode"), LandGenActor->bPreviewMode);
	Result->SetStringField(TEXT("generated_content_folder_name"), LandGenActor->GeneratedContentFolderName);

	TSharedPtr<FJsonObject> Bounds = MakeShared<FJsonObject>();
	Bounds->SetNumberField(TEXT("x"), LandGenActor->GenerationBounds.X);
	Bounds->SetNumberField(TEXT("y"), LandGenActor->GenerationBounds.Y);
	Bounds->SetNumberField(TEXT("z"), LandGenActor->GenerationBounds.Z);
	Result->SetObjectField(TEXT("generation_bounds"), Bounds);

	TArray<TSharedPtr<FJsonValue>> ZoneValues;
	for (const FRevoltLandGenPreviewZone& Zone : LandGenActor->PreviewZones)
	{
		TSharedPtr<FJsonObject> ZoneObject = MakeShared<FJsonObject>();
		ZoneObject->SetStringField(TEXT("label"), Zone.Label);
		switch (Zone.ZoneType)
		{
		case ERevoltLandGenPreviewZoneType::TerrainRegion:
			ZoneObject->SetStringField(TEXT("type"), TEXT("terrain_region"));
			break;
		case ERevoltLandGenPreviewZoneType::CandidateSpawnZone:
			ZoneObject->SetStringField(TEXT("type"), TEXT("candidate_spawn_zone"));
			break;
		case ERevoltLandGenPreviewZoneType::BlockedZone:
			ZoneObject->SetStringField(TEXT("type"), TEXT("blocked_zone"));
			break;
		default:
			ZoneObject->SetStringField(TEXT("type"), TEXT("unknown"));
			break;
		}

		TSharedPtr<FJsonObject> Center = MakeShared<FJsonObject>();
		Center->SetNumberField(TEXT("x"), Zone.Center.X);
		Center->SetNumberField(TEXT("y"), Zone.Center.Y);
		Center->SetNumberField(TEXT("z"), Zone.Center.Z);
		ZoneObject->SetObjectField(TEXT("center"), Center);

		TSharedPtr<FJsonObject> Extent = MakeShared<FJsonObject>();
		Extent->SetNumberField(TEXT("x"), Zone.Extent.X);
		Extent->SetNumberField(TEXT("y"), Zone.Extent.Y);
		Extent->SetNumberField(TEXT("z"), Zone.Extent.Z);
		ZoneObject->SetObjectField(TEXT("extent"), Extent);
		ZoneValues.Add(MakeShared<FJsonValueObject>(ZoneObject));
	}

	Result->SetArrayField(TEXT("preview_zones"), ZoneValues);
	Result->SetNumberField(TEXT("preview_zone_count"), ZoneValues.Num());
	Result->SetNumberField(TEXT("generated_actor_count"), LandGenActor->GetGeneratedActorCount());
	return Result;
}

void FRevoltEditorBridgeEditorModule::AddSpawnPreviewToResult(const ARevoltLandGenActor* LandGenActor, TSharedPtr<FJsonObject> Result) const
{
	if (!LandGenActor || !Result.IsValid())
	{
		return;
	}

	const TArray<FRevoltBiomeSpawnPreview> SpawnPreview = LandGenActor->BuildBiomeSpawnPreview();
	TArray<TSharedPtr<FJsonValue>> PlacementValues;
	for (const FRevoltBiomeSpawnPreview& Preview : SpawnPreview)
	{
		TSharedPtr<FJsonObject> Placement = MakeShared<FJsonObject>();
		Placement->SetStringField(TEXT("rule_name"), Preview.RuleName);
		Placement->SetStringField(TEXT("rule_type"), Preview.RuleType);
		Placement->SetStringField(TEXT("asset_or_class_path"), Preview.AssetOrClassPath);

		TSharedPtr<FJsonObject> Location = MakeShared<FJsonObject>();
		Location->SetNumberField(TEXT("x"), Preview.Location.X);
		Location->SetNumberField(TEXT("y"), Preview.Location.Y);
		Location->SetNumberField(TEXT("z"), Preview.Location.Z);
		Placement->SetObjectField(TEXT("location"), Location);

		TSharedPtr<FJsonObject> Rotation = MakeShared<FJsonObject>();
		Rotation->SetNumberField(TEXT("pitch"), Preview.Rotation.Pitch);
		Rotation->SetNumberField(TEXT("yaw"), Preview.Rotation.Yaw);
		Rotation->SetNumberField(TEXT("roll"), Preview.Rotation.Roll);
		Placement->SetObjectField(TEXT("rotation"), Rotation);

		TSharedPtr<FJsonObject> Scale = MakeShared<FJsonObject>();
		Scale->SetNumberField(TEXT("x"), Preview.Scale.X);
		Scale->SetNumberField(TEXT("y"), Preview.Scale.Y);
		Scale->SetNumberField(TEXT("z"), Preview.Scale.Z);
		Placement->SetObjectField(TEXT("scale"), Scale);

		PlacementValues.Add(MakeShared<FJsonValueObject>(Placement));
	}

	Result->SetArrayField(TEXT("placement_preview"), PlacementValues);
	Result->SetNumberField(TEXT("placement_preview_count"), PlacementValues.Num());
}

UClass* FRevoltEditorBridgeEditorModule::ResolveSpawnClass(const FString& ClassName, FString& ErrorCode, FString& ErrorMessage) const
{
	UClass* SpawnClass = LoadClass<AActor>(nullptr, *ClassName);
	if (!SpawnClass)
	{
		SpawnClass = FindObject<UClass>(nullptr, *ClassName);
	}

	if (!SpawnClass)
	{
		ErrorCode = TEXT("SPAWN_CLASS_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Spawn class '%s' was not found."), *ClassName);
		return nullptr;
	}

	if (!SpawnClass->IsChildOf(AActor::StaticClass()) ||
		SpawnClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NotPlaceable))
	{
		ErrorCode = TEXT("SPAWN_CLASS_NOT_ALLOWED");
		ErrorMessage = FString::Printf(TEXT("Spawn class '%s' is not an allowed placeable actor class."), *ClassName);
		return nullptr;
	}

	return SpawnClass;
}

bool FRevoltEditorBridgeEditorModule::ReadVectorObject(const TSharedPtr<FJsonObject>& Params, const FString& FieldName, FVector& OutVector, bool& bHasField, FString& ErrorCode, FString& ErrorMessage) const
{
	const TSharedPtr<FJsonObject>* Object = nullptr;
	bHasField = Params->TryGetObjectField(FieldName, Object);
	if (!bHasField)
	{
		return true;
	}

	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
	if (!(*Object)->TryGetNumberField(TEXT("x"), X) ||
		!(*Object)->TryGetNumberField(TEXT("y"), Y) ||
		!(*Object)->TryGetNumberField(TEXT("z"), Z))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = FString::Printf(TEXT("%s must include numeric x, y, and z fields."), *FieldName);
		return false;
	}

	OutVector = FVector(X, Y, Z);
	return true;
}

bool FRevoltEditorBridgeEditorModule::ReadRotatorObject(const TSharedPtr<FJsonObject>& Params, const FString& FieldName, FRotator& OutRotator, bool& bHasField, FString& ErrorCode, FString& ErrorMessage) const
{
	const TSharedPtr<FJsonObject>* Object = nullptr;
	bHasField = Params->TryGetObjectField(FieldName, Object);
	if (!bHasField)
	{
		return true;
	}

	double Pitch = 0.0;
	double Yaw = 0.0;
	double Roll = 0.0;
	if (!(*Object)->TryGetNumberField(TEXT("pitch"), Pitch) ||
		!(*Object)->TryGetNumberField(TEXT("yaw"), Yaw) ||
		!(*Object)->TryGetNumberField(TEXT("roll"), Roll))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = FString::Printf(TEXT("%s must include numeric pitch, yaw, and roll fields."), *FieldName);
		return false;
	}

	OutRotator = FRotator(Pitch, Yaw, Roll);
	return true;
}

bool FRevoltEditorBridgeEditorModule::AreTransformValuesFinite(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const
{
	return FMath::IsFinite(Location.X) &&
		FMath::IsFinite(Location.Y) &&
		FMath::IsFinite(Location.Z) &&
		FMath::IsFinite(Rotation.Pitch) &&
		FMath::IsFinite(Rotation.Yaw) &&
		FMath::IsFinite(Rotation.Roll) &&
		FMath::IsFinite(Scale.X) &&
		FMath::IsFinite(Scale.Y) &&
		FMath::IsFinite(Scale.Z);
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::MakeTransformObject(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const
{
	TSharedPtr<FJsonObject> Transform = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> LocationObject = MakeShared<FJsonObject>();
	LocationObject->SetNumberField(TEXT("x"), Location.X);
	LocationObject->SetNumberField(TEXT("y"), Location.Y);
	LocationObject->SetNumberField(TEXT("z"), Location.Z);
	Transform->SetObjectField(TEXT("location"), LocationObject);

	TSharedPtr<FJsonObject> RotationObject = MakeShared<FJsonObject>();
	RotationObject->SetNumberField(TEXT("pitch"), Rotation.Pitch);
	RotationObject->SetNumberField(TEXT("yaw"), Rotation.Yaw);
	RotationObject->SetNumberField(TEXT("roll"), Rotation.Roll);
	Transform->SetObjectField(TEXT("rotation"), RotationObject);

	TSharedPtr<FJsonObject> ScaleObject = MakeShared<FJsonObject>();
	ScaleObject->SetNumberField(TEXT("x"), Scale.X);
	ScaleObject->SetNumberField(TEXT("y"), Scale.Y);
	ScaleObject->SetNumberField(TEXT("z"), Scale.Z);
	Transform->SetObjectField(TEXT("scale"), ScaleObject);
	return Transform;
}

FString FRevoltEditorBridgeEditorModule::GetActorTargetName(AActor* Actor) const
{
	return Actor ? FString::Printf(TEXT("%s (%s)"), *Actor->GetActorNameOrLabel(), *Actor->GetPathName()) : TEXT("None");
}

bool FRevoltEditorBridgeEditorModule::IsGeneratedActor(AActor* Actor) const
{
	return Actor && Actor->Tags.Contains(FName(TEXT("Revolt.Generated")));
}

bool FRevoltEditorBridgeEditorModule::IsGeneratedAssetPath(const FString& AssetPath) const
{
	return AssetPath == TEXT("/Game/RevoltGenerated") || AssetPath.StartsWith(TEXT("/Game/RevoltGenerated/"));
}

bool FRevoltEditorBridgeEditorModule::NormalizeGeneratedPackagePath(const FString& InputPath, FString& OutPackagePath, FString& ErrorCode, FString& ErrorMessage) const
{
	OutPackagePath = InputPath;
	OutPackagePath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (OutPackagePath.EndsWith(TEXT("/")))
	{
		OutPackagePath.LeftChopInline(1);
	}

	if (OutPackagePath.IsEmpty())
	{
		OutPackagePath = TEXT("/Game/RevoltGenerated");
	}

	if (!IsGeneratedAssetPath(OutPackagePath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Generated asset commands may only write under /Game/RevoltGenerated.");
		return false;
	}

	if (!FPackageName::IsValidLongPackageName(OutPackagePath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = FString::Printf(TEXT("Package path '%s' is not valid."), *OutPackagePath);
		return false;
	}

	return true;
}

bool FRevoltEditorBridgeEditorModule::NormalizeGeneratedAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	FString AssetPath;
	if (Params->TryGetStringField(TEXT("asset_path"), AssetPath) || Params->TryGetStringField(TEXT("asset"), AssetPath))
	{
		AssetPath.ReplaceInline(TEXT("\\"), TEXT("/"));
		const int32 DotIndex = AssetPath.Find(TEXT("."));
		if (DotIndex != INDEX_NONE)
		{
			AssetPath.LeftInline(DotIndex);
		}

		int32 SlashIndex = INDEX_NONE;
		if (!AssetPath.FindLastChar('/', SlashIndex) || SlashIndex <= 0)
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = TEXT("asset_path must include a package path and asset name.");
			return false;
		}

		OutPackagePath = AssetPath.Left(SlashIndex);
		OutAssetName = AssetPath.Mid(SlashIndex + 1);
	}
	else
	{
		if (!Params->TryGetStringField(TEXT("asset_name"), OutAssetName) && !Params->TryGetStringField(TEXT("name"), OutAssetName))
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = TEXT("Asset command requires asset_name or asset_path.");
			return false;
		}

		if (!Params->TryGetStringField(TEXT("path"), OutPackagePath) && !Params->TryGetStringField(TEXT("package_path"), OutPackagePath))
		{
			OutPackagePath = TEXT("/Game/RevoltGenerated");
		}
	}

	if (!NormalizeGeneratedPackagePath(OutPackagePath, OutPackagePath, ErrorCode, ErrorMessage))
	{
		return false;
	}

	const FString CandidateObjectPath = FString::Printf(TEXT("%s/%s.%s"), *OutPackagePath, *OutAssetName, *OutAssetName);
	if (OutAssetName.IsEmpty() || !FPackageName::IsValidObjectPath(CandidateObjectPath))
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = FString::Printf(TEXT("Asset name '%s' is not valid."), *OutAssetName);
		return false;
	}

	OutAssetPath = FString::Printf(TEXT("%s/%s"), *OutPackagePath, *OutAssetName);
	return true;
}

bool FRevoltEditorBridgeEditorModule::EnsureGeneratedFolderExists(const FString& PackagePath, bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsGeneratedAssetPath(PackagePath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Generated folders must be under /Game/RevoltGenerated.");
		return false;
	}

	if (bDryRun)
	{
		return true;
	}

	const FString FolderFilename = FPackageName::LongPackageNameToFilename(PackagePath);
	if (!IFileManager::Get().MakeDirectory(*FolderFilename, true))
	{
		ErrorCode = TEXT("FOLDER_CREATE_FAILED");
		ErrorMessage = FString::Printf(TEXT("Failed to create generated folder '%s'."), *PackagePath);
		return false;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FString> PathsToScan;
	PathsToScan.Add(PackagePath);
	AssetRegistryModule.Get().ScanPathsSynchronous(PathsToScan, true);
	return true;
}

bool FRevoltEditorBridgeEditorModule::IsGeneratedBiomeAssetPath(const FString& AssetPath) const
{
	FString NormalizedPath = AssetPath;
	NormalizedPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (NormalizedPath.Contains(TEXT(".")))
	{
		NormalizedPath = FPackageName::ObjectPathToPackageName(NormalizedPath);
	}
	return NormalizedPath == TEXT("/Game/RevoltGenerated/Biomes") || NormalizedPath.StartsWith(TEXT("/Game/RevoltGenerated/Biomes/"));
}

bool FRevoltEditorBridgeEditorModule::NormalizeGeneratedBiomeAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!NormalizeGeneratedAssetPath(Params, OutPackagePath, OutAssetName, OutAssetPath, ErrorCode, ErrorMessage))
	{
		return false;
	}

	if (!Params->HasField(TEXT("path")) && !Params->HasField(TEXT("package_path")) && !Params->HasField(TEXT("asset_path")) && !Params->HasField(TEXT("asset")))
	{
		OutPackagePath = TEXT("/Game/RevoltGenerated/Biomes");
		OutAssetPath = FString::Printf(TEXT("%s/%s"), *OutPackagePath, *OutAssetName);
	}

	if (!IsGeneratedBiomeAssetPath(OutPackagePath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Generated biome assets must be under /Game/RevoltGenerated/Biomes.");
		return false;
	}

	return true;
}

URevoltBiomeDataAsset* FRevoltEditorBridgeEditorModule::LoadBiomeAsset(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsGeneratedBiomeAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Biome asset commands may only read generated biome assets under /Game/RevoltGenerated/Biomes.");
		return nullptr;
	}

	return Cast<URevoltBiomeDataAsset>(LoadAssetForMutation(AssetPath, URevoltBiomeDataAsset::StaticClass(), ErrorCode, ErrorMessage));
}

bool FRevoltEditorBridgeEditorModule::ReadBiomeJsonFromParams(const TSharedPtr<FJsonObject>& Params, TSharedPtr<FJsonObject>& OutBiomeJson, FString& ErrorCode, FString& ErrorMessage) const
{
	const TSharedPtr<FJsonObject>* BiomeObject = nullptr;
	if (Params.IsValid() && Params->TryGetObjectField(TEXT("biome"), BiomeObject) && BiomeObject && BiomeObject->IsValid())
	{
		OutBiomeJson = *BiomeObject;
		return true;
	}

	FString JsonString;
	if (Params.IsValid() && (Params->TryGetStringField(TEXT("json"), JsonString) || Params->TryGetStringField(TEXT("biome_json"), JsonString)))
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, OutBiomeJson) && OutBiomeJson.IsValid())
		{
			return true;
		}

		ErrorCode = TEXT("INVALID_JSON");
		ErrorMessage = TEXT("Biome JSON string could not be parsed.");
		return false;
	}

	ErrorCode = TEXT("INVALID_PARAMETERS");
	ErrorMessage = TEXT("Biome commands require a biome object or json string.");
	return false;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::MakeBiomeValidationResult(const URevoltBiomeDataAsset* BiomeAsset) const
{
	TArray<FString> Errors;
	if (BiomeAsset)
	{
		BiomeAsset->ValidateBiome(Errors);
	}
	else
	{
		Errors.Add(TEXT("Biome asset is null."));
	}

	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("valid"), Errors.IsEmpty());
	Result->SetArrayField(TEXT("errors"), ErrorValues);
	Result->SetNumberField(TEXT("error_count"), Errors.Num());
	return Result;
}

UClass* FRevoltEditorBridgeEditorModule::ResolveApprovedDataAssetClass(const FString& ClassName, FString& ErrorCode, FString& ErrorMessage) const
{
	UClass* DataAssetClass = LoadClass<UDataAsset>(nullptr, *ClassName);
	if (!DataAssetClass)
	{
		DataAssetClass = FindObject<UClass>(nullptr, *ClassName);
	}

	if (!DataAssetClass)
	{
		ErrorCode = TEXT("ASSET_CLASS_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Data asset class '%s' was not found."), *ClassName);
		return nullptr;
	}

	if (!DataAssetClass->IsChildOf(UDataAsset::StaticClass()) ||
		DataAssetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
	{
		ErrorCode = TEXT("ASSET_CLASS_NOT_ALLOWED");
		ErrorMessage = TEXT("create_data_asset only supports approved non-abstract UDataAsset classes.");
		return nullptr;
	}

	if (DataAssetClass != UDataAsset::StaticClass() && DataAssetClass->GetPathName() != TEXT("/Script/Engine.PrimaryDataAsset"))
	{
		ErrorCode = TEXT("ASSET_CLASS_NOT_APPROVED");
		ErrorMessage = TEXT("Phase 4 approved data asset classes are /Script/Engine.DataAsset and /Script/Engine.PrimaryDataAsset.");
		return nullptr;
	}

	return DataAssetClass;
}

UObject* FRevoltEditorBridgeEditorModule::LoadAssetForMutation(const FString& AssetPath, UClass* ExpectedClass, FString& ErrorCode, FString& ErrorMessage) const
{
	UObject* Asset = StaticLoadObject(ExpectedClass, nullptr, *AssetPath);
	if (!Asset)
	{
		ErrorCode = TEXT("ASSET_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Asset '%s' was not found."), *AssetPath);
		return nullptr;
	}

	return Asset;
}

bool FRevoltEditorBridgeEditorModule::ApplyEditablePropertyValue(UObject* Object, const FString& PropertyName, const TSharedPtr<FJsonValue>& Value, FString& BeforeValue, FString& AfterValue, bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Object)
	{
		ErrorCode = TEXT("ASSET_NOT_FOUND");
		ErrorMessage = TEXT("Object is unavailable.");
		return false;
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
	if (!Property)
	{
		ErrorCode = TEXT("PROPERTY_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Property '%s' was not found on '%s'."), *PropertyName, *Object->GetName());
		return false;
	}

	if (!Property->HasAnyPropertyFlags(CPF_Edit))
	{
		ErrorCode = TEXT("PROPERTY_NOT_EDITABLE");
		ErrorMessage = FString::Printf(TEXT("Property '%s' is not editable."), *PropertyName);
		return false;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
	Property->ExportTextItem_Direct(BeforeValue, ValuePtr, ValuePtr, Object, PPF_None);

	if (Value->Type == EJson::String)
	{
		AfterValue = Value->AsString();
	}
	else if (Value->Type == EJson::Number)
	{
		AfterValue = FString::SanitizeFloat(Value->AsNumber());
	}
	else if (Value->Type == EJson::Boolean)
	{
		AfterValue = Value->AsBool() ? TEXT("true") : TEXT("false");
	}
	else
	{
		ErrorCode = TEXT("INVALID_PARAMETERS");
		ErrorMessage = TEXT("Bulk data asset edits support string, number, and boolean values in Phase 4.");
		return false;
	}

	if (!bDryRun)
	{
		Object->Modify();
		if (!Property->ImportText_Direct(*AfterValue, ValuePtr, Object, PPF_None))
		{
			ErrorCode = TEXT("INVALID_PARAMETERS");
			ErrorMessage = FString::Printf(TEXT("Value could not be imported for property '%s'."), *PropertyName);
			return false;
		}
		Object->PostEditChange();
		Object->MarkPackageDirty();
	}

	return true;
}

bool FRevoltEditorBridgeEditorModule::SavePackageForAsset(UObject* Asset, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Asset || !Asset->GetPackage())
	{
		ErrorCode = TEXT("ASSET_NOT_FOUND");
		ErrorMessage = TEXT("Asset package is unavailable.");
		return false;
	}

	UPackage* Package = Asset->GetPackage();
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs))
	{
		ErrorCode = TEXT("SAVE_FAILED");
		ErrorMessage = FString::Printf(TEXT("Failed to save package '%s'."), *Package->GetName());
		return false;
	}

	return true;
}

bool FRevoltEditorBridgeEditorModule::NormalizeGeneratedBlueprintAssetPath(const TSharedPtr<FJsonObject>& Params, FString& OutPackagePath, FString& OutAssetName, FString& OutAssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!NormalizeGeneratedAssetPath(Params, OutPackagePath, OutAssetName, OutAssetPath, ErrorCode, ErrorMessage))
	{
		return false;
	}

	if (!IsGeneratedAssetPath(OutPackagePath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Generated Blueprints must be created under /Game/RevoltGenerated.");
		return false;
	}

	return true;
}

UClass* FRevoltEditorBridgeEditorModule::ResolveSafeBlueprintParentClass(const FString& ParentClassName, FString& ErrorCode, FString& ErrorMessage) const
{
	const FString NormalizedName = ParentClassName.TrimStartAndEnd();
	UClass* ParentClass = nullptr;

	if (NormalizedName == TEXT("Actor"))
	{
		ParentClass = AActor::StaticClass();
	}
	else if (NormalizedName == TEXT("Pawn"))
	{
		ParentClass = APawn::StaticClass();
	}
	else if (NormalizedName == TEXT("Character"))
	{
		ParentClass = ACharacter::StaticClass();
	}
	else if (NormalizedName == TEXT("ActorComponent"))
	{
		ParentClass = UActorComponent::StaticClass();
	}
	else if (NormalizedName == TEXT("SceneComponent"))
	{
		ParentClass = USceneComponent::StaticClass();
	}
	else if (NormalizedName == TEXT("UserWidget"))
	{
		ParentClass = UUserWidget::StaticClass();
	}
	else
	{
		ParentClass = LoadClass<UObject>(nullptr, *NormalizedName);
	}

	if (!ParentClass)
	{
		ErrorCode = TEXT("BLUEPRINT_PARENT_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Blueprint parent class '%s' was not found."), *ParentClassName);
		return nullptr;
	}

	const bool bAllowedParent =
		ParentClass == AActor::StaticClass() ||
		ParentClass == APawn::StaticClass() ||
		ParentClass == ACharacter::StaticClass() ||
		ParentClass == UActorComponent::StaticClass() ||
		ParentClass == USceneComponent::StaticClass() ||
		ParentClass == UUserWidget::StaticClass();

	if (!bAllowedParent)
	{
		ErrorCode = TEXT("BLUEPRINT_PARENT_NOT_ALLOWED");
		ErrorMessage = TEXT("Allowed Blueprint parents are Actor, Pawn, Character, ActorComponent, SceneComponent, and UserWidget.");
		return nullptr;
	}

	return ParentClass;
}

UClass* FRevoltEditorBridgeEditorModule::ResolveAllowedBlueprintComponentClass(const FString& ComponentClassName, FString& ErrorCode, FString& ErrorMessage) const
{
	UClass* ComponentClass = nullptr;
	const FString NormalizedName = ComponentClassName.TrimStartAndEnd();
	if (NormalizedName == TEXT("StaticMeshComponent"))
	{
		ComponentClass = UStaticMeshComponent::StaticClass();
	}
	else if (NormalizedName == TEXT("SceneComponent"))
	{
		ComponentClass = USceneComponent::StaticClass();
	}
	else
	{
		ComponentClass = LoadClass<UActorComponent>(nullptr, *NormalizedName);
	}

	if (!ComponentClass)
	{
		ErrorCode = TEXT("COMPONENT_CLASS_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Component class '%s' was not found."), *ComponentClassName);
		return nullptr;
	}

	if (!ComponentClass->IsChildOf(UActorComponent::StaticClass()) ||
		ComponentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated) ||
		!(ComponentClass == UStaticMeshComponent::StaticClass() || ComponentClass == USceneComponent::StaticClass()))
	{
		ErrorCode = TEXT("COMPONENT_CLASS_NOT_ALLOWED");
		ErrorMessage = TEXT("Phase 5 allows StaticMeshComponent and SceneComponent only.");
		return nullptr;
	}

	return ComponentClass;
}

UBlueprint* FRevoltEditorBridgeEditorModule::LoadBlueprintForMutation(const FString& BlueprintPath, bool bAllowUserBlueprintEdit, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsGeneratedAssetPath(BlueprintPath) && !bAllowUserBlueprintEdit)
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Blueprint edits are limited to /Game/RevoltGenerated unless allow_user_blueprint_edit is explicitly true and approved.");
		return nullptr;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *BlueprintPath));
	if (!Blueprint)
	{
		ErrorCode = TEXT("BLUEPRINT_NOT_FOUND");
		ErrorMessage = FString::Printf(TEXT("Blueprint '%s' was not found."), *BlueprintPath);
		return nullptr;
	}

	return Blueprint;
}

bool FRevoltEditorBridgeEditorModule::IsValidBlueprintVariableName(const FString& VariableName, FString& ErrorCode, FString& ErrorMessage) const
{
	if (VariableName.IsEmpty())
	{
		ErrorCode = TEXT("INVALID_BLUEPRINT_VARIABLE_NAME");
		ErrorMessage = FString::Printf(TEXT("Blueprint variable or component name '%s' is not valid."), *VariableName);
		return false;
	}

	const FString InvalidCharacters = FString(INVALID_OBJECTNAME_CHARACTERS) + TEXT(" ");
	for (const TCHAR Character : VariableName)
	{
		if (InvalidCharacters.Contains(FString::Chr(Character)))
		{
			ErrorCode = TEXT("INVALID_BLUEPRINT_VARIABLE_NAME");
			ErrorMessage = FString::Printf(TEXT("Blueprint variable or component name '%s' contains invalid characters."), *VariableName);
			return false;
		}
	}

	return true;
}

bool FRevoltEditorBridgeEditorModule::MakeSupportedBlueprintPinType(const FString& VariableType, FEdGraphPinType& OutPinType, FString& ErrorCode, FString& ErrorMessage) const
{
	const FString NormalizedType = VariableType.TrimStartAndEnd().ToLower();
	if (NormalizedType == TEXT("bool") || NormalizedType == TEXT("boolean"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (NormalizedType == TEXT("int") || NormalizedType == TEXT("integer") || NormalizedType == TEXT("teamid"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (NormalizedType == TEXT("float"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Float;
	}
	else if (NormalizedType == TEXT("double"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (NormalizedType == TEXT("string"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (NormalizedType == TEXT("name"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else
	{
		ErrorCode = TEXT("BLUEPRINT_VARIABLE_TYPE_NOT_SUPPORTED");
		ErrorMessage = TEXT("Supported Blueprint variable types are bool, int, float, double, string, and name.");
		return false;
	}

	return true;
}

FString FRevoltEditorBridgeEditorModule::GetBlueprintCompileStatusString(const UBlueprint* Blueprint) const
{
	if (!Blueprint)
	{
		return TEXT("unknown");
	}

	switch (Blueprint->Status)
	{
	case BS_Dirty:
		return TEXT("dirty");
	case BS_Error:
		return TEXT("error");
	case BS_UpToDate:
		return TEXT("success");
	case BS_UpToDateWithWarnings:
		return TEXT("success_with_warnings");
	case BS_BeingCreated:
		return TEXT("being_created");
	case BS_Unknown:
	default:
		return TEXT("unknown");
	}
}

void FRevoltEditorBridgeEditorModule::AddAuditIssue(TArray<TSharedPtr<FJsonValue>>& Issues, const FString& Source, const FString& Severity, const FString& Code, const FString& AffectedType, const FString& Affected, const FString& Message, const FString& RecommendedFix, int32& IssueCounter) const
{
	const FString SeverityDisplay = Severity.Equals(TEXT("error"), ESearchCase::IgnoreCase) ? TEXT("Error")
		: Severity.Equals(TEXT("warning"), ESearchCase::IgnoreCase) ? TEXT("Warning")
		: TEXT("Info");
	const FString Category = Source.Equals(TEXT("current_level"), ESearchCase::IgnoreCase) ? TEXT("Level")
		: Source.Equals(TEXT("selected_actors"), ESearchCase::IgnoreCase) ? TEXT("Actor")
		: Source.Equals(TEXT("blueprints"), ESearchCase::IgnoreCase) ? TEXT("Blueprint")
		: Source.Equals(TEXT("assets"), ESearchCase::IgnoreCase) ? TEXT("Asset")
		: Source.Equals(TEXT("generated_content"), ESearchCase::IgnoreCase) ? TEXT("GeneratedContent")
		: Source;
	const FString TargetType = AffectedType.Equals(TEXT("level"), ESearchCase::IgnoreCase) ? TEXT("Map")
		: AffectedType.Equals(TEXT("actor"), ESearchCase::IgnoreCase) ? TEXT("Actor")
		: AffectedType.Equals(TEXT("asset"), ESearchCase::IgnoreCase) ? TEXT("Asset")
		: AffectedType.Equals(TEXT("selection"), ESearchCase::IgnoreCase) ? TEXT("Selection")
		: AffectedType;
	const FString IssueId = Code.Equals(TEXT("map_without_player_start"), ESearchCase::IgnoreCase) ? TEXT("LEVEL_NO_PLAYER_START")
		: Code.ToUpper();

	bool bAutoFixAvailable = false;
	FString RequiredPermission = TEXT("ManualReview");
	FString SuggestedCommand;
	TSharedPtr<FJsonObject> SuggestedParams = MakeShared<FJsonObject>();
	if (Code == TEXT("map_without_player_start"))
	{
		bAutoFixAvailable = true;
		RequiredPermission = TEXT("SafeEdit");
		SuggestedCommand = TEXT("spawn_actor");
		SuggestedParams->SetStringField(TEXT("class"), TEXT("PlayerStart"));
		TArray<TSharedPtr<FJsonValue>> LocationValues;
		LocationValues.Add(MakeShared<FJsonValueNumber>(0.0));
		LocationValues.Add(MakeShared<FJsonValueNumber>(0.0));
		LocationValues.Add(MakeShared<FJsonValueNumber>(120.0));
		SuggestedParams->SetArrayField(TEXT("location"), LocationValues);
	}
	else if (Code == TEXT("blueprint_compile_error") || Code == TEXT("blueprint_compile_status_uncertain"))
	{
		bAutoFixAvailable = true;
		RequiredPermission = TEXT("ProjectEdit");
		SuggestedCommand = TEXT("compile_blueprint");
		SuggestedParams->SetStringField(TEXT("blueprint"), Affected);
	}
	else if (Code == TEXT("actor_missing_static_mesh") || Code == TEXT("empty_collision_on_important_actor") || Code == TEXT("extreme_light_intensity"))
	{
		bAutoFixAvailable = true;
		RequiredPermission = TEXT("SafeEdit");
		SuggestedCommand = TEXT("set_actor_property");
		SuggestedParams->SetStringField(TEXT("actor"), Affected);
	}
	else if (Code == TEXT("material_missing_parent"))
	{
		bAutoFixAvailable = true;
		RequiredPermission = TEXT("ProjectEdit");
		SuggestedCommand = TEXT("create_material_instance");
		SuggestedParams->SetStringField(TEXT("asset"), Affected);
	}

	TSharedPtr<FJsonObject> Issue = MakeShared<FJsonObject>();
	Issue->SetStringField(TEXT("id"), FString::Printf(TEXT("%s-%03d"), *Source.ToUpper(), IssueCounter++));
	Issue->SetStringField(TEXT("issueId"), IssueId);
	Issue->SetStringField(TEXT("source"), Source);
	Issue->SetStringField(TEXT("severity"), Severity);
	Issue->SetStringField(TEXT("severityDisplay"), SeverityDisplay);
	Issue->SetStringField(TEXT("category"), Category);
	Issue->SetStringField(TEXT("code"), Code);
	Issue->SetStringField(TEXT("affected_type"), AffectedType);
	Issue->SetStringField(TEXT("targetType"), TargetType);
	Issue->SetStringField(TEXT("affected"), Affected);
	Issue->SetStringField(TEXT("targetPath"), Affected);
	Issue->SetStringField(TEXT("message"), Message);
	Issue->SetNumberField(TEXT("confidence"), 0.95);
	Issue->SetStringField(TEXT("recommended_fix"), RecommendedFix);
	Issue->SetStringField(TEXT("recommendedFix"), RecommendedFix);
	Issue->SetBoolField(TEXT("autoFixAvailable"), bAutoFixAvailable);
	Issue->SetStringField(TEXT("requiredPermission"), RequiredPermission);
	Issue->SetStringField(TEXT("suggestedCommand"), SuggestedCommand);
	Issue->SetObjectField(TEXT("suggestedParams"), SuggestedParams);
	Issues.Add(MakeShared<FJsonValueObject>(Issue));
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::MakeFixPlanItemFromIssue(const TSharedPtr<FJsonObject>& Issue) const
{
	FString IssueId;
	FString Severity;
	FString Code;
	FString Affected;
	FString RecommendedFix;
	Issue->TryGetStringField(TEXT("id"), IssueId);
	Issue->TryGetStringField(TEXT("issueId"), IssueId);
	Issue->TryGetStringField(TEXT("severity"), Severity);
	Issue->TryGetStringField(TEXT("severityDisplay"), Severity);
	Issue->TryGetStringField(TEXT("code"), Code);
	Issue->TryGetStringField(TEXT("affected"), Affected);
	Issue->TryGetStringField(TEXT("targetPath"), Affected);
	Issue->TryGetStringField(TEXT("recommended_fix"), RecommendedFix);
	Issue->TryGetStringField(TEXT("recommendedFix"), RecommendedFix);

	bool bCanBeAutomated = false;
	FString RequiredPermissionLevel = TEXT("manual_review");
	FString SuggestedBridgeCommand = TEXT("");

	if (Code == TEXT("blueprint_compile_error") || Code == TEXT("blueprint_compile_status_uncertain"))
	{
		bCanBeAutomated = true;
		RequiredPermissionLevel = TEXT("ProjectEdit");
		SuggestedBridgeCommand = TEXT("compile_blueprint");
	}
	else if (Code == TEXT("actor_missing_static_mesh"))
	{
		bCanBeAutomated = true;
		RequiredPermissionLevel = TEXT("SafeEdit");
		SuggestedBridgeCommand = TEXT("set_actor_property");
	}
	else if (Code == TEXT("map_without_player_start"))
	{
		bCanBeAutomated = true;
		RequiredPermissionLevel = TEXT("SafeEdit");
		SuggestedBridgeCommand = TEXT("spawn_actor");
	}
	else if (Code == TEXT("material_missing_parent"))
	{
		bCanBeAutomated = true;
		RequiredPermissionLevel = TEXT("ProjectEdit");
		SuggestedBridgeCommand = TEXT("create_material_instance");
	}
	else if (Code == TEXT("generated_actor_missing_tag") || Code == TEXT("generated_asset_outside_generated_root"))
	{
		RequiredPermissionLevel = TEXT("ManualReview");
	}

	TSharedPtr<FJsonObject> PlanItem = MakeShared<FJsonObject>();
	PlanItem->SetStringField(TEXT("issue_id"), IssueId);
	PlanItem->SetStringField(TEXT("issueId"), IssueId);
	PlanItem->SetStringField(TEXT("severity"), Severity);
	PlanItem->SetStringField(TEXT("affected_actor_asset"), Affected);
	PlanItem->SetStringField(TEXT("affectedTarget"), Affected);
	PlanItem->SetStringField(TEXT("recommended_fix"), RecommendedFix);
	PlanItem->SetStringField(TEXT("recommendedFix"), RecommendedFix);
	PlanItem->SetBoolField(TEXT("can_be_automated"), bCanBeAutomated);
	PlanItem->SetBoolField(TEXT("autoFixAvailable"), bCanBeAutomated);
	PlanItem->SetStringField(TEXT("required_permission_level"), RequiredPermissionLevel);
	PlanItem->SetStringField(TEXT("requiredPermission"), RequiredPermissionLevel);
	PlanItem->SetStringField(TEXT("suggested_bridge_command"), SuggestedBridgeCommand);
	PlanItem->SetStringField(TEXT("suggestedCommand"), SuggestedBridgeCommand);
	PlanItem->SetBoolField(TEXT("executed"), false);
	return PlanItem;
}

void FRevoltEditorBridgeEditorModule::AddActorAuditIssues(AActor* Actor, TArray<TSharedPtr<FJsonValue>>& Issues, const FString& Source, int32& IssueCounter) const
{
	if (!Actor)
	{
		return;
	}

	const bool bHasGeneratedTag = Actor->Tags.Contains(TEXT("Revolt.Generated"));
	bool bHasBiomeTag = false;
	for (const FName& Tag : Actor->Tags)
	{
		if (Tag.ToString().StartsWith(TEXT("Revolt.Biome.")))
		{
			bHasBiomeTag = true;
			break;
		}
	}
	if (bHasBiomeTag && !bHasGeneratedTag)
	{
		AddAuditIssue(Issues, Source, TEXT("warning"), TEXT("generated_actor_missing_tag"), TEXT("actor"), Actor->GetPathName(), TEXT("Actor has a Revolt biome tag but is missing Revolt.Generated."), TEXT("Review whether this actor is generated content before adding tags or baking."), IssueCounter);
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (!StaticMeshComponent)
		{
			continue;
		}

		if (!StaticMeshComponent->GetStaticMesh())
		{
			AddAuditIssue(Issues, Source, TEXT("warning"), TEXT("actor_missing_static_mesh"), TEXT("actor"), Actor->GetPathName(), TEXT("Actor has a StaticMeshComponent with no Static Mesh assigned."), TEXT("Assign a mesh or remove the empty StaticMeshComponent."), IssueCounter);
		}
		else if (StaticMeshComponent->GetCollisionEnabled() == ECollisionEnabled::NoCollision && (Actor->ActorHasTag(TEXT("Revolt.Generated")) || Actor->GetClass()->GetName().Contains(TEXT("StaticMesh"))))
		{
			AddAuditIssue(Issues, Source, TEXT("info"), TEXT("empty_collision_on_important_actor"), TEXT("actor"), Actor->GetPathName(), TEXT("Static mesh actor has collision disabled."), TEXT("Enable collision if this actor should block players, traces, or AI."), IssueCounter);
		}
	}

	TArray<ULightComponent*> LightComponents;
	Actor->GetComponents<ULightComponent>(LightComponents);
	for (ULightComponent* LightComponent : LightComponents)
	{
		if (LightComponent && LightComponent->Intensity > 100000.0f)
		{
			AddAuditIssue(Issues, Source, TEXT("warning"), TEXT("extreme_light_intensity"), TEXT("actor"), Actor->GetPathName(), TEXT("Light intensity is unusually high."), TEXT("Review intensity units and lower the value if lighting or exposure is unstable."), IssueCounter);
		}
	}

	TArray<UActorComponent*> Components;
	Actor->GetComponents<UActorComponent>(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetClass()->GetName().Contains(TEXT("Niagara"), ESearchCase::IgnoreCase))
		{
			FNumericProperty* MaxParticlesProperty = CastField<FNumericProperty>(Component->GetClass()->FindPropertyByName(TEXT("MaxSimultaneousParticles")));
			if (MaxParticlesProperty && MaxParticlesProperty->GetFloatingPointPropertyValue(MaxParticlesProperty->ContainerPtrToValuePtr<void>(Component)) > 100000.0)
			{
				AddAuditIssue(Issues, Source, TEXT("warning"), TEXT("suspicious_niagara_spawn_rate"), TEXT("actor"), Actor->GetPathName(), TEXT("Niagara component reports an unusually high particle limit."), TEXT("Review Niagara spawn rates and particle limits for performance."), IssueCounter);
			}
		}
	}
}

bool FRevoltEditorBridgeEditorModule::EnsureArenaTemplateFolders(bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const
{
	const TArray<FString> Folders = {
		TEXT("/Game/RevoltGenerated/Templates"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Weapons"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Enemies"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Data/Waves"),
		TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/Maps")
	};

	for (const FString& Folder : Folders)
	{
		if (!EnsureGeneratedFolderExists(Folder, bDryRun, ErrorCode, ErrorMessage))
		{
			return false;
		}
	}
	return true;
}

UObject* FRevoltEditorBridgeEditorModule::CreateArenaDataAsset(const FString& PackagePath, const FString& AssetName, UClass* AssetClass, bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsArenaTemplateAssetPath(PackagePath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Arena shooter template assets must be under /Game/RevoltGenerated/Templates/ArenaShooter.");
		return nullptr;
	}

	const FString AssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
	UObject* ExistingAsset = StaticLoadObject(AssetClass, nullptr, *AssetPath);
	if (ExistingAsset || bDryRun)
	{
		return ExistingAsset;
	}

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = AssetClass;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, AssetClass, Factory);
	if (!NewAsset)
	{
		ErrorCode = TEXT("ASSET_CREATE_FAILED");
		ErrorMessage = FString::Printf(TEXT("Failed to create arena shooter asset '%s'."), *AssetPath);
		return nullptr;
	}

	NewAsset->MarkPackageDirty();
	return NewAsset;
}

URevoltArenaWeaponData* FRevoltEditorBridgeEditorModule::LoadArenaWeaponData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsArenaTemplateAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Arena weapon data may only be configured under /Game/RevoltGenerated/Templates/ArenaShooter.");
		return nullptr;
	}
	return Cast<URevoltArenaWeaponData>(LoadAssetForMutation(AssetPath, URevoltArenaWeaponData::StaticClass(), ErrorCode, ErrorMessage));
}

URevoltArenaEnemyData* FRevoltEditorBridgeEditorModule::LoadArenaEnemyData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsArenaTemplateAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Arena enemy data may only be configured under /Game/RevoltGenerated/Templates/ArenaShooter.");
		return nullptr;
	}
	return Cast<URevoltArenaEnemyData>(LoadAssetForMutation(AssetPath, URevoltArenaEnemyData::StaticClass(), ErrorCode, ErrorMessage));
}

bool FRevoltEditorBridgeEditorModule::IsArenaTemplateAssetPath(const FString& AssetPath) const
{
	FString NormalizedPath = AssetPath;
	NormalizedPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (NormalizedPath.Contains(TEXT(".")))
	{
		NormalizedPath = FPackageName::ObjectPathToPackageName(NormalizedPath);
	}
	return NormalizedPath == TEXT("/Game/RevoltGenerated/Templates/ArenaShooter") || NormalizedPath.StartsWith(TEXT("/Game/RevoltGenerated/Templates/ArenaShooter/"));
}

bool FRevoltEditorBridgeEditorModule::EnsureZombieTemplateFolders(bool bDryRun, FString& ErrorCode, FString& ErrorMessage) const
{
	const TArray<FString> Folders = {
		TEXT("/Game/RevoltGenerated/Templates"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Weapons"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Zombies"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Waves"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Data/Objectives"),
		TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/Maps")
	};

	for (const FString& Folder : Folders)
	{
		if (!EnsureGeneratedFolderExists(Folder, bDryRun, ErrorCode, ErrorMessage))
		{
			return false;
		}
	}
	return true;
}

bool FRevoltEditorBridgeEditorModule::IsZombieTemplateAssetPath(const FString& AssetPath) const
{
	FString NormalizedPath = AssetPath;
	NormalizedPath.ReplaceInline(TEXT("\\"), TEXT("/"));
	if (NormalizedPath.Contains(TEXT(".")))
	{
		NormalizedPath = FPackageName::ObjectPathToPackageName(NormalizedPath);
	}
	return NormalizedPath == TEXT("/Game/RevoltGenerated/Templates/ZombieShooter") || NormalizedPath.StartsWith(TEXT("/Game/RevoltGenerated/Templates/ZombieShooter/"));
}

URevoltZombieEnemyData* FRevoltEditorBridgeEditorModule::LoadZombieEnemyData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsZombieTemplateAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Zombie enemy data may only be configured under /Game/RevoltGenerated/Templates/ZombieShooter.");
		return nullptr;
	}
	return Cast<URevoltZombieEnemyData>(LoadAssetForMutation(AssetPath, URevoltZombieEnemyData::StaticClass(), ErrorCode, ErrorMessage));
}

URevoltZombieWaveData* FRevoltEditorBridgeEditorModule::LoadZombieWaveData(const FString& AssetPath, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!IsZombieTemplateAssetPath(AssetPath))
	{
		ErrorCode = TEXT("PATH_OUTSIDE_GENERATED");
		ErrorMessage = TEXT("Zombie wave data may only be configured under /Game/RevoltGenerated/Templates/ZombieShooter.");
		return nullptr;
	}
	return Cast<URevoltZombieWaveData>(LoadAssetForMutation(AssetPath, URevoltZombieWaveData::StaticClass(), ErrorCode, ErrorMessage));
}

bool FRevoltEditorBridgeEditorModule::CompileBlueprintAndValidate(UBlueprint* Blueprint, FString& ErrorCode, FString& ErrorMessage) const
{
	if (!Blueprint)
	{
		ErrorCode = TEXT("BLUEPRINT_NOT_FOUND");
		ErrorMessage = TEXT("Blueprint is unavailable.");
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error)
	{
		ErrorCode = TEXT("BLUEPRINT_COMPILE_FAILED");
		ErrorMessage = TEXT("Blueprint failed to compile after modification.");
		return false;
	}

	return true;
}

TArray<TSharedPtr<FJsonObject>> FRevoltEditorBridgeEditorModule::BuildDryRunDiff(const FString& RequestId, const FString& Command, const TSharedPtr<FJsonObject>& Params, const TSharedPtr<FJsonObject>& Preview, const FString& Target, FString& OutRiskLevel) const
{
	TArray<TSharedPtr<FJsonObject>> Diff;
	if (!Preview.IsValid())
	{
		OutRiskLevel = TEXT("Dangerous");
		return Diff;
	}

	const FString PermissionLevel = InferPermissionLevel(Params);
	const FString TargetPath = Target.IsEmpty() ? Preview->GetStringField(TEXT("target")) : Target;
	FString TargetType = TEXT("Generated asset");
	FString ChangeType = TEXT("Command");
	FString Property = TEXT("Preview");

	if (Command == TEXT("set_actor_transform"))
	{
		TargetType = TEXT("Actor transform");
		ChangeType = TEXT("Transform");
		Property = TEXT("Transform");
	}
	else if (Command == TEXT("set_actor_property"))
	{
		TargetType = TEXT("Actor property");
		ChangeType = TEXT("Property");
		Params->TryGetStringField(TEXT("property"), Property);
	}
	else if (Command == TEXT("spawn_actor") || Command == TEXT("duplicate_selected_actors") || Command.StartsWith(TEXT("landgen_")))
	{
		TargetType = TEXT("Actor");
		ChangeType = Command.Contains(TEXT("clear")) ? TEXT("Delete generated") : TEXT("Generated actor");
	}
	else if (Command == TEXT("set_material_instance_parameter") || Command == TEXT("create_material_instance"))
	{
		TargetType = TEXT("Material Instance parameter");
		ChangeType = Command == TEXT("create_material_instance") ? TEXT("Create asset") : TEXT("Parameter");
		Params->TryGetStringField(TEXT("parameter"), Property);
		Params->TryGetStringField(TEXT("parameter_name"), Property);
	}
	else if (Command.Contains(TEXT("blueprint")))
	{
		TargetType = TEXT("Blueprint variable/default");
		ChangeType = TEXT("Blueprint");
		Params->TryGetStringField(TEXT("variable_name"), Property);
	}
	else if (Command.Contains(TEXT("biome")))
	{
		TargetType = TEXT("Generated biome asset");
		ChangeType = Command.StartsWith(TEXT("create")) ? TEXT("Create asset") : TEXT("Data Asset field");
		Property = TEXT("Biome JSON");
	}
	else if (Command.Contains(TEXT("data_asset")) || Command.Contains(TEXT("weapon_data")) || Command.Contains(TEXT("enemy_data")) || Command.Contains(TEXT("wave_data")))
	{
		TargetType = TEXT("Data Asset field");
		ChangeType = TEXT("Property");
	}

	const TArray<TSharedPtr<FJsonValue>>* AssetPreviewValues = nullptr;
	if (Preview->TryGetArrayField(TEXT("preview"), AssetPreviewValues))
	{
		for (const TSharedPtr<FJsonValue>& AssetPreviewValue : *AssetPreviewValues)
		{
			const TSharedPtr<FJsonObject> AssetPreview = AssetPreviewValue->AsObject();
			if (!AssetPreview.IsValid())
			{
				continue;
			}

			const FString AssetPath = AssetPreview->GetStringField(TEXT("asset"));
			const TArray<TSharedPtr<FJsonValue>>* ChangeValues = nullptr;
			if (AssetPreview->TryGetArrayField(TEXT("changes"), ChangeValues))
			{
				for (const TSharedPtr<FJsonValue>& ChangeValue : *ChangeValues)
				{
					const TSharedPtr<FJsonObject> ChangeObject = ChangeValue->AsObject();
					if (!ChangeObject.IsValid())
					{
						continue;
					}

					FRevoltDryRunDiffItem Item;
					Item.CommandId = RequestId;
					Item.CommandName = Command;
					Item.TargetType = TargetType;
					Item.TargetPath = AssetPath;
					Item.ChangeType = TEXT("Property");
					Item.Property = ChangeObject->GetStringField(TEXT("property"));
					Item.OldValue = ChangeObject->GetStringField(TEXT("before"));
					Item.NewValue = ChangeObject->GetStringField(TEXT("after"));
					Item.PermissionLevel = PermissionLevel;
					Item.RiskLevel = InferRiskLevel(Command, AssetPath, AssetPreviewValues->Num());
					Diff.Add(MakeDiffItemObject(Item));
				}
			}
		}
	}

	if (Diff.IsEmpty())
	{
		FString OldValue = TEXT("No change applied");
		FString NewValue = JsonObjectToDisplayString(Preview);

		const TSharedPtr<FJsonObject>* BeforeObject = nullptr;
		const TSharedPtr<FJsonObject>* AfterObject = nullptr;
		if (Preview->TryGetObjectField(TEXT("before"), BeforeObject) && Preview->TryGetObjectField(TEXT("after"), AfterObject))
		{
			OldValue = JsonObjectToDisplayString(*BeforeObject);
			NewValue = JsonObjectToDisplayString(*AfterObject);
		}
		else
		{
			FString BeforeString;
			FString AfterString;
			if (Preview->TryGetStringField(TEXT("before"), BeforeString))
			{
				OldValue = BeforeString;
			}
			if (Preview->TryGetStringField(TEXT("after"), AfterString))
			{
				NewValue = AfterString;
			}
			else if (Preview->HasTypedField<EJson::Number>(TEXT("after")))
			{
				NewValue = FString::SanitizeFloat(Preview->GetNumberField(TEXT("after")));
			}
			else if (Command.StartsWith(TEXT("create_")) || Command.Contains(TEXT("_template")))
			{
				OldValue = TEXT("Does not exist");
				NewValue = TEXT("Will create generated content");
			}
			else if (Command.StartsWith(TEXT("save_")))
			{
				OldValue = TEXT("Dirty package state");
				NewValue = TEXT("Saved package state");
			}
			else if (Command.Contains(TEXT("clear")) || Command.Contains(TEXT("delete")))
			{
				OldValue = TEXT("Generated content present");
				NewValue = TEXT("Generated content removed only if tagged Revolt.Generated");
			}
		}

		FString ExplicitTargetPath = TargetPath;
		if (ExplicitTargetPath.IsEmpty())
		{
			Preview->TryGetStringField(TEXT("asset_path"), ExplicitTargetPath);
			Preview->TryGetStringField(TEXT("asset"), ExplicitTargetPath);
			Preview->TryGetStringField(TEXT("blueprint"), ExplicitTargetPath);
		}

		FRevoltDryRunDiffItem Item;
		Item.CommandId = RequestId;
		Item.CommandName = Command;
		Item.TargetType = TargetType;
		Item.TargetPath = ExplicitTargetPath.IsEmpty() ? TEXT("Unknown") : ExplicitTargetPath;
		Item.ChangeType = ChangeType;
		Item.Property = Property.IsEmpty() ? TEXT("Preview") : Property;
		Item.OldValue = OldValue;
		Item.NewValue = NewValue;
		Item.PermissionLevel = PermissionLevel;
		Item.RiskLevel = InferRiskLevel(Command, Item.TargetPath, 1);
		Diff.Add(MakeDiffItemObject(Item));
	}

	OutRiskLevel = TEXT("Low");
	for (const TSharedPtr<FJsonObject>& Item : Diff)
	{
		const FString ItemRisk = Item->GetStringField(TEXT("riskLevel"));
		if (ItemRisk == TEXT("Dangerous"))
		{
			OutRiskLevel = TEXT("Dangerous");
			break;
		}
		if (ItemRisk == TEXT("High"))
		{
			OutRiskLevel = TEXT("High");
		}
		else if (ItemRisk == TEXT("Medium") && OutRiskLevel == TEXT("Low"))
		{
			OutRiskLevel = TEXT("Medium");
		}
	}

	return Diff;
}

TSharedPtr<FJsonObject> FRevoltEditorBridgeEditorModule::MakeDiffItemObject(const FRevoltDryRunDiffItem& Item) const
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("commandId"), Item.CommandId);
	Object->SetStringField(TEXT("commandName"), Item.CommandName);
	Object->SetStringField(TEXT("targetType"), Item.TargetType);
	Object->SetStringField(TEXT("targetPath"), Item.TargetPath);
	Object->SetStringField(TEXT("targetNameOrPath"), Item.TargetPath);
	Object->SetStringField(TEXT("changeType"), Item.ChangeType);
	Object->SetStringField(TEXT("property"), Item.Property);
	Object->SetStringField(TEXT("field"), Item.Property);
	Object->SetStringField(TEXT("oldValue"), Item.OldValue);
	Object->SetStringField(TEXT("newValue"), Item.NewValue);
	Object->SetStringField(TEXT("riskLevel"), Item.RiskLevel);
	Object->SetStringField(TEXT("permissionLevel"), Item.PermissionLevel);
	Object->SetStringField(TEXT("approvalStatus"), Item.ApprovalStatus);
	return Object;
}

FString FRevoltEditorBridgeEditorModule::JsonValueToDisplayString(const TSharedPtr<FJsonValue>& Value) const
{
	if (!Value.IsValid())
	{
		return TEXT("null");
	}

	switch (Value->Type)
	{
	case EJson::String:
		return Value->AsString();
	case EJson::Number:
		return FString::SanitizeFloat(Value->AsNumber());
	case EJson::Boolean:
		return Value->AsBool() ? TEXT("true") : TEXT("false");
	case EJson::Object:
		return JsonObjectToDisplayString(Value->AsObject());
	case EJson::Array:
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Value->AsArray(), Writer);
		return Output;
	}
	default:
		return TEXT("null");
	}
}

FString FRevoltEditorBridgeEditorModule::JsonObjectToDisplayString(const TSharedPtr<FJsonObject>& Object) const
{
	if (!Object.IsValid())
	{
		return TEXT("{}");
	}

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
	return Output;
}

FString FRevoltEditorBridgeEditorModule::InferRiskLevel(const FString& Command, const FString& TargetPath, int32 ChangeCount) const
{
	if (Command.Contains(TEXT("delete")) || Command.Contains(TEXT("clear_generated_content")))
	{
		return TEXT("Dangerous");
	}

	if (!TargetPath.IsEmpty() && !TargetPath.Contains(TEXT("/Game/RevoltGenerated")) && (Command.Contains(TEXT("asset")) || Command.Contains(TEXT("blueprint")) || Command.StartsWith(TEXT("save_"))))
	{
		return TEXT("High");
	}

	if (ChangeCount > 1 || Command.Contains(TEXT("bulk")) || Command.Contains(TEXT("_template")) || Command.Contains(TEXT("spawn_biome_content")) || Command == TEXT("duplicate_selected_actors"))
	{
		return TEXT("Medium");
	}

	return TEXT("Low");
}

FString FRevoltEditorBridgeEditorModule::InferPermissionLevel(const TSharedPtr<FJsonObject>& Params) const
{
	if (!Params.IsValid())
	{
		return TEXT("SafeEdit");
	}

	FString PermissionLevel;
	Params->TryGetStringField(TEXT("permission_level"), PermissionLevel);
	if (PermissionLevel == TEXT("editor_mutation"))
	{
		return TEXT("SafeEdit");
	}
	if (PermissionLevel == TEXT("asset_mutation"))
	{
		return TEXT("ProjectEdit");
	}
	if (PermissionLevel == TEXT("blueprint_mutation"))
	{
		return TEXT("ProjectEdit");
	}

	return PermissionLevel.IsEmpty() ? TEXT("SafeEdit") : PermissionLevel;
}

void FRevoltEditorBridgeEditorModule::AttachDryRunDiff(TSharedPtr<FJsonObject> Result, const TArray<TSharedPtr<FJsonObject>>& Diff, const FString& RiskLevel, bool bDryRun) const
{
	if (!Result.IsValid())
	{
		return;
	}

	TArray<TSharedPtr<FJsonValue>> DiffValues;
	for (const TSharedPtr<FJsonObject>& DiffItem : Diff)
	{
		DiffValues.Add(MakeShared<FJsonValueObject>(DiffItem));
	}

	Result->SetBoolField(TEXT("dryRun"), bDryRun);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetBoolField(TEXT("requiresApproval"), bDryRun);
	Result->SetBoolField(TEXT("requires_approval"), bDryRun);
	Result->SetStringField(TEXT("riskLevel"), RiskLevel);
	Result->SetStringField(TEXT("risk_level"), RiskLevel);
	Result->SetArrayField(TEXT("diff"), DiffValues);
}

int32 FRevoltEditorBridgeEditorModule::AddHistoryEntry(const FString& Command, const FString& Target, const FString& Status, const FString& ResultSummary)
{
	FRevoltCommandHistoryEntry& Entry = CommandHistory.AddDefaulted_GetRef();
	Entry.Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
	Entry.Command = Command;
	Entry.Target = Target;
	Entry.Status = Status;
	Entry.ResultSummary = ResultSummary;

	return CommandHistory.Num() - 1;
}

void FRevoltEditorBridgeEditorModule::UpdateHistoryEntry(int32 HistoryIndex, const FString& Status, const FString& ResultSummary)
{
	if (CommandHistory.IsValidIndex(HistoryIndex))
	{
		CommandHistory[HistoryIndex].Status = Status;
		CommandHistory[HistoryIndex].ResultSummary = ResultSummary;
	}
}

void FRevoltEditorBridgeEditorModule::RefreshSelectedDiffIndex()
{
	if (!ApprovalQueue.IsValidIndex(SelectedApprovalIndex) || ApprovalQueue[SelectedApprovalIndex].Diff.IsEmpty())
	{
		SelectedDiffIndex = INDEX_NONE;
		return;
	}

	SelectedDiffIndex = FMath::Clamp(SelectedDiffIndex == INDEX_NONE ? 0 : SelectedDiffIndex, 0, ApprovalQueue[SelectedApprovalIndex].Diff.Num() - 1);
}

FReply FRevoltEditorBridgeEditorModule::ApproveSelectedChange()
{
	if (!ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		return FReply::Handled();
	}

	RefreshSelectedDiffIndex();
	if (SelectedDiffIndex == INDEX_NONE)
	{
		return FReply::Handled();
	}

	FRevoltPendingMutation& PendingMutation = ApprovalQueue[SelectedApprovalIndex];
	PendingMutation.Diff[SelectedDiffIndex]->SetStringField(TEXT("approvalStatus"), TEXT("Approved"));
	UpdateHistoryEntry(
		PendingMutation.HistoryIndex,
		TEXT("pending"),
		FString::Printf(TEXT("Approved by user: 1 of %d dry-run change(s). Approve entire command to apply. Risk: %s"), PendingMutation.Diff.Num(), *PendingMutation.RiskLevel));
	SetLastCommand(PendingMutation.Command);
	SetLastResponseSummary(TEXT("Selected dry-run change approved; command has not been applied"));
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::RejectSelectedChange()
{
	if (!ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		return FReply::Handled();
	}

	RefreshSelectedDiffIndex();
	if (SelectedDiffIndex == INDEX_NONE)
	{
		return FReply::Handled();
	}

	FRevoltPendingMutation PendingMutation = ApprovalQueue[SelectedApprovalIndex];
	PendingMutation.Diff[SelectedDiffIndex]->SetStringField(TEXT("approvalStatus"), TEXT("Rejected"));
	RejectedDiffItems.Add(PendingMutation.Diff[SelectedDiffIndex]);
	UpdateHistoryEntry(
		PendingMutation.HistoryIndex,
		TEXT("rejected"),
		FString::Printf(TEXT("Rejected by user | rejected change: %d | total changes: %d | risk: %s"), SelectedDiffIndex, PendingMutation.Diff.Num(), *PendingMutation.RiskLevel));
	SetLastCommand(PendingMutation.Command);
	SetLastResponseSummary(TEXT("Selected dry-run change rejected; command removed from approval queue"));
	ApprovalQueue.RemoveAt(SelectedApprovalIndex);
	SelectedApprovalIndex = ApprovalQueue.IsEmpty() ? INDEX_NONE : FMath::Clamp(SelectedApprovalIndex, 0, ApprovalQueue.Num() - 1);
	RefreshSelectedDiffIndex();
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::ApproveSelectedCommand()
{
	if (!ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		return FReply::Handled();
	}

	FRevoltPendingMutation PendingMutation = ApprovalQueue[SelectedApprovalIndex];
	FString Target;
	FString ResultSummary;
	FString ErrorCode;
	FString ErrorMessage;
	TSharedPtr<FJsonObject> Result = ApplyMutationCommand(PendingMutation.Command, PendingMutation.Params, false, Target, ResultSummary, ErrorCode, ErrorMessage);
	if (Result.IsValid())
	{
		AttachDryRunDiff(Result, PendingMutation.Diff, PendingMutation.RiskLevel, false);
		UpdateHistoryEntry(
			PendingMutation.HistoryIndex,
			TEXT("applied"),
			FString::Printf(TEXT("Approved by user | %s | changes: %d | risk: %s | %s"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")), PendingMutation.Diff.Num(), *PendingMutation.RiskLevel, *ResultSummary));
		SetLastCommand(PendingMutation.Command);
		SetLastResponseSummary(ResultSummary);
	}
	else
	{
		UpdateHistoryEntry(PendingMutation.HistoryIndex, TEXT("rejected"), FString::Printf(TEXT("Approval failed: %s"), *ErrorCode));
		SetLastCommand(PendingMutation.Command);
		SetLastResponseSummary(FString::Printf(TEXT("Error %s"), *ErrorCode));
	}

	ApprovalQueue.RemoveAt(SelectedApprovalIndex);
	SelectedApprovalIndex = ApprovalQueue.IsEmpty() ? INDEX_NONE : FMath::Clamp(SelectedApprovalIndex, 0, ApprovalQueue.Num() - 1);
	RefreshSelectedDiffIndex();
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::RejectSelectedCommand()
{
	if (ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		const FRevoltPendingMutation& PendingMutation = ApprovalQueue[SelectedApprovalIndex];
		for (const TSharedPtr<FJsonObject>& DiffItem : PendingMutation.Diff)
		{
			if (DiffItem.IsValid())
			{
				DiffItem->SetStringField(TEXT("approvalStatus"), TEXT("Rejected"));
				RejectedDiffItems.Add(DiffItem);
			}
		}
		UpdateHistoryEntry(PendingMutation.HistoryIndex, TEXT("rejected"), FString::Printf(TEXT("Rejected by user | changes: %d | risk: %s"), PendingMutation.Diff.Num(), *PendingMutation.RiskLevel));
		SetLastCommand(PendingMutation.Command);
		SetLastResponseSummary(TEXT("Command rejected by user"));
		ApprovalQueue.RemoveAt(SelectedApprovalIndex);
		SelectedApprovalIndex = ApprovalQueue.IsEmpty() ? INDEX_NONE : FMath::Clamp(SelectedApprovalIndex, 0, ApprovalQueue.Num() - 1);
		RefreshSelectedDiffIndex();
	}

	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::ClearRejectedDiffs()
{
	RejectedDiffItems.Empty();
	SetLastResponseSummary(TEXT("Rejected dry-run diffs cleared"));
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::CopyDiffAsJson()
{
	if (!ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		return FReply::Handled();
	}

	const FRevoltPendingMutation& PendingMutation = ApprovalQueue[SelectedApprovalIndex];
	TArray<TSharedPtr<FJsonValue>> DiffValues;
	for (const TSharedPtr<FJsonObject>& DiffItem : PendingMutation.Diff)
	{
		DiffValues.Add(MakeShared<FJsonValueObject>(DiffItem));
	}

	TSharedPtr<FJsonObject> DiffObject = MakeShared<FJsonObject>();
	DiffObject->SetStringField(TEXT("commandId"), PendingMutation.RequestId);
	DiffObject->SetStringField(TEXT("commandName"), PendingMutation.Command);
	DiffObject->SetStringField(TEXT("riskLevel"), PendingMutation.RiskLevel);
	DiffObject->SetArrayField(TEXT("diff"), DiffValues);

	const FString JsonString = JsonObjectToDisplayString(DiffObject);
	FPlatformApplicationMisc::ClipboardCopy(*JsonString);
	SetLastCommand(PendingMutation.Command);
	SetLastResponseSummary(TEXT("Dry-run diff copied as JSON"));
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::ClearCommandHistory()
{
	CommandHistory.Empty();
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::AuditCurrentLevelFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("audit_current_level"), HandleAuditCurrentLevel(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::AuditSelectedActorsFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("audit_selected_actors"), HandleAuditSelectedActors(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::AuditAssetsFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("audit_assets"), HandleAuditAssets(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::AuditBlueprintsFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("audit_blueprints"), HandleAuditBlueprints(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::AuditGeneratedContentFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("audit_generated_content"), HandleAuditGeneratedContent(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::RunFullProjectAuditFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	StoreAuditResult(TEXT("run_full_project_audit"), HandleRunProjectAudit(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::GenerateFixPlanFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	TSharedPtr<FJsonObject> Params = LastAuditReport.IsValid() ? LastAuditReport : MakeShared<FJsonObject>();
	StoreAuditResult(TEXT("generate_fix_plan"), HandleGenerateFixPlan(Params, ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	return FReply::Handled();
}

FReply FRevoltEditorBridgeEditorModule::ExportAuditJsonFromUi()
{
	FString ErrorCode;
	FString ErrorMessage;
	if (!LastAuditReport.IsValid())
	{
		StoreAuditResult(TEXT("export_audit_report"), HandleExportAuditReport(MakeShared<FJsonObject>(), ErrorCode, ErrorMessage), ErrorCode, ErrorMessage);
	}

	if (LastAuditReport.IsValid())
	{
		FPlatformApplicationMisc::ClipboardCopy(*JsonObjectToDisplayString(LastAuditReport));
		SetLastCommand(TEXT("export_audit_report"));
		SetLastResponseSummary(TEXT("Audit JSON copied locally to clipboard"));
		LastAuditSummary = TEXT("Audit JSON copied locally to clipboard.");
	}
	return FReply::Handled();
}

void FRevoltEditorBridgeEditorModule::StoreAuditResult(const FString& Command, const TSharedPtr<FJsonObject>& Result, const FString& ErrorCode, const FString& ErrorMessage)
{
	SetLastCommand(Command);
	if (!Result.IsValid())
	{
		LastAuditSummary = FString::Printf(TEXT("%s failed: %s"), *Command, ErrorMessage.IsEmpty() ? *ErrorCode : *ErrorMessage);
		SetLastResponseSummary(FString::Printf(TEXT("Error %s"), *ErrorCode));
		return;
	}

	LastAuditReport = Result;
	double IssueCount = 0.0;
	Result->TryGetNumberField(TEXT("issue_count"), IssueCount);
	LastAuditSummary = FString::Printf(TEXT("%s complete | issues: %d | read-only: true"), *Command, FMath::FloorToInt(IssueCount));
	SetLastResponseSummary(LastAuditSummary);
}

FString FRevoltEditorBridgeEditorModule::BuildApprovalQueueText() const
{
	if (ApprovalQueue.IsEmpty())
	{
		return TEXT("No commands waiting for approval.");
	}

	TArray<FString> Lines;
	for (int32 Index = 0; Index < ApprovalQueue.Num(); ++Index)
	{
		const FRevoltPendingMutation& PendingMutation = ApprovalQueue[Index];
		const FString SelectionMarker = Index == SelectedApprovalIndex ? TEXT("*") : TEXT(" ");
		Lines.Add(FString::Printf(TEXT("%s [%d] %s | %s | %s | %s"),
			*SelectionMarker,
			Index,
			*PendingMutation.Timestamp,
			*PendingMutation.Command,
			*PendingMutation.Target,
			*FString::Printf(TEXT("%s | changes: %d | risk: %s"), *PendingMutation.Summary, PendingMutation.Diff.Num(), *PendingMutation.RiskLevel)));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString FRevoltEditorBridgeEditorModule::BuildProjectAuditText() const
{
	TArray<FString> Lines;
	Lines.Add(LastAuditSummary);
	if (LastAuditReport.IsValid())
	{
		FString AuditType;
		LastAuditReport->TryGetStringField(TEXT("audit_type"), AuditType);
		double IssueCount = 0.0;
		LastAuditReport->TryGetNumberField(TEXT("issue_count"), IssueCount);
		Lines.Add(FString::Printf(TEXT("Last Audit Type: %s | Issue Count: %d | Read-only suggestions only"), *AuditType, FMath::FloorToInt(IssueCount)));

		const TArray<TSharedPtr<FJsonValue>>* Issues = nullptr;
		if (LastAuditReport->TryGetArrayField(TEXT("issues"), Issues) && Issues)
		{
			const int32 MaxVisibleIssues = FMath::Min(Issues->Num(), 8);
			for (int32 Index = 0; Index < MaxVisibleIssues; ++Index)
			{
				const TSharedPtr<FJsonObject> Issue = (*Issues)[Index].IsValid() ? (*Issues)[Index]->AsObject() : nullptr;
				if (!Issue.IsValid())
				{
					continue;
				}

				FString IssueId;
				FString Severity;
				FString Category;
				FString TargetPath;
				FString Message;
				Issue->TryGetStringField(TEXT("issueId"), IssueId);
				Issue->TryGetStringField(TEXT("severityDisplay"), Severity);
				Issue->TryGetStringField(TEXT("category"), Category);
				Issue->TryGetStringField(TEXT("targetPath"), TargetPath);
				Issue->TryGetStringField(TEXT("message"), Message);
				Lines.Add(FString::Printf(TEXT("- %s | %s | %s | %s | %s"), *Severity, *Category, *IssueId, *TargetPath, *Message));
			}
			if (Issues->Num() > MaxVisibleIssues)
			{
				Lines.Add(FString::Printf(TEXT("... %d more issue(s). Use Export Audit JSON for full details."), Issues->Num() - MaxVisibleIssues));
			}
		}
	}
	else
	{
		Lines.Add(TEXT("Audits never modify actors, assets, Blueprints, source files, maps, settings, or generated content."));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString FRevoltEditorBridgeEditorModule::BuildDryRunDiffText() const
{
	TArray<FString> Lines;
	Lines.Add(TEXT("Sel | Command ID | Command | Target Type | Target Name/Path | Property/Field | Old Value | New Value | Risk | Permission | Approval"));

	if (ApprovalQueue.IsValidIndex(SelectedApprovalIndex))
	{
		const FRevoltPendingMutation& PendingMutation = ApprovalQueue[SelectedApprovalIndex];
		for (int32 Index = 0; Index < PendingMutation.Diff.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>& Item = PendingMutation.Diff[Index];
			if (!Item.IsValid())
			{
				continue;
			}

			const FString SelectionMarker = Index == SelectedDiffIndex ? TEXT("*") : TEXT(" ");
			Lines.Add(FString::Printf(TEXT("%s [%d] | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s"),
				*SelectionMarker,
				Index,
				*Item->GetStringField(TEXT("commandId")),
				*Item->GetStringField(TEXT("commandName")),
				*Item->GetStringField(TEXT("targetType")),
				*Item->GetStringField(TEXT("targetNameOrPath")),
				*Item->GetStringField(TEXT("property")),
				*Item->GetStringField(TEXT("oldValue")),
				*Item->GetStringField(TEXT("newValue")),
				*Item->GetStringField(TEXT("riskLevel")),
				*Item->GetStringField(TEXT("permissionLevel")),
				*Item->GetStringField(TEXT("approvalStatus"))));
		}
	}
	else
	{
		Lines.Add(TEXT("No pending dry-run changes. Mutating commands must generate a diff before approval."));
	}

	if (!RejectedDiffItems.IsEmpty())
	{
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("Rejected changes retained: %d. Use Clear Rejected to hide them."), RejectedDiffItems.Num()));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString FRevoltEditorBridgeEditorModule::BuildCommandHistoryText() const
{
	if (CommandHistory.IsEmpty())
	{
		return TEXT("No mutating commands have been recorded.");
	}

	TArray<FString> Lines;
	const int32 FirstEntryIndex = FMath::Max(0, CommandHistory.Num() - 25);
	for (int32 Index = FirstEntryIndex; Index < CommandHistory.Num(); ++Index)
	{
		const FRevoltCommandHistoryEntry& Entry = CommandHistory[Index];
		Lines.Add(FString::Printf(TEXT("%s | %s | %s | %s | %s"),
			*Entry.Timestamp,
			*Entry.Command,
			*Entry.Target,
			*Entry.Status,
			*Entry.ResultSummary));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString FRevoltEditorBridgeEditorModule::MakeSuccessResponse(const FString& RequestId, const TSharedPtr<FJsonObject>& Result) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("id"), RequestId);
	Response->SetBoolField(TEXT("ok"), true);
	Response->SetObjectField(TEXT("result"), Result.IsValid() ? Result : MakeShared<FJsonObject>());

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return Output;
}

FString FRevoltEditorBridgeEditorModule::MakeErrorResponse(const FString& RequestId, const FString& ErrorCode, const FString& ErrorMessage) const
{
	TSharedPtr<FJsonObject> Error = MakeShared<FJsonObject>();
	Error->SetStringField(TEXT("code"), ErrorCode);
	Error->SetStringField(TEXT("message"), ErrorMessage);

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("id"), RequestId);
	Response->SetBoolField(TEXT("ok"), false);
	Response->SetObjectField(TEXT("error"), Error);

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	return Output;
}

void FRevoltEditorBridgeEditorModule::SetLastCommand(const FString& Command)
{
	LastCommand = Command;
}

void FRevoltEditorBridgeEditorModule::SetLastResponseSummary(const FString& Summary)
{
	LastResponseSummary = Summary;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRevoltEditorBridgeEditorModule, RevoltEditorBridgeEditor)
