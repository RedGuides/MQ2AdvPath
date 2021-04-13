//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
// Projet: MQ2AdvPath.cpp
// Author: A_Enchanter_00
// version 8.1011 by eqmule, changed the way open doors works
// version 9.0
// version 9.1 Fixed Underwater checks, it will now correctly face the loc when in/under water. -eqmule
// version 9.2 SwiftyMUSE 01-07-2019 Removed foreground detection since its in core
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
//
//
//
//
//   AdvPath TLO (${AdvPath.Member})
//			Active
//			State
//			Waypoints
//			NextWaypoint
//			Y[# or Checkpoint Name]
//			X[# or Checkpoint Name]
//			Z[# or Checkpoint Name]
//			Monitor
//			Idle
//			Length
//			Following
//			Playing
//			Recording
//			Status
//			Paused
//			WaitingWarp
//			Path (Returns Closest Path)
//          PathList[pathname] Returns True if pathname exists or False if pathname not found
//          PathExists[pathName] same as PathList
//			CheckPoint[# or Checkpoint Name]
//			CustomPaths(gives a count filtered by CustomSearch)
//			Pulling
//          Direction(n/r)
//          Fleeing (overides noeval or when eval flag for checkpoint is 0)
//          Flag1 Internal flags that can be set and checked.
//          Flag2
//          Flag3
//          Flag4
//          Flag5
//          Flag6
//          Flag7
//          Flag8
//          Flag9
//          CustomSearch (contains value used for filtering CustomPaths and /play listcustom)
//			PathCount Gives the total number of paths in the path ini file
//			FirstWayPoint[PathName] returns the first waypoint in "Y X Z CheckPoint" format as a string
//
//   /Play Command
//   /play path on|off pause|unpause loop|noloop normal|reverse smart|nosmart door|nodoor fast|slow eval|noeval zone|nozone list listcustom show stop flee|noflee setflag1-setflag9 n resetflags
//         setflag1 n (n can be any single alpha character) default is 'y' and resetflags sets all flags back to 'y'
//   /advpath Command
//   /advpath on|off pull|nopull customsearch save help
//
//   *************************************************************************************************************
//   Pulling mode will automatically /play reverse when the end of the current path is reached. Kind of like loop, but with
//   some additional logic build in.
//
//   /advpath customsearch apath (sets the value of costumsearch = apath)
//   /advpath save (saves the changes to customsearch to the advpath.ini settings file)
//
#include <mq/Plugin.h>

#include <direct.h>

PreSetup("MQ2AdvPath");
PLUGIN_VERSION(9.2);

constexpr int FOLLOW_OFF = 0;
constexpr int FOLLOW_FOLLOWING = 1;
constexpr int FOLLOW_PLAYING = 2;
constexpr int FOLLOW_RECORDING = 3;

constexpr int STATUS_OFF = 0;
constexpr int STATUS_ON = 1;
constexpr int STATUS_PAUSED = 2;

constexpr float DISTANCE_BETWEN_LOG = 5;
constexpr float DISTANCE_OPEN_DOOR_CLOSE = 10;
constexpr float ANGLE_OPEN_DOOR_CLOSE = 50;
constexpr float DISTANCE_OPEN_DOOR_LONG = 15;
constexpr float ANGLE_OPEN_DOOR_LONG = 95;

constexpr uint64_t ZONE_TIME = 6000;

// Timer Structure
struct Position {
	FLOAT X;
	FLOAT Y;
	FLOAT Z;
	FLOAT Heading;
	char CheckPoint[MAX_STRING];
	bool Warping;
	bool Eval;
} pPosition;

//struct PathInfo {
//	char Path[MAX_STRING];
//    bool PlaySmart;
//    bool PlayEval;
//    int  PlayDirection;				// Play Direction - replaces PlayReverse
//    long PlayWaypoint;
//} pPathInfo;

int FollowState = FOLLOW_OFF;		// Active?
int StatusState = STATUS_OFF;		// Active?

int FollowSpawnDistance = 10;		// Active?

int FollowIdle = 0;				// FollowIdle time when Follow on?
int NextClickDoor = 0;				// NextClickDoor when Follow on?
int PauseDoor = 0;					// PauseDoor paused Follow on and near door?
bool AutoOpenDoor = true;

bool PlayOne = false;			// Play one frame only - used for debugging
bool PlaySlow = false;			// Play Slow - turns on walk and doesn't skip points when in background
bool PlaySmart = false;
bool PlayEval = true;
int  PlayDirection = 1;				// Play Direction - replaces PlayReverse
									//bool PlayReverse   = false;			// Play Reversed ?
bool PlayLoop = false;			// Play Loop? a->b a->b a->b a->b ....
								//bool PlayReturn  = false;			// Play Return? a->b b->a
long PlayWaypoint = 0;
bool PlayZone = false;
uint64_t PlayZoneTick = 0;
uint64_t unPauseTick = 0;
bool AdvPathStatus = true;

// Pulling Mode Variables Here
bool PullMode = false;         // Mode for running paths when pulling.
bool IFleeing = false;         // Setting this will play back pathing in reverse direction
bool SaveMapPath   = false;         // to control clearing the map path
CHAR advFlag[11] = { 'y','y','y','y','y','y','y','y','y','y','\0' }; // Flags that can be used for controlling pathing.
bool StopState = false;
CHAR custSearch[64] = { 0 };
long begWayPoint   = 0;
long endWayPoint   = 0;

long MonitorID = 0;					// Spawn To Monitor and follow
FLOAT MonitorX = 0;					// Spawn To MonitorX
FLOAT MonitorY = 0;					// Spawn To MonitorY
FLOAT MonitorZ = 0;					// Spawn To MonitorZ
FLOAT MonitorHeading = 0;			// Spawn To MonitorHeading
bool MonitorWarp = false;			// Spawn To Monitor has warped

FLOAT MeMonitorX = 0;					// MeMonitorX monitor your self
FLOAT MeMonitorY = 0;					// MeMonitorY monitor your self
FLOAT MeMonitorZ = 0;					// MeMonitorZ monitor your self

CHAR Buffer[MAX_STRING] = { 0 };					// Buffer for String manipulatsion
CHAR SavePathName[MAX_STRING] = { 0 };			// Buffer for Save Path Name
CHAR SavePathZone[MAX_STRING] = { 0 };			// Buffer for Save Zone Name
long MaxChkPointDist = 0;

std::list<Position>	FollowPath;			// FollowPath
std::queue<std::string>   PathList;

class MQ2AdvPathType *pAdvPathType = 0;

void MQRecordCommand(PSPAWNINFO pChar, PCHAR szLine);
void MQPlayCommand(PSPAWNINFO pChar, PCHAR szLine);
void MQFollowCommand(PSPAWNINFO pChar, PCHAR szLine);
void MQAdvPathCommand(PSPAWNINFO pChar, PCHAR szLine);
void ReleaseKeys();
void DoWalk(bool walk = false);
void DoFwd(bool hold, bool walk = false);
void DoBck(bool hold);
void DoLft(bool hold);
void DoRgt(bool hold);
void DoStop();
void LookAt(FLOAT X, FLOAT Y, FLOAT Z);
void NextPath();
void ClearAll();
void ClearOne(std::list<Position>::iterator &CurList);
void AddWaypoint(long SpawnID, bool Warping = false);
PDOOR ClosestDoor();
bool IsOpenDoor(PDOOR pDoor);
void OpenDoor(bool force = false);
bool InFront(float X, float Y, float Angle, bool Reverse = false);
void SavePath(const char* PathName, const char* PathZone);
void LoadPath(const char* PathName);
void FollowSpawn();
void FollowWaypoints();
void ClearLag();
void RecordingWaypoints();
void __stdcall WarpingDetect(unsigned int ID, void *pData, PBLECHVALUE pValues);
void ReplaceWaypointWith(const char* s);
void NextWaypoint();
void ShowHelp(int helpFlag);

std::vector<PMAPLINE>  pFollowPath;

unsigned long thisClock = clock();
unsigned long lastClock = clock();
long DistanceMod = 0;


///////////////////////////////////////////////////////////////////////////////
////
//
//      EVIL WINDOW CODE -- WORK IN PROGRESS
//
////
///////////////////////////////////////////////////////////////////////////////
////
//
//      Global Functions
//
////
///////////////////////////////////////////////////////////////////////////////

void WriteWindowINI(CSidlScreenWnd* pWindow);
void ReadWindowINI(CSidlScreenWnd* pWindow);
void DestroyMyWindow();
void CreateMyWindow();
void DoPlayWindow();

///////////////////////////////////////////////////////////////////////////////
////
//
//      Global Variables
//
////
///////////////////////////////////////////////////////////////////////////////

int iShowDoor = 1;              // AutoOpenDoor = true
int iShowLoop = 0;              // PlayLoop     = false
int iShowReverse = 0;           // PlayReverse  = false
int iShowSmart = 1;             // PlaySmart    = false
int iShowZone = 1;              // PlayZone     = false
int iShowEval = 1;              // PlayEval     = true
int iShowPause = 0;             // StatusState  = STATUS_OFF | STATUS_ON | STATUS_PAUSED
int iShowSlow = 0;              // PlaySlow     = false
int iShowStuck = 0;             // -- removed
int iShowBreakPath = 0;         // -- removed
int iShowBreakFollow = 0;       // -- removed

										///////////////////////////////////////////////////////////////////////////////
										////
										//
										//      Window Class - Uses custom XLM Window - MQUI_AdvPathWnd.xml
										//
										////
										///////////////////////////////////////////////////////////////////////////////


										// Window Declarations
class CMyWnd : public CCustomWnd
{
public:
	CCheckBoxWnd *APW_EvalChecked;
	CCheckBoxWnd *APW_LoopChecked;
	CCheckBoxWnd *APW_PauseChecked;
	CCheckBoxWnd *APW_ReverseChecked;
	CCheckBoxWnd *APW_SlowChecked;

	CCheckBoxWnd *APW_DoorChecked;
	CCheckBoxWnd *APW_SmartChecked;
	CCheckBoxWnd *APW_ZoneChecked;

	CCheckBoxWnd *APW_StuckChecked;
	CCheckBoxWnd *APW_BreakPathChecked;
	CCheckBoxWnd *APW_BreakFollowChecked;


	CButtonWnd   *APW_PlayButton;
	CButtonWnd   *APW_CancelButton;
	CButtonWnd   *APW_RecordButton;

	CEditWnd     *APW_EditBox;

	CListWnd	 *APW_PathList;
	CListWnd	 *APW_INIList;
	char		  szList[MAX_STRING];

	CMyWnd() : CCustomWnd("AdvPathWnd"), szList{ 0 }
	{
		APW_PlayButton = (CButtonWnd*)GetChildItem("APW_PlayButton");
		APW_CancelButton = (CButtonWnd*)GetChildItem("APW_CancelButton");
		APW_RecordButton = (CButtonWnd*)GetChildItem("APW_RecordButton");
		APW_EditBox = (CEditWnd*)GetChildItem("APW_EditBox");

		APW_PathList = (CListWnd*)GetChildItem("APW_PathList");
		APW_INIList = (CListWnd*)GetChildItem("APW_INIList");

		APW_EvalChecked = (CCheckBoxWnd*)GetChildItem("APW_EvalButton");
		APW_LoopChecked = (CCheckBoxWnd*)GetChildItem("APW_LoopButton");
		APW_PauseChecked = (CCheckBoxWnd*)GetChildItem("APW_PauseButton");
		APW_ReverseChecked = (CCheckBoxWnd*)GetChildItem("APW_ReverseButton");
		APW_SlowChecked = (CCheckBoxWnd*)GetChildItem("APW_SlowButton");

		APW_DoorChecked = (CCheckBoxWnd*)GetChildItem("APW_DoorButton");
		APW_SmartChecked = (CCheckBoxWnd*)GetChildItem("APW_SmartButton");
		APW_ZoneChecked = (CCheckBoxWnd*)GetChildItem("APW_ZoneButton");
		APW_StuckChecked = (CCheckBoxWnd*)GetChildItem("APW_StuckButton");
		APW_BreakPathChecked = (CCheckBoxWnd*)GetChildItem("APW_BreakPathButton");
		APW_BreakFollowChecked = (CCheckBoxWnd*)GetChildItem("APW_BreakFollowButton");
	}

	void SetCheckMarks(void)
	{
		APW_EvalChecked->bChecked = iShowEval ? true:false;
		APW_PauseChecked->bChecked = iShowPause ? true:false;
		APW_LoopChecked->bChecked = iShowLoop ? true:false;
		APW_ReverseChecked->bChecked = iShowReverse ? true:false;
		APW_SlowChecked->bChecked = iShowSlow ? true:false;
		APW_DoorChecked->bChecked = iShowDoor ? true:false;
		APW_SmartChecked->bChecked = iShowSmart ? true:false;
		APW_ZoneChecked->bChecked = iShowZone ? true:false;
		APW_StuckChecked->bChecked = iShowStuck ? true:false;
		APW_BreakPathChecked->bChecked = iShowBreakPath ? true:false;
		APW_BreakFollowChecked->bChecked = iShowBreakFollow ? true:false;
	}

	void GetCheckMarks(void)
	{
		iShowEval = APW_EvalChecked->bChecked;
		iShowPause = APW_PauseChecked->bChecked;
		iShowLoop = APW_LoopChecked->bChecked;
		iShowReverse = APW_ReverseChecked->bChecked;
		iShowSlow = APW_SlowChecked->bChecked;
		iShowDoor = APW_DoorChecked->bChecked;
		iShowSmart = APW_SmartChecked->bChecked;
		iShowStuck = APW_StuckChecked->bChecked;
		iShowZone = APW_ZoneChecked->bChecked;
		iShowBreakPath = APW_BreakPathChecked->bChecked;
		iShowBreakFollow = APW_BreakFollowChecked->bChecked;
	}


	void ShowWin(void)
	{
		((CXWnd*)this)->Show(1, 1);
		SetVisible(true);
	}

	void HideWin(void)
	{
		((CXWnd*)this)->Show(0, 0);
		SetVisible(false);
	}


	CXStr GetPathListItem()
	{
		return APW_PathList->GetItemText(APW_PathList->GetCurSel(), 0);
	}

	void AddINIListItem(const char* s)
	{
		APW_INIList->AddString(s, 0xFFFFFFFF, 0, 0);
	}

	CXStr GetINIListItem()
	{
		return APW_INIList->GetItemText(APW_INIList->GetCurSel(), 0);
	}

	void SetINIListItem(char *s, int m)
	{
		//CXStr Str;
		//		SetCXStr(&Str, s);
		//((CListWnd*)APW_INIList)->SetItemText(APW_INIList->GetCurSel(),0,&Str);
	}

	CXStr GetEditBoxItem()
	{
		return APW_EditBox->InputText;
	}


	void SetEditBoxItem(const char* text)
	{
		APW_EditBox->InputText = text;
	}


	void UpdateUI(void) { UpdateUI(0, 0, 0, 0, 0); }

	void UpdateUI(int waypoint, float x, float y, float z, char *checkpoint)
	{
		if (StatusState == STATUS_ON && FollowState == FOLLOW_RECORDING) // Recording a path
		{
			APW_PlayButton->SetWindowText("Save");
			//SetCXStr(&APW_PlayButton->WindowText, "Save");
			APW_RecordButton->SetWindowText("CheckPoint");
			//SetCXStr(&APW_RecordButton->WindowText, "CheckPoint");
		}
		else
		{
			APW_PlayButton->SetWindowText("Play");
			//SetCXStr(&APW_PlayButton->WindowText, "Play");
			APW_RecordButton->SetWindowText("Record");
			//SetCXStr(&APW_RecordButton->WindowText, "Record");
		}

		if (StatusState == STATUS_ON && FollowState == FOLLOW_PLAYING && checkpoint)  // Playing a path
		{
			char szTemp[MAX_STRING] = { 0 };
			sprintf_s(szTemp, "%d=%5.2f %5.2f %5.2f %s ", waypoint, y, x, z, checkpoint);
			APW_EditBox->InputText = szTemp;
			APW_INIList->SetCurSel(waypoint - 1);
			APW_INIList->EnsureVisible(waypoint - 1);
		}
		if (StatusState == STATUS_OFF)
			APW_EditBox->InputText.clear();
	}


	///////////////////////////////////////////////////////////////////////////////
	////
	//
	//      WndNotification - Major place for interactions
	//
	////
	///////////////////////////////////////////////////////////////////////////////

	int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown)
	{
		if (pWnd == 0) {
			if (Message == XWM_CLOSE) {
				CreateMyWindow();
				ShowWin();
				return 0;
			}
		}

		if ((pWnd == (CXWnd*)APW_StuckChecked) || (pWnd == (CXWnd*)APW_BreakPathChecked) || (pWnd == (CXWnd*)APW_BreakFollowChecked)) {
			return 0;
		}

		if (pWnd->GetType() == UI_Button) GetCheckMarks();

		if (pWnd == (CXWnd*)APW_EditBox  && Message == 6) // Pressed ENTER
		{
			ReplaceWaypointWith(GetEditBoxItem().c_str());
			auto temp = GetPathListItem();

			//			sprintf_s(INIFileNameTemp,"%s\\%s\\%s.ini",gszINIPath,"MQ2AdvPath",PathZone);
			//			sprintf_s(szTemp,"%.2f %.2f %.2f %s",CurList->Y,CurList->X,CurList->Z,CurList->CheckPoint);
			//			WritePrivateProfileString(PathName,szTemp2,      szTemp,INIFileNameTemp);

			///			SavePath(szTemp,GetShortZone(GetCharInfo()->zoneId));
			//			LoadPath(szTemp);
			return 0;
		}

		if (pWnd == (CXWnd*)APW_PathList && Message == 1)
		{
			auto temp = GetPathListItem();
			ClearAll();
			LoadPath(temp.c_str());
			return 0;
		}
		if (pWnd == (CXWnd*)APW_INIList && Message == 1)
		{
			CHAR szTemp[MAX_STRING] = { 0 };
			if (StatusState == STATUS_OFF) {
				APW_EditBox->InputText = GetINIListItem();
				PlayWaypoint = APW_INIList->GetCurSel() + 1;
				PlayOne = true;
				StatusState = STATUS_ON;
				FollowState = FOLLOW_PLAYING;
			}

			return 0;
		}


		if (pWnd == (CXWnd*)APW_PlayButton && Message == 1) {

			if (StatusState == STATUS_ON && FollowState == FOLLOW_RECORDING) // Recording a path
			{
				MQRecordCommand(nullptr, "save");
				MQPlayCommand(nullptr, "show");
				UpdateUI();
				return 0;
			}


			auto Buffer = GetPathListItem();
			if (Buffer[0] != 0)
			{
				char cmd[MAX_STRING] = { 0 };
				sprintf_s(cmd, "%s%s%s%s%s%s%s%s", Buffer.c_str(),
					(APW_DoorChecked->bChecked ? " Door" : " NoDoor"),
					(APW_LoopChecked->bChecked ? " Loop" : ""),
					(APW_ReverseChecked->bChecked ? " Reverse" : " Normal"),
					(APW_SmartChecked->bChecked ? " Smart" : ""),
					(APW_SlowChecked->bChecked ? " Slow" : " Fast"),
					(APW_EvalChecked->bChecked ? " Eval" : " NoEval"),
					(APW_ZoneChecked->bChecked ? " Zone" : ""));
				MQPlayCommand(nullptr, "off");
				MQPlayCommand(nullptr, cmd);
				WriteChatf("GUI cmd = [%s]", cmd);
			}
			UpdateUI();
			return 0;
		}

		if (pWnd == (CXWnd*)APW_CancelButton) {
			MQPlayCommand(nullptr, "off");
			UpdateUI();
			return 0;
		}

		if (pWnd == (CXWnd*)APW_RecordButton) {
			char cmd[MAX_STRING] = { 0 };

			auto temp = APW_EditBox->InputText;
			if (!temp.empty())
			{
				APW_EditBox->InputText.clear();
				if (StatusState == STATUS_OFF)
					MQRecordCommand(nullptr, temp.mutable_data());
				else if (FollowState == FOLLOW_RECORDING) // Recording a path
				{
					sprintf_s(cmd, "checkpoint %s", temp.c_str());
					MQRecordCommand(nullptr, cmd);
				}
			}
			UpdateUI();
			return 0;
		}


		return CSidlScreenWnd::WndNotification(pWnd, Message, unknown);
	};

};


///////////////////////////////////////////////////////////////////////////////
////
//
//      The rest of the non Window class routines.
//
////
///////////////////////////////////////////////////////////////////////////////


CMyWnd *MyWnd = 0;

void CreateMyWindow()
{
	DebugSpewAlways("MQ2MyWnd::CreateWindow()");
	if (MyWnd) return;
	//	if (MyWnd) DestroyMyWindow();
	if (pSidlMgr->FindScreenPieceTemplate("AdvPathWnd")) {
		MyWnd = new CMyWnd;
		if (MyWnd) {
			ReadWindowINI(MyWnd);
			WriteWindowINI(MyWnd);
			MyWnd->SetCheckMarks();
			MyWnd->HideWin();
		}
	}
}

void DestroyMyWindow()
{
	DebugSpewAlways("MQ2MyWnd::DestroyWindow()");
	if (MyWnd)
	{
		WriteWindowINI(MyWnd);
		delete MyWnd;
		MyWnd = 0;
	}
}

PLUGIN_API void OnCleanUI() {
	DestroyMyWindow(); 
}

PLUGIN_API void OnReloadUI() {
	if (gGameState == GAMESTATE_INGAME && pCharSpawn)
		CreateMyWindow();
}

void ReadWindowINI(CSidlScreenWnd* pWindow)
{
	pWindow->SetLocation({ (LONG)GetPrivateProfileInt("Settings", "ChatLeft", 164, INIFileName),
		(LONG)GetPrivateProfileInt("Settings", "ChatTop", 357, INIFileName),
		(LONG)GetPrivateProfileInt("Settings", "ChatRight", 375, INIFileName),
		(LONG)GetPrivateProfileInt("Settings", "ChatBottom", 620, INIFileName) });

	pWindow->SetLocked((GetPrivateProfileInt("Settings", "Locked", 0, INIFileName) ? true:false));
	pWindow->SetFades((GetPrivateProfileInt("Settings", "Fades", 1, INIFileName) ? true:false));
	pWindow->SetFadeDelay(GetPrivateProfileInt("Settings", "Delay", 2000, INIFileName));
	pWindow->SetFadeDuration(GetPrivateProfileInt("Settings", "Duration", 500, INIFileName));
	pWindow->SetAlpha(GetPrivateProfileInt("Settings", "Alpha", 255, INIFileName));
	pWindow->SetFadeToAlpha(GetPrivateProfileInt("Settings", "FadeToAlpha", 255, INIFileName));
	pWindow->SetBGType(GetPrivateProfileInt("Settings", "BGType", 1, INIFileName));
	ARGBCOLOR col = { 0 };
	col.A = GetPrivateProfileInt("Settings", "BGTint.alpha", 255, INIFileName);
	col.R = GetPrivateProfileInt("Settings", "BGTint.red", 0, INIFileName);
	col.G = GetPrivateProfileInt("Settings", "BGTint.green", 0, INIFileName);
	col.B = GetPrivateProfileInt("Settings", "BGTint.blue", 0, INIFileName);
	pWindow->SetBGColor(col.ARGB);
}

template <unsigned int _Size>LPSTR SafeItoa(int _Value,char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}

void WriteWindowINI(CSidlScreenWnd* pWindow)
{
	CHAR szTemp[MAX_STRING] = { 0 };
	if (pWindow->IsMinimized())
	{
		WritePrivateProfileString("Settings", "ChatTop", SafeItoa(pWindow->GetOldLocation().top, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatBottom", SafeItoa(pWindow->GetOldLocation().bottom, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatLeft", SafeItoa(pWindow->GetOldLocation().left, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatRight", SafeItoa(pWindow->GetOldLocation().right, szTemp, 10), INIFileName);
	}
	else
	{
		WritePrivateProfileString("Settings", "ChatTop", SafeItoa(pWindow->GetLocation().top, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatBottom", SafeItoa(pWindow->GetLocation().bottom, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatLeft", SafeItoa(pWindow->GetLocation().left, szTemp, 10), INIFileName);
		WritePrivateProfileString("Settings", "ChatRight", SafeItoa(pWindow->GetLocation().right, szTemp, 10), INIFileName);
	}
	WritePrivateProfileString("Settings", "Locked", SafeItoa(pWindow->IsLocked(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "Fades", SafeItoa(pWindow->GetFades(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "Delay", SafeItoa(pWindow->GetFadeDelay(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "Duration", SafeItoa(pWindow->GetFadeDuration(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "Alpha", SafeItoa(pWindow->GetAlpha(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "FadeToAlpha", SafeItoa(pWindow->GetFadeToAlpha(), szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "BGType", SafeItoa(pWindow->GetBGType(), szTemp, 10), INIFileName);
	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->GetBGColor();
	WritePrivateProfileString("Settings", "BGTint.alpha", SafeItoa(col.A, szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "BGTint.red", SafeItoa(col.R, szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "BGTint.green", SafeItoa(col.G, szTemp, 10), INIFileName);
	WritePrivateProfileString("Settings", "BGTint.blue", SafeItoa(col.B, szTemp, 10), INIFileName);
}

void DoPlayWindow()
{
	int i;
	char *p, *q;
	char INIFileNameTemp[MAX_PATH];

	CreateMyWindow();
	if (MyWnd) {
		MyWnd->APW_PathList->DeleteAll();

		sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));

		GetPrivateProfileString(nullptr, nullptr, nullptr, MyWnd->szList, MAX_STRING, INIFileNameTemp);
		i = 0;
		p = q = MyWnd->szList;
		while (*q != 0 && i<99)
		{
			if (*p == 0)
			{
				MyWnd->APW_PathList->AddString(q, 0xFFFFFFFF, 0, 0);
				q = p + 1;
			}
			p++;
		}

		MyWnd->ShowWin();
	}
}

void ReadOtherSettings()
{
	GetPrivateProfileString("Settings", "CustomSearch", "0", custSearch, 64, INIFileName);
}

void WriteOtherSettings()
{
	WritePrivateProfileString("Settings", "CustomSearch", custSearch, INIFileName);
}

///////////////////////////////////////////////////////////////////////////////
////
//
//      AdvPath Functions
//
////
///////////////////////////////////////////////////////////////////////////////

inline PMAPLINE InitLine() {
	typedef PMAPLINE(__cdecl *InitLineCALL) ();
	MQPlugin* pLook = GetPlugin("MQ2Map");
	if (pLook)
		if (InitLineCALL Request = (InitLineCALL)GetProcAddress(pLook->hModule, "MQ2MapAddLine"))
			return Request();
	return 0;
}

inline void DeleteLine(PMAPLINE pLine) {
	typedef VOID(__cdecl *DeleteLineCALL) (PMAPLINE);
	MQPlugin* pLook = GetPlugin("MQ2Map");
	if (pLook)
		if (DeleteLineCALL Request = (DeleteLineCALL)GetProcAddress(pLook->hModule, "MQ2MapDeleteLine"))
			Request(pLine);
}

void MapClear() {
	if (pFollowPath.size()) {
		for (unsigned long i = 0; i<(unsigned long)pFollowPath.size(); i++) DeleteLine(pFollowPath[i]);
		pFollowPath.clear();
	    SaveMapPath = false;
	}
}

void ListPaths()
{
	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
	CHAR szTemp[MAX_STRING] = { 0 };
	CHAR szList[MAX_STRING] = { 0 };

	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
	GetPrivateProfileString(nullptr, nullptr, nullptr, szList, MAX_STRING, INIFileNameTemp);
	sprintf_s(szTemp, "[MQ2AdvPath] Paths available in %s", GetShortZone(GetCharInfo()->zoneId));
	WriteChatColor(szTemp, CONCOLOR_LIGHTBLUE);

	char* p = (char*)szList;
	size_t length = 0;
	int nCnt = 0;
	while (*p)
	{
		length = strlen(p);
		nCnt++;
		sprintf_s(szTemp, "%u: %s", nCnt, p);
		WriteChatColor(szTemp, CONCOLOR_LIGHTBLUE);
		p += length;
		p++;
	}
	return;
}

int GetCustPaths(int kFlag)
{
	if (custSearch[0] == '\0')
		return 0;

	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
	CHAR szTemp[MAX_STRING] = { 0 };
	CHAR szList[MAX_STRING] = { 0 };

	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
	GetPrivateProfileString(nullptr, nullptr, nullptr, szList, MAX_STRING, INIFileNameTemp);

	char* p = (char*)szList;
	size_t length = 0;
	int kCnt = 0;
	while (*p)
	{
		length = strlen(p);
		if (!_strnicmp(p, custSearch, strlen(custSearch))) {
			kCnt++;
			if (kFlag) {
				sprintf_s(szTemp, "%u: %s", kCnt, p);
				WriteChatColor(szTemp, CONCOLOR_LIGHTBLUE);
			}
		}
		p += length;
		p++;
	}
	if (kFlag && !kCnt) {
		sprintf_s(szTemp, "No Custom Paths Found for: %s", custSearch);
		WriteChatColor(szTemp, CONCOLOR_LIGHTBLUE);
	}
	return kCnt;
}

/*
//	Ingame commands:
//	/afollow    # Follow's your Target
*/
void MQFollowCommand(PSPAWNINFO pChar, PCHAR szLine) {
	DebugSpewAlways("MQ2AdvPath::MQFollowCommand()");
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus) return;
	bool doFollow = false;
	bool doAutoOpenDoor = true;
	long MyTarget = (pTarget) ? ((long)((PSPAWNINFO)pTarget)->SpawnID) : 0;

	if (szLine[0] == 0) {
		if (MonitorID || FollowPath.size()) {
			ClearAll();
			return;
		}
		else {
			doFollow = true;
		}
	}
	else {
		long iParm = 0;
		do {
			GetArg(Buffer, szLine, ++iParm);
			if (Buffer[0] == 0) break;
			if (!_strnicmp(Buffer, "on", 2)) {
				doFollow = true;
			}
			else if (!_strnicmp(Buffer, "off", 3)) {
				ClearAll();
				return;
			}
			else if (!_strnicmp(Buffer, "pause", 5)) {
				WriteChatf("[MQ2AdvPath] Follow Paused");
				DoStop();
				StatusState = STATUS_PAUSED;
				return;
			}
			else if (!_strnicmp(Buffer, "unpause", 7)) {
				WriteChatf("[MQ2AdvPath] Follow UnPaused");
				StatusState = STATUS_ON;
				return;
			}
			else if (!_strnicmp(Buffer, "spawn", 5)) {
				GetArg(Buffer, szLine, ++iParm);
				MyTarget = atol(Buffer);
				doFollow = true;
			}
			else if (!_strnicmp(Buffer, "nodoor", 6)) {
				doAutoOpenDoor = false;
				doFollow = true;
			}
			else if (!_strnicmp(Buffer, "door", 4)) {
				doAutoOpenDoor = true;
				doFollow = true;
			}
			else if (!_strnicmp(Buffer, "help", 4)) {
				ShowHelp(4);
				return;
			}
			else {
				if (atol(Buffer)) {
					if (atol(Buffer) < 1) FollowSpawnDistance = 1;
					else FollowSpawnDistance = atol(Buffer);
				}
			}
		} while (true);
	}
	if (doFollow) {
		ClearAll();
		if (MyTarget == GetCharInfo()->pSpawn->SpawnID) MonitorID = 0;
		else MonitorID = MyTarget;

		if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID)) {
			AutoOpenDoor = doAutoOpenDoor;
			AddWaypoint(MonitorID);
			FollowState = FOLLOW_FOLLOWING;
			StatusState = STATUS_ON;
			WriteChatf("[MQ2AdvPath] Following %s", pSpawn->Name);

			MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
			MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
			MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
		}
	}
}

void MQPlayCommand(PSPAWNINFO pChar, PCHAR szLine) {
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus)
		return;
	DebugSpewAlways("MQ2AdvPath::MQPlayCommand()");
	bool doPlay = false;
	bool doPause = false;

	long iParm = 0;
	do {
		GetArg(Buffer, szLine, ++iParm);
		if (Buffer[0] == 0) break;

		if (!_strnicmp(Buffer, "off", 3)) {
			StopState = false;
			ClearAll();
			while (!PathList.empty())
				PathList.pop();
			return;
		}
		else if (!_strnicmp(Buffer, "stop", 4)) {
			StopState = true;
			ClearAll();
			StopState = false;
			while (!PathList.empty())
				PathList.pop();
		}
		else if (!_strnicmp(Buffer, "pause", 5)) {
			float v = 0;
			GetArg(Buffer, szLine, iParm + 1);
			if (Buffer[0] != 0)
				v = (float)atof(Buffer);
			doPause = true;
			if (v != 0) {
				iParm++;
				unPauseTick = GetTickCount64() + (long)v;
			}
			else {
				unPauseTick = 0;
			}
		}
		else if (!_strnicmp(Buffer, "unpause", 7)) {
			WriteChatf("[MQ2AdvPath] Playing UnPaused");
			StatusState = STATUS_ON;
		}
		else if (!_strnicmp(Buffer, "setflag", 7)) {
			int t1 = 0;
			if (Buffer[7] != '\0')
				t1 = Buffer[7] - '0';
			char v1 = '\0';
			GetArg(Buffer, szLine, iParm + 1);
			if (Buffer[0] != '\0')
				v1 = Buffer[0];
			if (v1 != '\0' && (int)strlen(Buffer) == 1)
				iParm++;
			if (t1>0 && t1<10)
				advFlag[t1] = v1;
		}
		else if (!_strnicmp(Buffer, "resetflags", 10)) {
			strcpy_s(advFlag, "yyyyyyyyyy");
			return;
		}
		else if (!_strnicmp(Buffer, "loop", 4))
			PlayLoop = true;
		else if (!_strnicmp(Buffer, "noloop", 6))
			PlayLoop = false;
		else if (!_strnicmp(Buffer, "reverse", 7))
			PlayDirection = -1;
		else if (!_strnicmp(Buffer, "normal", 6))
			PlayDirection = +1;
		else if (!_strnicmp(Buffer, "smart", 5))
			PlaySmart = 1;
		else if (!_strnicmp(Buffer, "nosmart", 7))
			PlaySmart = 0;
		else if (!_strnicmp(Buffer, "nodoor", 6))
			AutoOpenDoor = false;
		else if (!_strnicmp(Buffer, "door", 4))
			AutoOpenDoor = true;
		else if (!_strnicmp(Buffer, "slow", 4))
			PlaySlow = true;
		else if (!_strnicmp(Buffer, "fast", 4))
			PlaySlow = false;
		else if (!_strnicmp(Buffer, "eval", 4))
			PlayEval = true;
		else if (!_strnicmp(Buffer, "noeval", 6))
			PlayEval = false;
		else if (!_strnicmp(Buffer, "zone", 4))
			PlayZone = true;
		else if (!_strnicmp(Buffer, "nozone", 6))
			PlayZone = false;
		else if(!_strnicmp(Buffer,"savemap",7))
			SaveMapPath = true;
		else if (!_strnicmp(Buffer, "flee", 4)) {
			if (FollowPath.size()) {
				IFleeing = true;
				if (PlayDirection == +1) {
					PlayDirection = -1;
					NextWaypoint();
				}
			}
		}
		else if (!_strnicmp(Buffer, "noflee", 6)) {
			IFleeing = false;
			if (PlayDirection == -1) {
				PlayDirection = +1;
				if (FollowPath.size())
					NextWaypoint();
			}
		}
		else if (!_strnicmp(Buffer, "listcustom", 10)) {
			GetCustPaths(1);
			return;
		}
		else if (!_strnicmp(Buffer, "list", 4)) {
			ListPaths();
			return;
		}
		else if (!_strnicmp(Buffer, "show", 4)) {
			DoPlayWindow();
			return;
		}
		else if (!_strnicmp(Buffer, "help", 4)) {
			ShowHelp(1);
			return;
		}
		else {
			doPlay = true;
			PathList.push(Buffer);
		}
	} while (true);

	if (doPlay)
		NextPath();
	if (doPause) {
		WriteChatf("[MQ2AdvPath] Playing Paused");
		DoStop();
		StatusState = STATUS_PAUSED;
	}
}

void NextPath()
{
	std::string Buffer;
	if (PathList.empty()) return;
	Buffer = PathList.front();
	PathList.pop();
	if(!FollowPath.size() && (FollowState == FOLLOW_OFF || StopState)) {
		SaveMapPath=false;
		ClearAll();
		LoadPath((char *)Buffer.c_str());
		if (FollowPath.size()) {
			WriteChatf("[MQ2AdvPath] Playing Path: %s", Buffer.c_str());
			FollowState = FOLLOW_PLAYING;
			StatusState = STATUS_ON;
			StopState = false;
		} else {
			WriteChatf("[MQ2AdvPath] Playing Path: %s Failed",Buffer.c_str());
			if (StopState) {
				FollowState = FOLLOW_OFF;
				StopState = false;
		}
			return;
		}
	}
	if (PlaySmart) {
		auto CurList = FollowPath.begin();
		auto EndList = FollowPath.end();
		int i = 1;
		float D, MinD = 100000;
		while (CurList != EndList) {
			D = GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, CurList->X, CurList->Y, CurList->Z);
			if (D < MinD) {
				MinD = D;
				PlayWaypoint = i;
			}
			i++;
			CurList++;
		}
	}
	else if (PlayDirection == 1)
		PlayWaypoint = 1;
	else
		PlayWaypoint = FollowPath.size();
}

void MQRecordCommand(PSPAWNINFO pChar, PCHAR szLine) {
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus) return;
	DebugSpewAlways("MQ2AdvPath::MQPlayCommand()");

	long iParm = 0;
	CHAR Buffer1[MAX_STRING] = { 0 };
	GetArg(Buffer, szLine, ++iParm);

	if (!_strnicmp(Buffer, "help", 4)) {
		ShowHelp(2);
		return;
	}
	WriteChatf("MQ2AdvPath::MQPlayCommand(%s)", szLine);

	if (!_strnicmp(Buffer, "save", 4)) {
		if (FollowState == FOLLOW_RECORDING && StatusState) {
			GetArg(Buffer, szLine, ++iParm);
			GetArg(Buffer1, szLine, ++iParm);
			MaxChkPointDist = 0;

			if (Buffer[0] == 0) {
				if (SavePathName[0] != 0 && SavePathZone[0] != 0)
					SavePath(SavePathName, SavePathZone);
				return;
			}
			if (Buffer1[0] != 0 && IsNumber(Buffer1)) {
				MaxChkPointDist = atol(Buffer1);
				if(MaxChkPointDist<30)
					MaxChkPointDist = 30;
			}
			else if (Buffer1[0] == '\0' || !IsNumber(Buffer1)) {
				MaxChkPointDist = 40;
			}
			SavePath(Buffer, GetShortZone(GetCharInfo()->zoneId));
		}
		return;
	}
	else if (!_strnicmp(Buffer, "checkpoint", 10)) {
		GetArg(Buffer, szLine, ++iParm);
		if (Buffer[0] == 0) return;
		if (!FollowPath.empty() && FollowState == FOLLOW_RECORDING) {
			int i = 1;
			auto CurList = FollowPath.begin();
			auto EndList = FollowPath.end();
			while (CurList != EndList) {
				if (FollowPath.size() == i) {
					strcpy_s(CurList->CheckPoint, Buffer);
					return;
				}
				i++;
				CurList++;
			}
		}
	}
	else {
		ClearAll();
		//		GetArg(Buffer,szLine,++iParm);
		if (Buffer[0] == 0) {
			SavePathZone[0] = 0;
			SavePathName[0] = 0;
		}
		else {
			strcpy_s(SavePathName, Buffer);
			strcpy_s(SavePathZone, GetShortZone(GetCharInfo()->zoneId));
		}

		WriteChatf("[MQ2AdvPath] Recording Path: %s Zone: %s", SavePathName, SavePathZone);
		MonitorID = GetCharInfo()->pSpawn->SpawnID;
		AddWaypoint(MonitorID);
		FollowState = FOLLOW_RECORDING;
		StatusState = STATUS_ON;
		MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
		MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
		MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self
	}
}

//Movement Related Functions
void ReleaseKeys() {
	DoWalk(false);
	DoFwd(false);
	DoBck(false);
	DoRgt(false);
	DoLft(false);
}

void DoWalk(bool walk)
{
	if ((GetGameState() == GAMESTATE_INGAME) && pLocalPlayer) {
		const bool state_walking = !*EQADDR_RUNWALKSTATE;
		if (pLocalPlayer->SpeedMultiplier < 0)
		{
			walk = false; // we're snared, don't go into walk mode no matter what
		}
		if (walk != state_walking) {
			ExecuteCmd(FindMappableCommand("run_walk"), true, nullptr);
			ExecuteCmd(FindMappableCommand("run_walk"), false, nullptr);
		}
	}
}

void DoFwd(bool hold, bool walk) {
	static bool held = false;
	if (hold) {
		DoWalk(walk);
		DoBck(false);
		//if( !GetCharInfo()->pSpawn->SpeedRun || GetCharInfo()->pSpawn->PossiblyStuck || !((((gbMoving) && GetCharInfo()->pSpawn->SpeedRun==0.0f) && (GetCharInfo()->pSpawn->Mount ==  NULL )) || (fabs(FindSpeed((PSPAWNINFO)pCharSpawn)) > 0.0f )) )
		if (!GetCharInfo()->pSpawn->SpeedRun && held)
			held = false;
		if (!held) ExecuteCmd(FindMappableCommand("forward"), 1, 0);
		held = true;
	}
	else {
		DoWalk(false);
		if (held) ExecuteCmd(FindMappableCommand("forward"), 0, 0);
		held = false;
	}
}

void DoBck(bool hold) {
	static bool held = false;
	if (hold) {
		DoFwd(false);
		if (!held) ExecuteCmd(FindMappableCommand("back"), 1, 0);
		held = true;
	}
	else {
		if (held) ExecuteCmd(FindMappableCommand("back"), 0, 0);
		held = false;
	}
}

void DoLft(bool hold) {
	static bool held = false;
	if (hold) {
		DoRgt(false);
		if (!held) ExecuteCmd(FindMappableCommand("strafe_left"), 1, 0);
		held = true;
	}
	else {
		if (held) ExecuteCmd(FindMappableCommand("strafe_left"), 0, 0);
		held = false;
	}
}

void DoRgt(bool hold) {
	static bool held = false;
	if (hold) {
		DoLft(false);
		if (!held) ExecuteCmd(FindMappableCommand("strafe_right"), 1, 0);
		held = true;
	}
	else {
		if (held) ExecuteCmd(FindMappableCommand("strafe_right"), 0, 0);
		held = false;
	}
}

void DoStop() {
	if (!FollowIdle) FollowIdle = (long)clock();
	DoBck(true);
	ReleaseKeys();
}

void LookAt(FLOAT X, FLOAT Y, FLOAT Z) {
	if (PCHARINFO pChar = GetCharInfo())
	{
		if (pChar->pSpawn)
		{
			float angle = (atan2(X - pChar->pSpawn->X, Y - pChar->pSpawn->Y)  * 256.0f / (float)PI);
			if (angle >= 512.0f)
				angle -= 512.0f;
			if (angle < 0.0f)
				angle += 512.0f;
			((PSPAWNINFO)pCharSpawn)->Heading = (FLOAT)angle;
			gFaceAngle = 10000.0f;
			if (pChar->pSpawn->FeetWet) {
				float locdist = GetDistance(pChar->pSpawn->X, pChar->pSpawn->Y, X, Y);
				pChar->pSpawn->CameraAngle = (atan2(Z + 0.0f * 0.9f - pChar->pSpawn->Z - pChar->pSpawn->AvatarHeight * 0.9f, locdist) * 256.0f / (float)PI);
			}
			else if (pChar->pSpawn->mPlayerPhysicsClient.Levitate == 2) {
				if (Z < pChar->pSpawn->Z - 5)
					pChar->pSpawn->CameraAngle = -64.0f;
				else if (Z > pChar->pSpawn->Z + 5)
					pChar->pSpawn->CameraAngle = 64.0f;
				else
					pChar->pSpawn->CameraAngle = 0.0f;
			}
			else
				pChar->pSpawn->CameraAngle = 0.0f;
			gLookAngle = 10000.0f;
		}
	}
}

void ClearAll() {
	if (MonitorID || FollowPath.size()) WriteChatf("[MQ2AdvPath] Stopped");
	FollowPath.clear();
	unPauseTick = NextClickDoor = PauseDoor = FollowIdle = MonitorID = 0;
	DoStop();

	if (!StopState)
		FollowState = FOLLOW_OFF;		// Active?
	StatusState = STATUS_OFF;		// Active?
	PlayOne = MonitorWarp = PlayLoop = false;

	SavePathZone[0] = 0;
	SavePathName[0] = 0;

	if(!SaveMapPath)
		MapClear();
}

void ClearOne(std::list<Position>::iterator &CurList) {
	std::list<Position>::iterator PosList;
	PosList = CurList;
	CurList->X = 0;
	CurList->Y = 0;
	CurList->Z = 0;
	CurList++;
	FollowPath.erase(PosList);
}

void AddWaypoint(long SpawnID, bool Warping) {
	if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(SpawnID)) {
		Position MonitorPosition;

		MonitorPosition.Warping = Warping;

		MonitorPosition.X = MonitorX = pSpawn->X;
		MonitorPosition.Y = MonitorY = pSpawn->Y;
		MonitorPosition.Z = MonitorZ = pSpawn->Z;
		MonitorPosition.Heading = MonitorHeading = pSpawn->Heading;
		strcpy_s(MonitorPosition.CheckPoint, "");
		MonitorPosition.Eval = 0;

		FollowPath.push_back(MonitorPosition);
	}
}

PDOOR ClosestDoor() {
	PDOORTABLE pDoorTable = (PDOORTABLE)pSwitchMgr;
	FLOAT Distance = 100000.00;
	PDOOR pDoor = nullptr;
	for (int Count = 0; Count<pDoorTable->NumEntries; Count++) {
		if (Distance > GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ)) {
			Distance = GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoorTable->pDoor[Count]->DefaultX, pDoorTable->pDoor[Count]->DefaultY, pDoorTable->pDoor[Count]->DefaultZ);
			pDoor = pDoorTable->pDoor[Count];
		}
	}
	return pDoor;
}

bool IsOpenDoor(PDOOR pDoor) {
	//if(pDoor->DefaultHeading!=pDoor->Heading || pDoor->Y!=pDoor->DefaultY || pDoor->X!=pDoor->DefaultX  || pDoor->Z!=pDoor->DefaultZ )return true;
	if (pDoor->State == 1 || pDoor->State == 2)
		return true;
	return false;
}

void OpenDoor(bool force) {
	if (!AutoOpenDoor)
		return;
	if (force) {
		DoCommand(GetCharInfo()->pSpawn, "/click left door");
	}
	else if (PDOOR pDoor = (PDOOR)ClosestDoor()) {
		//DoCommand(GetCharInfo()->pSpawn,"/doortarget id ");
		if (GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) <  DISTANCE_OPEN_DOOR_CLOSE) {
			if (InFront(pDoor->X, pDoor->Y, ANGLE_OPEN_DOOR_CLOSE, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock()) {
				CHAR szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "id %d", pDoor->ID);
				DoorTarget(GetCharInfo()->pSpawn, szTemp);
				DoCommand(GetCharInfo()->pSpawn, "/click left door");
				NextClickDoor = (long)clock() + 100;
				if ((PauseDoor - (long)clock()) < 0) {
					PauseDoor = (long)clock() + 1000;
					DoStop();
				}
			}
		}
		else if (GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pDoor->DefaultX, pDoor->DefaultY, pDoor->DefaultZ) <  DISTANCE_OPEN_DOOR_LONG) {
			if (InFront(pDoor->X, pDoor->Y, ANGLE_OPEN_DOOR_LONG, false) && !IsOpenDoor(pDoor) && NextClickDoor < (long)clock()) {
				CHAR szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "id %d", pDoor->ID);
				DoorTarget(GetCharInfo()->pSpawn, szTemp);
				DoCommand(GetCharInfo()->pSpawn, "/click left door");
				NextClickDoor = (long)clock() + 100;
			}
		}
	}
}

bool InFront(float X, float Y, float AngleIn, bool Reverse) {
	float Angle = ((atan2f(X - GetCharInfo()->pSpawn->X, Y - GetCharInfo()->pSpawn->Y) * 180.0f / (float)PI));
	if (Angle<0)	Angle += 360;
	Angle = Angle*1.42f;

	if (Reverse) {
		if (Angle + 256 > 512) {
			Angle = Angle - 256;
		}
		else if (Angle - 256 < 0) {
			Angle = Angle + 256;
		}
	}

	bool Low = false;
	bool High = false;

	float Angle1 = GetCharInfo()->pSpawn->Heading - AngleIn;
	if (Angle1<0) {
		Low = true;
		Angle1 += 512.0f;
	}

	float Angle2 = GetCharInfo()->pSpawn->Heading + AngleIn;
	if (Angle2>512.0f) {
		High = true;
		Angle2 -= 512.0f;
	}

	if (Low) {
		if (Angle1 < (Angle + 512.0f) && Angle2 > Angle)
			return true;
	}
	else if (High) {
		if (Angle1 < Angle && Angle2 >(Angle - 512.0f))
			return true;
	}
	else if (Angle1 < Angle && Angle2 > Angle) {
		return true;
	}

	//	if( Angle1 < Angle && Angle2 > Angle ) return true;
	return false;
}

void SavePath(const char* PathName, const char* PathZone) {
	WriteChatf("[MQ2AdvPath] Saveing Path: %s Zone: %s", PathName, PathZone);
	CHAR INIFileNameTemp[400];
	CHAR szTemp[MAX_STRING] = { 0 };
	CHAR szTemp2[MAX_STRING] = { 0 };

	unsigned long thisWaypoint = 0;
	long CurrentDist = 0;
	FLOAT lastHeading = 0;
	FLOAT LastX = 0;
	FLOAT LastY = 0;
	FLOAT LastZ = 0;

	auto CurList = FollowPath.begin();
	auto EndList = FollowPath.end();

	if (EndList != CurList)
		EndList--;
	lastHeading = CurList->Heading;
	thisWaypoint = 1;
	while (CurList != EndList) {
		if (lastHeading == CurList->Heading && thisWaypoint > 1 && !CurList->Warping) {
			if (strlen(CurList->CheckPoint)>0) {
				CurrentDist = 0;
				LastX = CurList->X;
				LastY = CurList->Y;
				LastZ = CurList->Z;
				thisWaypoint++;
				CurList++;
				continue;
			}
			if (MaxChkPointDist) {
				//if(GetDistance3D(CurList->X,CurList->Y,CurList->Z,LastX,LastY,LastZ) > 100) {
				CurrentDist = CurrentDist + (long)GetDistance3D(CurList->X, CurList->Y, CurList->Z, LastX, LastY, LastZ);
				if (CurrentDist>MaxChkPointDist) {
					CurrentDist = 0;
					LastX = CurList->X;
					LastY = CurList->Y;
					LastZ = CurList->Z;
					thisWaypoint++;
					CurList++;
					continue;
				}
				else {
					LastX = CurList->X;
					LastY = CurList->Y;
					LastZ = CurList->Z;
				}
			}
			ClearOne(CurList);
		}
		else {
			lastHeading = CurList->Heading;
			CurrentDist = 0;
			LastX = CurList->X;
			LastY = CurList->Y;
			LastZ = CurList->Z;
			thisWaypoint++;
			CurList++;
		}
	}

	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, PathZone);
	if (FollowPath.size()) {
		WritePrivateProfileString(PathName, nullptr, nullptr, INIFileNameTemp);
		int i = 1;
		CurList = FollowPath.begin();
		EndList = FollowPath.end();
		while (CurList != EndList) {
			sprintf_s(szTemp2, "%d", i);
			sprintf_s(szTemp, "%.2f %.2f %.2f %s", CurList->Y, CurList->X, CurList->Z, CurList->CheckPoint);
			WritePrivateProfileString(PathName, szTemp2, szTemp, INIFileNameTemp);
			i++;
			CurList++;
		}
	}
	ClearAll();
}

void LoadPath(const char* PathName) {
	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));

	if (MyWnd) MyWnd->APW_INIList->DeleteAll();

	int i = 1;

	char szTemp[MAX_STRING] = { 0 };
	do {
		char szTemp2[MAX_STRING] = { 0 };
		char szTemp3[MAX_STRING] = { 0 };
		sprintf_s(szTemp, "%d", i);
		GetPrivateProfileString(PathName, szTemp, nullptr, szTemp2, MAX_STRING, INIFileNameTemp);
		if (szTemp2[0] == 0) break;
		Position TempPosition;

		GetArg(szTemp3, szTemp2, 1);
		TempPosition.Y = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, szTemp2, 2);
		TempPosition.X = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, szTemp2, 3);
		TempPosition.Z = (FLOAT)atof(szTemp3);

		strcpy_s(TempPosition.CheckPoint, GetNextArg(szTemp2, 3));


		TempPosition.Heading = 0;
		TempPosition.Warping = 0;
		TempPosition.Eval = 1;

		FollowPath.push_back(TempPosition);
		if (MyWnd)
		{
			sprintf_s(szTemp3, "%d=%s", i, szTemp2);
			MyWnd->APW_INIList->AddString(szTemp3, 0xFFFFFFFF, 0, 0);
		}
		i++;
	} while (true);
	if (MyWnd && !FollowPath.empty()) {
		for (i = 0;; i++) {
			MyWnd->APW_PathList->SetCurSel(i);
			if (ci_equals(MyWnd->GetPathListItem(), PathName) == 0)	break;
			if (szTemp[0] == 0)					break;
		}

	}
}

void FollowSpawn() {
	if (FollowPath.size()) {
		if (GetDistance3D(MeMonitorX, MeMonitorY, MeMonitorZ, GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z) > 50) {
			auto CurList = FollowPath.begin();
			auto EndList = FollowPath.end();
			do {
				if (CurList == EndList) break;
				if (CurList->Warping) break;
				ClearOne(CurList);
			} while (true);
			WriteChatf("[MQ2AdvPath] Warping Detected on SELF");
		}
		if (!FollowPath.size()) return;

		auto CurList = FollowPath.begin();
		auto EndList = FollowPath.end();

		bool run = false;

		if (CurList->Warping && GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, CurList->X, CurList->Y, CurList->Z) > 50) {
			if (!MonitorWarp) {
				WriteChatf("[MQ2AdvPath] Warping Wating");
				DoStop();
			}
			MonitorWarp = true;
			return;
		}
		MonitorWarp = false;

		if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID)) {
			if (GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) >= 50) run = true;
			if (GetDistance3D(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, GetCharInfo()->pSpawn->Z, pSpawn->X, pSpawn->Y, pSpawn->Z) <= FollowSpawnDistance) {
				DoStop();
				return;
			}
		}
		else if ((FollowPath.size()*DISTANCE_BETWEN_LOG) >= 20) run = true;
		//		 else if( GetDistance3D(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,GetCharInfo()->pSpawn->Z,CurList->X,CurList->Y,CurList->Z) >= 20 ) run = true;



		//		if( ( gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->X,CurList->Y) > DISTANCE_BETWEN_LOG ) || ( !gbInForeground && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->X,CurList->Y) > (DISTANCE_BETWEN_LOG+10) ) ) {
		if (GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->X, CurList->Y) > DISTANCE_BETWEN_LOG + DistanceMod) {
			LookAt(CurList->X, CurList->Y, CurList->Z);
			if ((PauseDoor - (long)clock()) < 300) {
				if (run) DoFwd(true);
				else DoFwd(true, true);
			}

			OpenDoor();
			FollowIdle = 0;
		}
		else {
			ClearOne(CurList);
			if (CurList != EndList) {
				if (CurList->Warping) return;
				// Clean up lag
				ClearLag();
			}
			else {
				DoStop();
				if (!MonitorID) {
					OpenDoor(true);
					ClearAll();
				}
			}
		}
	}
}

void ReplaceWaypointWith(const char* s)
{
	char mutableArg[MAX_STRING] = { 0 };
	strcpy_s(mutableArg, s);

	auto CurList = FollowPath.begin();
	auto EndList = FollowPath.end();

	int i = PlayWaypoint;
	while (CurList != EndList && i>0)
	{
		i--;
		CurList++;
	}
	if (i == 0)
	{
		char szTemp3[MAX_STRING];

		GetArg(szTemp3, mutableArg, 1);		CurList->Y = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, mutableArg, 2);		CurList->X = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, mutableArg, 3);		CurList->Z = (FLOAT)atof(szTemp3);

		strcpy_s(CurList->CheckPoint, GetNextArg(mutableArg, 3));
	}

}

void ResetPathEval()
{
	auto CurList = FollowPath.begin();
	auto EndList = FollowPath.end();
	while (CurList != EndList) {
		CurList->Eval = 1;
		CurList++;
	}
}

void EvalWaypoint(std::list<Position>::iterator &CurList)
{
	//WriteChatf("Checkpoint [%s]",CurList->CheckPoint);
	if ((PlayEval && (CurList->Eval || IFleeing)) && CurList->CheckPoint[0] == '/')
	{
		char szLine[MAX_STRING] = { 0 };
		strcpy_s(szLine, CurList->CheckPoint);
		//WriteChatf("Executing %s",szLine);
		HideDoCommand(((PSPAWNINFO)pCharSpawn), szLine, FALSE);
		/*
		if (_strnicmp(CurList->CheckPoint,"/play",5)==0)
		{
		char szLine[MAX_STRING];
		strcpy_s(szLine,CurList->CheckPoint);
		//MQPlayCommand(((PSPAWNINFO)pCharSpawn),szLine);
		HideDoCommand(((PSPAWNINFO)pCharSpawn),szLine,FALSE);
		}
		else
		EzCommand(CurList->CheckPoint);
		*/
		CurList->Eval = 0;
	}
}

void NextWaypoint()
{
	if (PlayOne)
	{
		PlayOne = false;
		DoStop();
		StatusState = STATUS_OFF;
		return;
	}
	// Check for End conditions .

	if ((PlayDirection == 1 && FollowPath.size() == PlayWaypoint) || (PlayDirection == -1 && PlayWaypoint == 1))
	{
		if (PlayLoop && PlayDirection == 1)
			PlayWaypoint = 1;
		else if (PlayLoop && PlayDirection == -1)
			PlayWaypoint = FollowPath.size();
		else if (PullMode && PlayDirection == 1 && FollowPath.end()->CheckPoint[0] != '/') {
			PlayDirection = -1;
			PlayWaypoint += PlayDirection;
		}
		else
		{
			ClearAll();
			if (PlayZone)
				PlayZoneTick = GetTickCount64() + ZONE_TIME;
			else
				NextPath();
			return;
		}
		ResetPathEval();
	}
	else if (gbInForeground || PlaySlow || PlayEval)
	{
		PlayWaypoint += PlayDirection;
	}
	else
	{
		PlayWaypoint += PlayDirection * 3;			// Check we don't advance past either end.
		if ((long)FollowPath.size() < PlayWaypoint) PlayWaypoint = FollowPath.size();
		if (PlayWaypoint < 1) 						PlayWaypoint = 1;
	}
}


void FollowWaypoints() {
	static WORD tock;
	float  d;
	if (!FollowPath.size()) return;

	auto CurList = FollowPath.begin();
	auto EndList = FollowPath.end();

	long WaypointIndex = 1;
	do {
		if (CurList == EndList) break;
		if (WaypointIndex == PlayWaypoint) {
			//				if( ( GetForegroundWindow()==EQhWnd && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->X,CurList->Y) > DISTANCE_BETWEN_LOG ) || ( GetForegroundWindow()!=EQhWnd && GetDistance(GetCharInfo()->pSpawn->X,GetCharInfo()->pSpawn->Y,CurList->X,CurList->Y) > (DISTANCE_BETWEN_LOG+10) ) )

			// Allow X,Y,Z = 0 to match current position
			if (CurList->X == 0 && CurList->Y == 0 && CurList->Z == 0)
				d = 0;
			else
				d = GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->X, CurList->Y);

			if (d > DISTANCE_BETWEN_LOG + DistanceMod) {
				LookAt(CurList->X, CurList->Y, CurList->Z);
				tock = (tock + 1) % 10;
				if (!PlaySlow || (PlaySlow && tock > 2))
				{
					if ((PauseDoor - (long)clock()) < 300) DoFwd(true, false);
				}
				else
					ReleaseKeys();
				OpenDoor();
				FollowIdle = 0;
				if (MyWnd) MyWnd->UpdateUI(PlayWaypoint, CurList->X, CurList->Y, CurList->Z, CurList->CheckPoint);
				break;
			}
			else {
				EvalWaypoint(CurList);
				NextWaypoint();
				break;
			}
		}
		WaypointIndex++;
		CurList++;
	} while (true);
}

void ClearLag() {
	if (FollowPath.size()) {
		auto CurList = FollowPath.begin();
		auto EndList = FollowPath.end();

		if (CurList != EndList) {
			if (CurList->Warping) return;
			if (InFront(CurList->X, CurList->Y, 100, true)) {
				CurList++;
				for (int LagCount = 0; LagCount<15; LagCount++) {
					if (CurList != EndList) {
						if ((InFront(CurList->X, CurList->Y, 100, false) && LagCount < 10) || (InFront(CurList->X, CurList->Y, 50, false) && LagCount >= 10)) {
							DebugSpewAlways("MQ2AdvPath::Removing lag() %d", LagCount);
							CurList = FollowPath.begin();
							for (int LagCount2 = 0; LagCount2 <= LagCount; LagCount2++) ClearOne(CurList);
							return;
						}
					}
					else {
						return;
					}
					CurList++;
				}
			}
		}
	}
}

void RecordingWaypoints() {
	if (MonitorID) {
		if (PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(MonitorID)) {
			if (GetDistance3D(MonitorX, MonitorY, MonitorZ, pSpawn->X, pSpawn->Y, pSpawn->Z) > 100) {
				WriteChatf("[MQ2AdvPath] Warping Detected on %s", pSpawn->Name);
				AddWaypoint(MonitorID, true);
			}
			else if (GetDistance3D(MonitorX, MonitorY, MonitorZ, pSpawn->X, pSpawn->Y, pSpawn->Z) >= DISTANCE_BETWEN_LOG) AddWaypoint(MonitorID);
		}
	}
}

void FillInPath(char *PathName, std::list<Position>&thepath, std::map<std::string, std::list<Position>>&themap)
{
	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
	char szTemp[MAX_STRING] = { 0 };
	int i = 1;
	do {
		char szTemp2[MAX_STRING] = { 0 };
		char szTemp3[MAX_STRING] = { 0 };
		sprintf_s(szTemp, "%d", i);
		GetPrivateProfileString(PathName, szTemp, nullptr, szTemp2, MAX_STRING, INIFileNameTemp);
		if (szTemp2[0] == 0) break;
		Position TempPosition;

		GetArg(szTemp3, szTemp2, 1);
		TempPosition.Y = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, szTemp2, 2);
		TempPosition.X = (FLOAT)atof(szTemp3);
		GetArg(szTemp3, szTemp2, 3);
		TempPosition.Z = (FLOAT)atof(szTemp3);

		strcpy_s(TempPosition.CheckPoint, GetNextArg(szTemp2, 3));


		TempPosition.Heading = 0;
		TempPosition.Warping = 0;
		TempPosition.Eval = 1;

		thepath.push_back(TempPosition);
		i++;
	} while (true);
	themap[PathName] = thepath;
}

std::string GetPathWeAreOn(char *PathName)
{
	std::list<Position>thepath;
	std::map<std::string, std::list<Position>>themap;
	CHAR INIFileNameTemp[MAX_STRING] = { 0 };
	sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
	//first get all sections in the ini
	HANDLE fHandle = 0;
	bool bDoAll = false;
	if (!_stricmp(PathName, "*")) {
		bDoAll = true;
	}

	if ((fHandle = CreateFile(INIFileNameTemp, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr)) != INVALID_HANDLE_VALUE) {
		DWORD hsize = 0;
		if (DWORD fsize = GetFileSize(fHandle, &hsize)) {
			if (char *szBuffer = (char *)LocalAlloc(LPTR, fsize)) {
				char *szOrgBuff = szBuffer;
				if (DWORD thesize = GetPrivateProfileString(nullptr, nullptr, nullptr, szBuffer, fsize, INIFileNameTemp)) {
					//we got some paths...
					size_t newsize = 0;
					CHAR szTemp[MAX_STRING] = { 0 };
					while (newsize <= thesize) {
						strcpy_s(szTemp, szBuffer);
						if (!bDoAll) {
							if (!_strnicmp(szTemp, PathName, strlen(PathName))) {
								FillInPath(szTemp, thepath, themap);
							}
						}
						else {
							FillInPath(szTemp, thepath, themap);
						}
						newsize += strlen(szBuffer);
						if (newsize<thesize) {
							szBuffer += strlen(szBuffer) + 1;
							if (szBuffer[0] == '\0') {
								break;
							}
						}
					}
				}
				LocalFree(szOrgBuff);
			}
		}
		CloseHandle(fHandle);
	}
	if (themap.size() && pLocalPlayer) {
		struct _ShortDist
		{
			FLOAT Shortestdist;
			std::string Name;
		} ShortDist{ 0, "" };
		ShortDist.Shortestdist = 1000000.0;
		PSPAWNINFO pLPlayer = (PSPAWNINFO)pLocalPlayer;
		for (std::map<std::string, std::list<Position>>::iterator m = themap.begin(); m != themap.end(); m++) {
			for (std::list<Position>::iterator l = m->second.begin(); l != m->second.end(); l++) {
				FLOAT fDist3D = GetDistance3D(pLPlayer->X, pLPlayer->Y, pLPlayer->Z, (*l).X, (*l).Y, (*l).Z);
				if (fDist3D<ShortDist.Shortestdist) {
					ShortDist.Shortestdist = fDist3D;
					ShortDist.Name = m->first;
				}
			}
		}
		return ShortDist.Name;
	}
	return "";
}

class MQ2AdvPathType : public MQ2Type {
	char Temps[MAX_STRING];
public:
	enum AdvPathMembers {
		Active = 1,
		State = 2,
		Waypoints = 3,
		NextWaypoint = 4,
		Y = 5,
		X = 6,
		Z = 7,
		Monitor = 8,
		Idle = 9,
		Length = 10,
		Following = 11,
		Playing = 12,
		Recording = 13,
		Status = 14,
		Paused = 15,
		WaitingWarp = 16,
		Path = 17,
		PathList = 18,
		PathExists=19,
		CheckPoint=20,
		CustomPaths=21,
		Pulling=22,
		Direction=23,
		Fleeing=24,
		Flag1=25,
		Flag2=26,
		Flag3=27,
		Flag4=28,
		Flag5=29,
		Flag6=30,
		Flag7=31,
		Flag8=32,
		Flag9=33,
		CustomSearch=34,
		PathCount=35,
		FirstWayPoint=36
	};
	MQ2AdvPathType() : MQ2Type("AdvPath"), Temps{ 0 } {
		TypeMember(Active);
		TypeMember(State);
		TypeMember(Waypoints);
		TypeMember(NextWaypoint);
		TypeMember(Y);
		TypeMember(X);
		TypeMember(Z);
		TypeMember(Monitor);
		TypeMember(Idle);
		TypeMember(Length);
		TypeMember(Following);
		TypeMember(Playing);
		TypeMember(Recording);
		TypeMember(Status);
		TypeMember(Paused);
		TypeMember(WaitingWarp);
		TypeMember(Path);
		TypeMember(PathList);
		TypeMember(PathExists);
		TypeMember(CheckPoint);
		TypeMember(CustomPaths);
		TypeMember(Pulling);
		TypeMember(Direction);
		TypeMember(Fleeing);
		TypeMember(Flag1);
		TypeMember(Flag2);
		TypeMember(Flag3);
		TypeMember(Flag4);
		TypeMember(Flag5);
		TypeMember(Flag6);
		TypeMember(Flag7);
		TypeMember(Flag8);
		TypeMember(Flag9);
		TypeMember(CustomSearch);
		TypeMember(PathCount);
		TypeMember(FirstWayPoint);
	}

	virtual bool MQ2AdvPathType::GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override {
		auto CurList = FollowPath.begin();
		auto EndList = FollowPath.end();
		int i = 1;
		float TheLength = 0;

		//WriteChatf("[MQ2AdvPath] AdvPath Member: %s %s",Member,Index);

		MQTypeMember* pMember = MQ2AdvPathType::FindMember(Member);
		if (pMember) switch ((AdvPathMembers)pMember->ID) {
		case Active:										// Plugin on and Ready
			Dest.DWord = (gbInZone && GetCharInfo() && GetCharInfo()->pSpawn && AdvPathStatus);
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case State:											// FollowState, 0 = off, 1 = Following, 2 = Playing, 3 = Recording
			Dest.DWord = FollowState;
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Waypoints:										// Number of Waypoints
			Dest.DWord = FollowPath.size();
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case NextWaypoint:									// Next Waypoint
			Dest.DWord = PlayWaypoint;
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Y:												// Waypoint Y
			while (CurList != EndList) {
				if (i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint))) {
					Dest.Float = CurList->Y;
					Dest.Type = mq::datatypes::pFloatType;
					return true;
				}
				i++;
				CurList++;
			}
			strcpy_s(Temps, "NULL");
			Dest.Type = mq::datatypes::pStringType;
			Dest.Ptr = Temps;
			return true;
		case X:												// Waypoint X
			while (CurList != EndList) {
				if (i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint))) {
					Dest.Float = CurList->X;
					Dest.Type = mq::datatypes::pFloatType;
					return true;
				}
				i++;
				CurList++;
			}
			strcpy_s(Temps, "NULL");
			Dest.Type = mq::datatypes::pStringType;
			Dest.Ptr = Temps;
			return true;
		case Z:												// Waypoint Z
			while (CurList != EndList) {
				if (i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint))) {
					Dest.Float = CurList->Z;
					Dest.Type = mq::datatypes::pFloatType;
					return true;
				}
				i++;
				CurList++;
			}
			strcpy_s(Temps, "NULL");
			Dest.Type = mq::datatypes::pStringType;
			Dest.Ptr = Temps;
			return true;
		case Monitor:										// Spawn your following
			Dest.Ptr = (PSPAWNINFO)GetSpawnByID(MonitorID);
			Dest.Type = mq::datatypes::pSpawnType;
			return true;
		case Idle:											// FollowIdle time when following and not moving
			Dest.DWord = (FollowState && FollowIdle) ? (((long)clock() - FollowIdle) / 1000) : 0;
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Length:										// Estimated length off the follow path
			if (FollowPath.size()) {
				CurList = FollowPath.begin();
				TheLength = GetDistance(GetCharInfo()->pSpawn->X, GetCharInfo()->pSpawn->Y, CurList->X, CurList->Y);
				if (FollowPath.size() > 1)	TheLength = ((FollowPath.size() - 1)*DISTANCE_BETWEN_LOG) + TheLength;
			}
			Dest.Float = TheLength;
			Dest.Type = mq::datatypes::pFloatType;
			return true;
		case Following:
			Dest.DWord = (FollowState == FOLLOW_FOLLOWING);
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Playing:
			Dest.DWord = (FollowState == FOLLOW_PLAYING);
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Recording:
			Dest.DWord = (FollowState == FOLLOW_RECORDING);
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Status:
			Dest.DWord = StatusState;
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Paused:
			Dest.DWord = (StatusState == STATUS_PAUSED);
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case WaitingWarp:
			Dest.DWord = MonitorWarp;
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Pulling:
			Dest.DWord = PullMode;
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Fleeing:
			Dest.DWord = IFleeing;
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		case Flag1:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[1];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag2:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[2];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag3:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[3];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag4:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[4];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag5:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[5];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag6:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[6];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag7:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[7];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag8:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[8];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Flag9:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = advFlag[9];
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case Direction:
			Dest.Type = mq::datatypes::pStringType;
			Temps[0] = PlayDirection == 1 ? 'N' : 'R';
			Temps[1] = '\0';
			Dest.Ptr = Temps;
			return true;
		case CustomSearch:
			Dest.Type = mq::datatypes::pStringType;
			strcpy_s(Temps, custSearch);
			Dest.Ptr = Temps;
			return true;
		case CheckPoint:
			while (CurList != EndList) {
				if (i == atol(Index) || (Index[0] != 0 && !_stricmp(Index, CurList->CheckPoint))) {
					if (CurList->CheckPoint[0] == '/') {
						strcpy_s(Temps, "Check Point exists as MQ2Command");
					}
					else {
						strcpy_s(Temps, CurList->CheckPoint);
					}
					Dest.Ptr = Temps;
					Dest.Type = mq::datatypes::pStringType;
					return true;
				}
				i++;
				CurList++;
			}
			strcpy_s(Temps, "NULL");
			Dest.Type = mq::datatypes::pStringType;
			Dest.Ptr = Temps;
			return true;
		case CustomPaths:
			Dest.DWord = GetCustPaths(0);
			Dest.Type = mq::datatypes::pIntType;
			return true;
		case Path:
		{
			if (Index[0] != '\0') {
				std::string sPath = GetPathWeAreOn(Index);
				if (sPath.size()) {
					strcpy_s(DataTypeTemp, sPath.c_str());
					Dest.Ptr = &DataTypeTemp[0];
					Dest.Type = mq::datatypes::pStringType;
					return true;
				}
			}
		}
		case PathList:
		case PathExists:
		{
			if (Index[0]) {
				if (IsNumber(Index)) {
					Dest.DWord = false;
					Dest.Type = mq::datatypes::pBoolType;
					return true;
				}
				CHAR INIFileNameTemp[MAX_STRING] = { 0 };
				sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
				GetPrivateProfileString(Index, "1", "", Temps, MAX_STRING, INIFileNameTemp);
				if (!Temps[0]) {
					Dest.DWord = false;
				}
				else {
					Dest.DWord = true;
				}
				Dest.Type = mq::datatypes::pBoolType;
				return true;
			}
			Dest.DWord = false;
			Dest.Type = mq::datatypes::pBoolType;
			return true;
		}
		case PathCount:
		{
			CHAR INIFileNameTemp[MAX_STRING] = { 0 };
            sprintf_s(INIFileNameTemp, "%s\\%s\\%s.ini", gPathConfig, "MQ2AdvPath", GetShortZone(GetCharInfo()->zoneId));
	        //GetPrivateProfileString("","","",Temps,MAX_STRING,INIFileNameTemp);
			DWORD count1 = GetPrivateProfileSectionNames(Temps, MAX_STRING, INIFileNameTemp);
			if (count1) {
				int tot1 = 0;
				char * pch1;
				pch1 = strchr(Temps,'\0');
				while (pch1 != nullptr && pch1 < Temps+count1)
				{
					tot1++;
					pch1=strchr(pch1+1, '\0');
				}
				//
				//for(int idx1 = 1; idx1<=count1; idx1++)
				//{
				//	  if(Temps[idx1]=='\0') tot1++;
				//
				//}
				Dest.DWord=tot1;
			} else {
                Dest.DWord=0;
			}
                Dest.Type=mq::datatypes::pIntType;
                return true;
		}
		case FirstWayPoint:
		{
			if (Index && Index[0]!='\0') {
				if (IsNumber(Index)) {
					strcpy_s(Temps,"NULL");
					Dest.Type=mq::datatypes::pStringType;
					Dest.Ptr=Temps;
					return true;
                }
				CHAR INIFileNameTemp[MAX_STRING] = { 0 };
                sprintf_s(INIFileNameTemp, "%s\\MQ2AdvPath\\%s.ini", gPathConfig, GetShortZone(GetCharInfo()->zoneId));
	            GetPrivateProfileString(Index,"1","",Temps,MAX_STRING,INIFileNameTemp);
				if(Temps && Temps[0]=='\0') {
				    strcpy_s(Temps,"NULL");
				}
				Dest.Ptr=Temps;
                Dest.Type=mq::datatypes::pStringType;
                return true;
            }
            Dest.DWord=false;
            Dest.Type=mq::datatypes::pBoolType;
            return true;
		}
		}
		strcpy_s(DataTypeTemp, "NULL");
		Dest.Type = mq::datatypes::pStringType;
		Dest.Ptr = &DataTypeTemp[0];
		return true;
	}
	bool ToString(MQVarPtr VarPtr, PCHAR Destination) override {
		strcpy_s(Destination, MAX_STRING, "TRUE");
		return true;
	}
};

bool dataAdvPath(const char* szName, MQTypeVar &Dest) {
	Dest.DWord = 1;
	Dest.Type = pAdvPathType;
	return true;
}

void ShowHelp(int helpFlag) {
	WriteChatColor("========= Advance Pathing Help =========", CONCOLOR_YELLOW);
	//WriteChatf("");
	switch (helpFlag)
	{
	case 1:
		WriteChatColor("/play [pathName|off] [stop] [pause|unpause] [loop|noloop] [normal|reverse] [smart|nosmart] [flee|noflee] [door|nodoor] [fast|slow] [eval|noeval] [zone|nozone] [list] [listcustom] [show] [help] [setflag1]-[setflag9] y/n [resetflags]", CONCOLOR_GREEN);
		WriteChatColor(" The /play command will execute each command on the line. The commands that should be used single, or at the end of a line:", CONCOLOR_GREEN);
		WriteChatColor("   [off] [list] [listcustom] [show] [help]", CONCOLOR_GREEN);
		WriteChatf("");
		WriteChatColor("setflag1-setflag9 can be used anywhere in the line and uses the following format:", CONCOLOR_GREEN);
		WriteChatColor(" /play setflag1 n pathname or /play pathname setflag n (n can be any single alpha character)", CONCOLOR_GREEN);
		WriteChatColor(" AdvPath flags can be accessed using ${AdvPath.Flag1} - ${AdvPath.Flag9}", CONCOLOR_GREEN);
		WriteChatColor(" /play resetflags resets all flags(1-9) to 'y'", CONCOLOR_GREEN);
		break;
	case 2:
		WriteChatColor("/record ", CONCOLOR_GREEN);
		WriteChatColor("/record save <PathName> ##", CONCOLOR_GREEN);
		WriteChatColor("/record Checkpoint <checkpointname>", CONCOLOR_GREEN);
		WriteChatColor("/record help", CONCOLOR_GREEN);
		WriteChatColor("## is the distance between checkpoints to force checkpoints to be writen to the path file", CONCOLOR_GREEN);
		break;
	case 3:
		WriteChatColor("/advpath [on|off] [pull|nopull] [customsearch value] [save] [help]", CONCOLOR_GREEN);
		WriteChatColor("  For Addional help use:", CONCOLOR_YELLOW);
		WriteChatColor("    /play help:", CONCOLOR_GREEN);
		WriteChatColor("    /record help", CONCOLOR_GREEN);
		WriteChatColor("    /afollow help:", CONCOLOR_GREEN);
		break;
	case 4:
		WriteChatColor("/afollow [on|off] [pause|unpause] [slow|fast] ", CONCOLOR_GREEN);
		WriteChatColor("/afollow spawn # [slow|fast] - default=fast", CONCOLOR_GREEN);
		WriteChatColor("/afollow help", CONCOLOR_GREEN);
	}
	//WriteChatf("");
	WriteChatColor("========================================", CONCOLOR_YELLOW);
}
// Set Active Status true or false.
void MQAdvPathCommand(PSPAWNINFO pChar, PCHAR szLine) {
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn) return;
	DebugSpewAlways("MQ2AdvPath::MQAdvPathCommand()");
	long iParm = 0;
	do {
		GetArg(Buffer, szLine, ++iParm);
		if (Buffer[0] == 0) break;

		if (!_strnicmp(Buffer, "off", 3)) {
			ClearAll();
			while (!PathList.empty())
				PathList.pop();
			AdvPathStatus = false;
		}
		else if (!_strnicmp(Buffer, "on", 2))
			AdvPathStatus = true;
		else if (!_strnicmp(Buffer, "pull", 4))
			PullMode = true;
		else if (!_strnicmp(Buffer, "nopull", 6))
			PullMode = false;
		else if (!_strnicmp(Buffer, "flee", 4)) {
			if (FollowPath.size()) {
				IFleeing = true;
				if (PlayDirection == +1) {
					PlayDirection = -1;
					NextWaypoint();
				}
			}
		}
		else if (!_strnicmp(Buffer, "noflee", 6)) {
			IFleeing = false;
			if (PlayDirection == -1) {
				PlayDirection = +1;
				if (FollowPath.size())
					NextWaypoint();
			}
		}
		else if (!_strnicmp(Buffer, "customsearch", 12)) {
			GetArg(Buffer, szLine, iParm + 1);
			if (Buffer[0] != '\0') {
				strcpy_s(custSearch, Buffer);
			}
			break;
		}
		else if (!_strnicmp(Buffer, "save", 4)) {
			WriteOtherSettings();
			break;
		}
		else if (!_strnicmp(Buffer, "help", 4))
			ShowHelp(3);
		break;
	} while (true);
}

// Called once, when the plugin is to initialize
PLUGIN_API void InitializePlugin() {
	DebugSpewAlways("Initializing MQ2AdvPath");

	AdvPathStatus = true;

	AddCommand("/afollow", MQFollowCommand);
	AddCommand("/play", MQPlayCommand);
	AddCommand("/record", MQRecordCommand);
	AddCommand("/arecord", MQRecordCommand);
	AddCommand("/advpath", MQAdvPathCommand);
	pAdvPathType = new MQ2AdvPathType;
	AddMQ2Data("AdvPath", dataAdvPath);
	sprintf_s(Buffer, "%s\\MQ2AdvPath", gPathConfig);
	if (_mkdir(Buffer) != 0)
		WriteChatf("[MQ2AdvPath] Failed to create MQ2AdvPath directory");
	AddXMLFile("MQUI_AdvPathWnd.xml");
	ReadOtherSettings();
}

// Called once, when the plugin is to shutdown
PLUGIN_API void ShutdownPlugin() {
	DebugSpewAlways("Shutting down MQ2AdvPath");

	DestroyMyWindow();
	RemoveCommand("/afollow");
	RemoveCommand("/play");
	RemoveCommand("/record");
	RemoveCommand("/arecord");
	RemoveCommand("/advpath");
	delete pAdvPathType;
	RemoveMQ2Data("AdvPath");
	ClearAll();
}

// This is called every time MQ pulses
PLUGIN_API void OnPulse() {
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus) return;

	thisClock = (unsigned long)clock();
	DistanceMod = ((thisClock - lastClock + 15)*(thisClock - lastClock + 15)) / 1000;
	lastClock = (unsigned long)clock();

	RecordingWaypoints();
	if (FollowState == FOLLOW_FOLLOWING && StatusState == STATUS_ON) FollowSpawn();
	else if (FollowState == FOLLOW_PLAYING && StatusState == STATUS_ON) FollowWaypoints();

	if (PlayZoneTick && PlayZoneTick < GetTickCount64() && PlayZone)
	{
		NextPath();
		PlayZoneTick = 0;
	}

	if (unPauseTick && GetTickCount64() > unPauseTick) {
		unPauseTick = 0;
		if (FollowState == FOLLOW_PLAYING && StatusState == STATUS_PAUSED)
		{
			WriteChatf("[MQ2AdvPath] Playing UnPaused");
			StatusState = STATUS_ON;
		}

	}

	MeMonitorX = GetCharInfo()->pSpawn->X; // MeMonitorX monitor your self
	MeMonitorY = GetCharInfo()->pSpawn->Y; // MeMonitorY monitor your self
	MeMonitorZ = GetCharInfo()->pSpawn->Z; // MeMonitorZ monitor your self

	if (FollowPath.size()) {
		auto CurList = FollowPath.begin();
		auto EndList = FollowPath.end();

		Position LastList;			// FollowPath

		if (FollowState == FOLLOW_FOLLOWING) {
			LastList.Z = GetCharInfo()->pSpawn->Z;
			LastList.Y = GetCharInfo()->pSpawn->Y;
			LastList.X = GetCharInfo()->pSpawn->X;
		}
		else {
			LastList.Z = CurList->Z;
			LastList.Y = CurList->Y;
			LastList.X = CurList->X;
		}
		MapClear();
		while (CurList != EndList) {
			if (CurList->X != 0 && CurList->Y != 0 && CurList->Z != 0) {
				pFollowPath.push_back(InitLine());
				if (!pFollowPath[pFollowPath.size() - 1]) break;

				pFollowPath[pFollowPath.size() - 1]->Layer = 3;
				if (CurList->Warping) {
					pFollowPath[pFollowPath.size() - 1]->Color.ARGB = 0xFFFF0000;
				}
				else {
					pFollowPath[pFollowPath.size() - 1]->Color.ARGB = 0xFF00FF00;
				}
				pFollowPath[pFollowPath.size() - 1]->Start.Z = LastList.Z;
				pFollowPath[pFollowPath.size() - 1]->End.Z = CurList->Z;

				pFollowPath[pFollowPath.size() - 1]->Start.X = -LastList.X;
				pFollowPath[pFollowPath.size() - 1]->End.X = -CurList->X;

				pFollowPath[pFollowPath.size() - 1]->Start.Y = -LastList.Y;
				pFollowPath[pFollowPath.size() - 1]->End.Y = -CurList->Y;

				LastList.Z = CurList->Z;
				LastList.Y = CurList->Y;
				LastList.X = CurList->X;
			}
			CurList++;
		}
	} else if (pFollowPath.size() && !SaveMapPath) {
		MapClear();
	}
}

PLUGIN_API void OnEndZone() {
	DebugSpewAlways("MQ2AdvPath::OnZoned()");
	if (FollowState == FOLLOW_RECORDING && StatusState && SavePathName[0] != 0 && SavePathZone[0] != 0)
		SavePath(SavePathName, SavePathZone);
	ClearAll();
	if (MyWnd && MyWnd->IsVisible()) {
		DestroyMyWindow();
		DoPlayWindow();
	}
}

PLUGIN_API void OnRemoveSpawn(PSPAWNINFO pSpawn) {
	//DebugSpewAlways("MQ2AdvPath::OnRemoveSpawn(%s)", pSpawn->Name);
	if (pSpawn->SpawnID == MonitorID) MonitorID = 0;
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color) {
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn || !AdvPathStatus) return 0;
	if (!_stricmp(Line, "You have been summoned!") && FollowState) {
		WriteChatf("[MQ2AdvPath] summon Detected");
		ClearAll();
	}
	else if (!_strnicmp(Line, "You will now auto-follow", 24)) DoLft(true);
	else if (!_strnicmp(Line, "You are no longer auto-follow", 29) || !_strnicmp(Line, "You must first target a group member to auto-follow.", 52)) {
		if (FollowState) MQFollowCommand(GetCharInfo()->pSpawn, "off");
		else MQFollowCommand(GetCharInfo()->pSpawn, "on");
	}
	return 0;
}
