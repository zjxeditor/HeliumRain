
#include "../../Flare.h"
#include "FlareMainOverlay.h"

#include "../Components/FlareObjectiveInfo.h"
#include "../../Game/FlareGameUserSettings.h"
#include "../../Player/FlareHUD.h"
#include "../../Player/FlareMenuManager.h"

#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "Runtime/Engine/Classes/Engine/RendererSettings.h"

#define LOCTEXT_NAMESPACE "FlareMainOverlay"


/*----------------------------------------------------
	Construct
----------------------------------------------------*/

void SFlareMainOverlay::Construct(const FArguments& InArgs)
{
	// Data
	MenuManager = InArgs._MenuManager;
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();
	TitleButtonWidth = 2.0f;
	TitleButtonHeight = AFlareMenuManager::GetMainOverlayHeight() / (float)(Theme.ButtonHeight);

	// Create the layout
	ChildSlot
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Fill)
	[
		SAssignNew(Background, SBorder)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0))
		.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(MenuList, SHorizontalBox)

				// Title
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					.WidthOverride(0.8 * Theme.ContentWidth)
					[
						SNew(SHorizontalBox)

						// Title icon
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Top)
						[
							SNew(SImage)
							.Image(this, &SFlareMainOverlay::GetCurrentMenuIcon)
						]

						// Title text
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.SmallContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.TitleFont)
								.Text(this, &SFlareMainOverlay::GetCurrentMenuName)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.SmallContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.TextFont)
								.Text(this, &SFlareMainOverlay::GetSpacecraftInfo)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(Theme.SmallContentPadding)
							[
								SNew(STextBlock)
								.TextStyle(&Theme.TextFont)
								.Text(this, &SFlareMainOverlay::GetPlayerInfo)
							]
						]
					]
				]

				// Objective
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SFlareObjectiveInfo)
					.PC(MenuManager->GetPC())
					.Visibility(EVisibility::SelfHitTestInvisible)
				]
			]

			// Bottom border
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SAssignNew(Border, SImage)
				.Image(&Theme.NearInvisibleBrush)
			]
		]
	];

	// Add menus
	AddMenuLink(EFlareMenu::MENU_Ship);
	AddMenuLink(EFlareMenu::MENU_Sector);
	AddMenuLink(EFlareMenu::MENU_Orbit);
	AddMenuLink(EFlareMenu::MENU_Leaderboard);
	AddMenuLink(EFlareMenu::MENU_Company);
	AddMenuLink(EFlareMenu::MENU_Fleet);
	AddMenuLink(EFlareMenu::MENU_Settings);
	AddMenuLink(EFlareMenu::MENU_Main);

	TSharedPtr<SFlareButton> BackButton;
	TSharedPtr<SFlareButton> ExitButton;
	MenuList->AddSlot()
	.HAlign(HAlign_Right)
	[
		SNew(SVerticalBox)

		// Back button
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(BackButton, SFlareButton)
			.Width(TitleButtonWidth)
			.Height(TitleButtonHeight / 2)
			.Transparent(true)
			.OnClicked(this, &SFlareMainOverlay::OnBack)
			.IsDisabled(this, &SFlareMainOverlay::IsBackDisabled)
		]

		// Exit button
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ExitButton, SFlareButton)
			.Width(TitleButtonWidth)
			.Height(TitleButtonHeight / 2)
			.Transparent(true)
			.OnClicked(this, &SFlareMainOverlay::OnCloseMenu)
		]
	];

	SetupMenuLink(BackButton, FFlareStyleSet::GetIcon("Back"), FText(), true);
	SetupMenuLink(ExitButton, AFlareMenuManager::GetMenuIcon(EFlareMenu::MENU_FlyShip), FText(), true);

	// Init
	Close();

	// Setup the post process 
	TArray<AActor*> PostProcessCandidates;
	UGameplayStatics::GetAllActorsOfClass(MenuManager->GetPC()->GetWorld(), APostProcessVolume::StaticClass(), PostProcessCandidates);
	if (PostProcessCandidates.Num())
	{
		APostProcessVolume* Volume = Cast<APostProcessVolume>(PostProcessCandidates.Last());
		check(Volume);

		FWeightedBlendable Blendable = Volume->Settings.WeightedBlendables.Array.Last();
		UMaterial* MasterMaterial = Cast<UMaterial>(Blendable.Object);
		if (MasterMaterial)
		{
			BlurMaterial = UMaterialInstanceDynamic::Create(MasterMaterial, MenuManager->GetPC()->GetWorld());
			check(BlurMaterial);
			
			Volume->Settings.RemoveBlendable(MasterMaterial);
			Volume->Settings.AddBlendable(BlurMaterial, 1.0f);
			FLOG("SFlareMainOverlay::Construct : blur material ready");
		}
		else
		{
			FLOG("SFlareMainOverlay::Construct : no usable material found for blur");
		}
	}
	else
	{
		FLOG("SFlareMainOverlay::Construct : no post process found");
	}
}

void SFlareMainOverlay::AddMenuLink(EFlareMenu::Type Menu)
{
	TSharedPtr<SFlareButton> Button;

	// Create button
	MenuList->AddSlot()
	.AutoWidth()
	.HAlign(HAlign_Left)
	[
		SAssignNew(Button, SFlareButton)
		.Width(TitleButtonWidth)
		.Height(TitleButtonHeight)
		.Transparent(true)
		.OnClicked(this, &SFlareMainOverlay::OnOpenMenu, Menu)
	];

	// Fill button contents
	SetupMenuLink(Button, AFlareMenuManager::GetMenuIcon(Menu), AFlareMenuManager::GetMenuName(Menu), false);
}

void SFlareMainOverlay::SetupMenuLink(TSharedPtr<SFlareButton> Button, const FSlateBrush* Icon, FText Text, bool Small)
{
	const FFlareStyleCatalog& Theme = FFlareStyleSet::GetDefaultTheme();

	Button->GetContainer()->SetHAlign(HAlign_Center);
	Button->GetContainer()->SetVAlign(VAlign_Fill);
	Button->GetContainer()->SetPadding(FMargin(0, Small ? 5 : 30));

	Button->GetContainer()->SetContent(
		SNew(SVerticalBox)

		// Icon
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(64)
			.HeightOverride(64)
			[
				SNew(SImage)
				.Image(Icon)
			]
		]

		// Text
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&Theme.SmallFont)
			.Text(Text)
		]
	);
}


/*----------------------------------------------------
	Interaction
----------------------------------------------------*/

void SFlareMainOverlay::Open()
{
	IsOverlayVisible = true;
	Background->SetVisibility(EVisibility::Visible);
}

void SFlareMainOverlay::Close()
{
	IsOverlayVisible = false;
	Background->SetVisibility(EVisibility::Hidden);
}

bool SFlareMainOverlay::IsOpen() const
{
	return IsOverlayVisible;
}


/*----------------------------------------------------
	Callbacks
----------------------------------------------------*/

void SFlareMainOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Get 2D position
	FVector2D MousePosition = MenuManager->GetPC()->GetMousePosition();
	FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
	float ViewportScale = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));

	// Visibility check
	float Height = AFlareMenuManager::GetMainOverlayHeight();
	if (!IsOverlayVisible || (MousePosition.Y / ViewportScale) > Height)
	{
		SetVisibility(EVisibility::HitTestInvisible);
	}
	else
	{
		SetVisibility(EVisibility::Visible);
	}

	// Update blur material with appropriate values
	if (IsOpen())
	{
		UFlareGameUserSettings* MyGameSettings = Cast<UFlareGameUserSettings>(GEngine->GetGameUserSettings());
		float ScreenScale = MyGameSettings->ScreenPercentage / 100.0f;
		BlurMaterial->SetVectorParameterValue("PanelPosition", FVector(0.0, 0.0, 0));
		BlurMaterial->SetVectorParameterValue("PanelSize", FVector(ScreenScale * ViewportSize.X, ViewportScale * ScreenScale * Height, 0));
	}
	else
	{
		BlurMaterial->SetVectorParameterValue("PanelSize", FVector(0.0, 0.0, 0));
	}
}

bool SFlareMainOverlay::IsBackDisabled() const
{
	return (MenuManager->HasPreviousMenu() == false);
}

FText SFlareMainOverlay::GetCurrentMenuName() const
{
	FText Name;
	
	if (MenuManager->IsMenuOpen())
	{
		Name = AFlareMenuManager::GetMenuName(MenuManager->GetCurrentMenu());
	}
	else
	{
		Name = LOCTEXT("FlyingText", "Flying");
	}

	return FText::FromString(Name.ToString().ToUpper());
}

const FSlateBrush* SFlareMainOverlay::GetCurrentMenuIcon() const
{
	if (MenuManager->IsMenuOpen())
	{
		return AFlareMenuManager::GetMenuIcon(MenuManager->GetCurrentMenu());
	}
	else
	{
		return AFlareMenuManager::GetMenuIcon(EFlareMenu::MENU_Sector);
	}
}

FText SFlareMainOverlay::GetSpacecraftInfo() const
{
	AFlarePlayerController* PC = MenuManager->GetPC();

	if (MenuManager->IsMenuOpen() && MenuManager->GetCurrentMenu() == EFlareMenu::MENU_None)
	{
		return FText();
	}
	else if (PC->GetShipPawn())
	{
		return PC->GetShipPawn()->GetShipStatus();
	}
	else
	{
		return LOCTEXT("FastForwarding", "Fast forwarding...");
	}

	return FText();
}

FText SFlareMainOverlay::GetPlayerInfo() const
{
	AFlarePlayerController* PC = MenuManager->GetPC();

	if (MenuManager->IsMenuOpen() && MenuManager->GetCurrentMenu() == EFlareMenu::MENU_None)
	{
		return FText();
	}
	else if (PC->GetCompany())
	{
		return FText::Format(LOCTEXT("PlayerMoneyFormat", "{0} credits available"),
			FText::AsNumber(PC->GetCompany()->GetMoney() / 100));
	}

	return FText();
}

void SFlareMainOverlay::OnOpenMenu(EFlareMenu::Type Menu)
{
	MenuManager->OpenMenu(Menu);
}

void SFlareMainOverlay::OnBack()
{
	MenuManager->Back();
}

void SFlareMainOverlay::OnCloseMenu()
{
	if (MenuManager->IsMenuOpen())
	{
		MenuManager->CloseMenu();
	}
	else
	{
		MenuManager->CloseMainOverlay();
	}
}



#undef LOCTEXT_NAMESPACE
