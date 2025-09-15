//The following include files are necessary to allow your MyPanel.cpp to compile.
#include "cbase.h"
#include "vgui_singeplayer.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ListPanel.h>
#include <filesystem.h>
#include <KeyValues.h>
#include "ienginevgui.h"
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/ImagePanel.h>

//Swarm wants YOU!
#include "vgui_controls/Menu.h"

#include <stdlib.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Used by the autocompletion system
//-----------------------------------------------------------------------------
class CNonFocusableMenu : public Menu
{
	DECLARE_CLASS_SIMPLE(CNonFocusableMenu, Menu);

public:
	CNonFocusableMenu(Panel *parent, const char *panelName)
		: BaseClass(parent, panelName),
		m_pFocus(0)
	{
	}

	void SetFocusPanel(Panel *panel)
	{
		m_pFocus = panel;
	}

	VPANEL GetCurrentKeyFocus()
	{
		if (!m_pFocus)
			return GetVPanel();

		return m_pFocus->GetVPanel();
	}

private:
	Panel		*m_pFocus;
};

class CMyPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CMyPanel, vgui::Frame);

public:
	CMyPanel(vgui::VPANEL parent);
	~CMyPanel() {}

protected:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand) override;

private:
	PropertySheet* m_pTabPanel;
	ListPanel* m_pBrowseAllList;
	Button* m_pPlayButton;
	PanelListPanel* m_pBrowseAllPanel; // For "Browse All" tab
	PanelListPanel* m_pGameMapPanel;  // For "Game" tab to hold icons

	void PopulateBrowseAll();
	void PopulateGameTab();
	void PlaySelectedMap(); // Function to handle playing the selected map
	void CreateGameIcon(const char* mapName, const char* imagePath, const char* command);
};



CMyPanel::CMyPanel(vgui::VPANEL parent)
	: BaseClass(nullptr, "MyPanel")
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetTitle("Play Singleplayer", false);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	LoadControlSettings("resource/UI/singleplayer_srcbox.res");

	// Create PropertySheet (Tab Panel)
	m_pTabPanel = new PropertySheet(this, "TabPanel");
	m_pTabPanel->SetBounds(10, 40, 456, 714); // Adjust to fit your window dimensions

	// Placeholder for "Game" Tab
	Panel* pGamePanel = new Panel(m_pTabPanel, "GamePanel");
	pGamePanel->SetPaintBackgroundEnabled(true);
	pGamePanel->SetBgColor(Color(50, 50, 50, 255)); // Background color for the "Game" tab

	// Container for map icons in the Game tab
	m_pGameMapPanel = new PanelListPanel(pGamePanel, "GameMapPanel");
	m_pGameMapPanel->SetBounds(10, 10, 430, 700); // Adjust to fit your window dimensions
	m_pGameMapPanel->SetFirstColumnWidth(200); // Layout adjustments

	// Add the Game tab to the PropertySheet
	//m_pTabPanel->AddPage(pGamePanel, "Game");

	// "Game" Tab
	PopulateGameTab();
	m_pTabPanel->AddPage(m_pGameMapPanel, "Game");

	// "Browse All" Tab
	m_pBrowseAllList = new ListPanel(m_pTabPanel, "BrowseAllList");
	m_pBrowseAllList->SetPaintBackgroundEnabled(true);
	m_pBrowseAllList->SetBgColor(Color(50, 50, 50, 255));

	// Define the columns for the table
	m_pBrowseAllList->AddColumnHeader(0, "MapName", "Map Name", 350, ListPanel::COLUMN_FIXEDSIZE);
	m_pBrowseAllList->AddColumnHeader(1, "Game", "Game", 100, ListPanel::COLUMN_FIXEDSIZE);

	m_pTabPanel->AddPage(m_pBrowseAllList, "Browse All");

	// Populate "Browse All" Tab
	PopulateBrowseAll();


	// Add the Play button
	m_pPlayButton = new Button(this, "PlayButton", "Play");
	m_pPlayButton->SetCommand("play_selected_map");
	m_pPlayButton->SetBounds(300, 760, 64, 24); // Adjust position and size
	m_pPlayButton->SetVisible(true);

	// can disable for now.
	SetVisible(false);
}

void CMyPanel::PopulateBrowseAll()
{
	m_pBrowseAllList->RemoveAll(); // Clear any existing entries

	// Define prefix categories
	struct GameCategory
	{
		const char* gameName;
		const char* prefixes[30]; // Up to 10 prefixes per category (adjust as needed)
	};

	GameCategory categories[] = {
		{ "Gmod", { "gm_", nullptr } },
		{ "TF2", { "ctf_", "koth_", "cp_", "pl_", "mvm_", "pass_", "pd_", "sd_", "tc_", "vsh_", "zi_", "tr_", nullptr } },
		{ "CS:S", { "de_", "cs_", nullptr } },
		{ "HL2 Beta", { "d1_garage_", nullptr } },
		{ "HL:S", { "t0", nullptr } },
		{ "HL2", { "d1_trainstation", "d2_", "c17_", "ep1_", "ep1_", nullptr } },
		{ "HL2:DM", { "dm_", nullptr } },
		{ "L4D", { "l4d_", nullptr } },
		{ "L4D2", { "c1m1_hotel", "c1m2_streets", "c1m3_mall", "c2m1_highway", nullptr } },
		{ "Unknown Game", { nullptr } } // Default fallback
	};

	// Scan maps directory
	const char* mapsDir = "maps/*.bsp";
	FileFindHandle_t findHandle;
	const char* fileName = g_pFullFileSystem->FindFirst(mapsDir, &findHandle);

	while (fileName)
	{
		if (strstr(fileName, ".bsp"))
		{
			// Remove the ".bsp" extension
			char mapName[MAX_PATH];
			Q_strncpy(mapName, fileName, sizeof(mapName));
			mapName[strlen(mapName) - 4] = '\0';

			// Determine the game category based on prefixes
			const char* detectedGame = "Srcbox";
			for (const auto& category : categories)
			{
				for (int i = 0; category.prefixes[i] != nullptr; ++i)
				{
					if (Q_strnicmp(mapName, category.prefixes[i], strlen(category.prefixes[i])) == 0)
					{
						detectedGame = category.gameName;
						break;
					}
				}
				//if (strcmp(detectedGame, "Unknown Game") != 0)
					//break; // Stop searching if we found a match
			}

			// Create a row for the ListPanel
			KeyValues* kv = new KeyValues("MapData");
			kv->SetString("MapName", mapName);
			kv->SetString("Game", detectedGame);

			m_pBrowseAllList->AddItem(kv, 0, false, false); // Add the map as a new row
			kv->deleteThis(); // Clean up
		}

		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);
}

void CMyPanel::PopulateGameTab()
{
	// Clear previous entries
	if (m_pGameMapPanel)
	{
		m_pGameMapPanel->DeleteAllItems();
	}

	// Define layout settings
	const int itemWidth = 150;    // Width of each map container
	const int itemHeight = 200;   // Height of each map container (to fit image + labels)
	const int columns = 3;        // Number of columns per row
	const int spacing = 1;       // Spacing between containers

	int x = 10;  // Initial X position
	int y = 10;  // Initial Y position
	int columnIndex = 0;

	// Scan maps directory for ".bsp" files
	const char* mapsDir = "maps/*.bsp";
	FileFindHandle_t findHandle;
	const char* fileName = g_pFullFileSystem->FindFirst(mapsDir, &findHandle);

	while (fileName)
	{
		if (strstr(fileName, ".bsp"))
		{
			// Remove the ".bsp" extension
			char mapName[MAX_PATH];
			Q_strncpy(mapName, fileName, sizeof(mapName));
			mapName[strlen(mapName) - 4] = '\0';

			// Create a container panel for the map entry
			Panel* pMapContainer = new Panel(m_pGameMapPanel, mapName);
			pMapContainer->SetBounds(x, y, itemWidth, itemHeight);
			pMapContainer->SetPaintBackgroundEnabled(true);
			pMapContainer->SetBgColor(Color(60, 60, 60, 255)); // Background for the container

			// Add a clickable ImagePanel for the map thumbnail
			ImagePanel* pImagePanel = new ImagePanel(pMapContainer, "MapThumbnail");
			pImagePanel->SetBounds(-50 , 5, itemWidth - 10, 150); // Thumbnail size
			pImagePanel->SetImage("../maps/no_ico");              // Use a default thumbnail
			pImagePanel->AddActionSignalTarget(this);

			// Add a label for the map name (top)
			//Label* pMapLabel = new Label(pMapContainer, "MapNameLabel", mapName);
			//pMapLabel->SetBounds(5, 130, itemWidth - 10, 20);
			//pMapLabel->SetContentAlignment(Label::a_center);
			//pMapLabel->SetFgColor(Color(255, 255, 255, 255));

			// Add a secondary label for the author (bottom)
			//Label* pAuthorLabel = new Label(pMapContainer, "AuthorLabel", "Garry Newman");
			//pAuthorLabel->SetBounds(5, 150, itemWidth - 10, 20); // Adjust position for author label pAuthorLabel->SetContentAlignment(Label::a_center); pAuthorLabel->SetFgColor(Color(200, 200, 200, 255));

			// Add the map container to the GameMapPanel
			m_pGameMapPanel->AddItem(nullptr, pMapContainer);

			// Adjust X and Y positions for the next map container
			columnIndex++;
			if (columnIndex >= columns)
			{
				columnIndex = 0;
				x = 10;                      // Reset to initial X position
				y += itemHeight + spacing;   // Move down to the next row
			}
			else
			{
				x += itemWidth + spacing;    // Move to the next column
			}
		}

		// Get the next map file
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);
}


// Start Game icon stuff

void CMyPanel::CreateGameIcon(const char* mapName, const char* imagePath, const char* command)
{
	// Create a Button that acts as the clickable area
	vgui::Button* pImageButton = new vgui::Button(m_pGameMapPanel, mapName, "");
	//pImageButton->SetSize(160, 90); // Set the size of the button
	pImageButton->SetCommand(command);
	pImageButton->AddActionSignalTarget(this);

	// Add an ImagePanel inside the button to display the image
	vgui::ImagePanel* pImage = new vgui::ImagePanel(pImageButton, "GameImage");
	pImage->SetImage(imagePath);
	pImage->SetShouldScaleImage(true);
	//pImage->SetSize(160, 90); // Set the image size same as button
}



void CMyPanel::PlaySelectedMap()
{
	// Get the selected item from the list
	int selectedItemID = m_pBrowseAllList->GetSelectedItem(0); // Get the first selected item
	if (selectedItemID == -1)
	{
		// No map selected
		Msg("No map selected.\n");
		return;
	}

	// Retrieve the map name from the selected item
	KeyValues* kv = m_pBrowseAllList->GetItem(selectedItemID);
	const char* mapName = kv->GetString("MapName");

	// Construct the map load command
	char command[256];
	Q_snprintf(command, sizeof(command), "map %s\n", mapName);

	// Execute the command to load the map
	engine->ClientCmd(command);
}

void CMyPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);

	if (FStrEq(pcCommand, "play_selected_map"))
	{
		PlaySelectedMap(); // Handle the Play button command
	}
}

// Class for managing panel instance
class CMyPanelInterface : public IMyPanel
{
private:
	CMyPanel* m_pPanel;

public:
	CMyPanelInterface()
		: m_pPanel(nullptr) {}

	void Create(vgui::VPANEL parent) override
	{
		if (!m_pPanel)
		{
			m_pPanel = new CMyPanel(parent);
		}
	}

	void Destroy() override
	{
		if (m_pPanel)
		{
			m_pPanel->SetParent(nullptr);
			delete m_pPanel;
			m_pPanel = nullptr;
		}
	}

	void Activate() override
	{
		if (m_pPanel)
		{
			m_pPanel->Activate();
		}
	}
};

static CMyPanelInterface g_MyPanel;
IMyPanel* mypanel = (IMyPanel*)&g_MyPanel;

void CMyPanel::OnTick()
{
	BaseClass::OnTick();
}

CON_COMMAND(srcbox_singleplayer, "Toggle the panel for Singleplayer")
{
	mypanel->Activate();
};

































