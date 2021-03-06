#include "MainWindow.hpp"

#include <cstdint>

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include <ShObjIdl.h>
#include <Windows.h>

#include <bass.h>

#include "MUtil.hpp"
#include "OSUtil.hpp"

using std::find_if;
using std::list;
using std::string;
using std::vector;
using std::wstring;

namespace inetr {
	void MainWindow::bufferTimer_Tick() {
		QWORD progress = BASS_StreamGetFilePosition(currentStream,
			BASS_FILEPOS_BUFFER) * 100 / BASS_StreamGetFilePosition(
			currentStream, BASS_FILEPOS_END);

		if (progress > 75 || !BASS_StreamGetFilePosition(currentStream,
			BASS_FILEPOS_CONNECTED)) {

				KillTimer(window, bufferTimerId);

				radioStatus = INTER_RS_Connected;
				updateStatusLabel();

				updateMeta();

				BASS_ChannelSetSync(currentStream, BASS_SYNC_META, 0,
					&staticMetaSync, reinterpret_cast<void*>(this));
				BASS_ChannelSetSync(currentStream, BASS_SYNC_OGG_CHANGE, 0,
					&staticMetaSync, reinterpret_cast<void*>(this));

				BASS_ChannelSetAttribute(currentStream, BASS_ATTRIB_VOL,
					radioGetVolume());
				BASS_ChannelPlay(currentStream, FALSE);

				SetTimer(window, metaTimerId, 5000,
					nullptr);
		} else {
			radioStatus = INETR_RS_Buffering;
			radioStatus_bufferingProgress = progress;
			updateStatusLabel();
		}
	}

	void MainWindow::metaTime_Tick() {
		updateMeta();
	}

	void MainWindow::slideTimer_Tick() {
		int oSlideOffset = leftPanelSlideProgress;

		switch (leftPanelSlideStatus) {
		case INETR_WSS_Expanding:
			leftPanelSlideProgress += slideStep;
			if (leftPanelSlideProgress >= slideMax_Left) {
				leftPanelSlideProgress = slideMax_Left;
				leftPanelSlideStatus = INETR_WSS_Expanded;
			}
			break;
		case INETR_WSS_Retracting:
			leftPanelSlideProgress -= slideStep;
			if (leftPanelSlideProgress <= 0) {
				leftPanelSlideProgress = 0;
				leftPanelSlideStatus = INETR_WSS_Retracted;
			}
		}
		switch (bottomPanelSlideStatus) {
		case INETR_WSS_Expanding:
			bottomPanelSlideProgress += slideStep;
			if (bottomPanelSlideProgress >= slideMax_Bottom) {
				bottomPanelSlideProgress = slideMax_Bottom;
				bottomPanelSlideStatus = INETR_WSS_Expanded;
			}
			break;
		case INETR_WSS_Retracting:
			bottomPanelSlideProgress -= slideStep;
			if (bottomPanelSlideProgress <= 0) {
				bottomPanelSlideProgress = 0;
				bottomPanelSlideStatus = INETR_WSS_Retracted;

				ShowWindow(updateBtn, SW_HIDE);
				ShowWindow(dontUpdateBtn, SW_HIDE);
			}
		}
		switch (bottom2PanelSlideStatus) {
		case INETR_WSS_Expanding:
			bottom2PanelSlideProgress += slideStep;
			if (bottom2PanelSlideProgress >= slideMax_Bottom2) {
				bottom2PanelSlideProgress = slideMax_Bottom2;
				bottom2PanelSlideStatus = INETR_WSS_Expanded;
			}
			break;
		case INETR_WSS_Retracting:
			bottom2PanelSlideProgress -= slideStep;
			if (bottom2PanelSlideProgress <= 0) {
				bottom2PanelSlideProgress = 0;
				bottom2PanelSlideStatus = INETR_WSS_Retracted;
			}
			break;
		}

		if (leftPanelSlideStatus != INETR_WSS_Expanding && leftPanelSlideStatus
			!= INETR_WSS_Retracting && bottomPanelSlideStatus !=
			INETR_WSS_Expanding && bottomPanelSlideStatus !=
			INETR_WSS_Retracting && bottom2PanelSlideStatus !=
			INETR_WSS_Expanding && bottom2PanelSlideStatus !=
			INETR_WSS_Retracting) {

			KillTimer(window, slideTimerId);
		}

		calculateControlPositions(window);

		RECT wndPos;
		GetWindowRect(window, &wndPos);
		int slideOffsetDiff = leftPanelSlideProgress - oSlideOffset;

		MoveWindow(window, wndPos.left - slideOffsetDiff,
			wndPos.top, windowWidth + leftPanelSlideProgress,
			windowHeight + bottomPanelSlideProgress +
			bottom2PanelSlideProgress, TRUE);

		SetWindowPos(stationsLbox, nullptr,
			controlPositions["stationsLbox"].left,
			controlPositions["stationsLbox"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(statusLbl, nullptr, controlPositions["statusLbl"].left,
			controlPositions["statusLbl"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(stationImg, nullptr, controlPositions["stationImg"].left,
			controlPositions["stationImg"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(noStationsInfoLbl, nullptr,
			controlPositions["noStationsInfoLbl"].left,
			controlPositions["noStationsInfoLbl"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(updateInfoLbl, nullptr,
			controlPositions["updateInfoLbl"].left,
			controlPositions["updateInfoLbl"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(updateBtn, nullptr, controlPositions["updateBtn"].left,
			controlPositions["updateBtn"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(dontUpdateBtn, nullptr,
			controlPositions["dontUpdateBtn"].left,
			controlPositions["dontUpdateBtn"].top, 0, 0, SWP_NOSIZE);

		SetWindowPos(volumePbar, nullptr, 0, 0,
			RWIDTH(controlPositions["volumePbar"]),
			RHEIGHT(controlPositions["volumePbar"]), SWP_NOMOVE);

		SetWindowPos(updatingLbl, nullptr, controlPositions["updatingLbl"].left,
			controlPositions["updatingLbl"].top, 0, 0, SWP_NOSIZE);

		if (RWIDTH(controlPositions["allStationsLbox"]) <= 0) {
			ShowWindow(allStationsLbox, SW_HIDE);
			ShowWindow(languageCbox, SW_HIDE);
		} else {
			ShowWindow(allStationsLbox, SW_SHOW);
			ShowWindow(languageCbox, SW_SHOW);

			SetWindowPos(allStationsLbox, nullptr, 0, 0,
				RWIDTH(controlPositions["allStationsLbox"]),
				RHEIGHT(controlPositions["allStationsLbox"]),
				SWP_NOMOVE);
			SetWindowPos(languageCbox, nullptr, 0, 0,
				RWIDTH(controlPositions["languageCbox"]),
				RHEIGHT(controlPositions["languageCbox"]),
				SWP_NOMOVE);
		}

		if (RHEIGHT(controlPositions["updateInfoEd"]) <= 0) {
			ShowWindow(updateInfoEd, SW_HIDE);
		} else {
			ShowWindow(updateInfoEd, SW_SHOW);

			SetWindowPos(updateInfoEd, nullptr, 0, 0,
				RWIDTH(controlPositions["updateInfoEd"]),
				RHEIGHT(controlPositions["updateInfoEd"]),
				SWP_NOMOVE);
		}

		if (OSUtil::IsWin7OrLater()) {
			ITaskbarList3 *taskbarList = nullptr;

			if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr,
				CLSCTX_INPROC_SERVER, __uuidof(taskbarList),
				reinterpret_cast<void**>(&taskbarList)))) {

				taskbarList->SetThumbnailClip(window,
					&controlPositions["stationImg"]);

				taskbarList->Release();
			}
		}
	}

	void MainWindow::hideVolBarTimer_Tick() {
		KillTimer(window, hideVolBarTimerId);

		ShowWindow(volumePbar, SW_HIDE);
	}


	void MainWindow::stationsListBox_SelChange() {
		if (leftPanelSlideStatus != INETR_WSS_Retracted)
			return;

		LRESULT index = SendMessage(stationsLbox, LB_GETCURSEL, (WPARAM)0,
			(LPARAM)0);
		LRESULT textLength = SendMessage(stationsLbox, LB_GETTEXTLEN,
			(WPARAM)index, (LPARAM)0);
		char* cText = new char[size_t(textLength + 1)];
		SendMessage(stationsLbox, LB_GETTEXT, (WPARAM)index, (LPARAM)cText);
		string text(cText);
		delete[] cText;

		list<Station>::const_iterator it = find_if(stations.begin(),
			stations.end(), [&text](const Station &elem) {

			return elem.Name == text;
		});

		if (it == stations.end() || &*it == currentStation) {
			if (radioStatus == INETR_RS_ConnectionError)
				radioOpenURL(it->StreamURL);
			return;
		}

		currentStation = &*it;
		ShowWindow(stationImg, SW_SHOW);
		SendMessage(stationImg, STM_SETIMAGE, IMAGE_BITMAP,
			(LPARAM)currentStation->Image);
		radioOpenURL(it->StreamURL);
	}

	void MainWindow::stationsListBox_DblClick() {
		if (leftPanelSlideStatus != INETR_WSS_Expanded)
			return;

		LRESULT index = SendMessage(stationsLbox, LB_GETCURSEL, (WPARAM)0,
			(LPARAM)0);
		LRESULT textLength = SendMessage(stationsLbox, LB_GETTEXTLEN,
			(WPARAM)index, (LPARAM)0);
		char* cText = new char[size_t(textLength + 1)];
		SendMessage(stationsLbox, LB_GETTEXT, (WPARAM)index, (LPARAM)cText);
		string text(cText);
		delete[] cText;

		userConfig.FavoriteStations.remove_if([&text](const Station* &elem) {
			return elem->Name == text;
		});

		populateFavoriteStationsListbox();
	}

	void MainWindow::moreStationsListBox_DblClick() {
		if (leftPanelSlideStatus != INETR_WSS_Expanded)
			return;

		LRESULT index = SendMessage(allStationsLbox, LB_GETCURSEL, (WPARAM)0,
			(LPARAM)0);
		LRESULT textLength = SendMessage(allStationsLbox, LB_GETTEXTLEN,
			(WPARAM)index, (LPARAM)0);
		char* cText = new char[size_t(textLength + 1)];
		SendMessage(allStationsLbox, LB_GETTEXT, (WPARAM)index,
			(LPARAM)cText);
		string text(cText);
		delete[] cText;

		list<Station>::const_iterator it = find_if(stations.begin(),
			stations.end(), [&text](const Station &elem) {

			return elem.Name == text;
		});

		if (it != stations.end() && find(userConfig.FavoriteStations.begin(),
			userConfig.FavoriteStations.end(), &*it) ==
			userConfig.FavoriteStations.end()) {

			userConfig.FavoriteStations.push_back(&*it);
		}

		populateFavoriteStationsListbox();
	}

	void MainWindow::languageComboBox_SelChange() {
		if (leftPanelSlideStatus != INETR_WSS_Expanded)
			return;

		LRESULT index = SendMessage(languageCbox, CB_GETCURSEL, 0, 0);
		LRESULT textLength = SendMessage(languageCbox, CB_GETLBTEXTLEN,
			(WPARAM)index, 0);
		char* cText = new char[size_t(textLength + 1)];
		SendMessage(languageCbox, CB_GETLBTEXT, (WPARAM)index,
			(LPARAM)cText);
		string text(cText);
		delete[] cText;

		if (userConfig.CurrentLanguage.Name == text)
			return;

		vector<Language>::const_iterator it = find_if(languages.begin(),
			languages.end(), [&text](const Language &elem) {

			return elem.Name == text;
		});

		if (it == languages.end())
			return;

		userConfig.CurrentLanguage = *it;

		updateControlLanguageStrings();

		ITaskbarList3 *taskbarList;
		if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr,
			CLSCTX_INPROC_SERVER, __uuidof(taskbarList),
			reinterpret_cast<void**>(&taskbarList)))) {

			THUMBBUTTON thumbButtons[1];
			thumbButtons[0].dwMask = THB_TOOLTIP;
			thumbButtons[0].iId = thumbBarMuteBtnId;
			string muteButtonStr = userConfig.CurrentLanguage["mute"];
			wstring wMuteButtonStr(muteButtonStr.length(), L'');
			copy(muteButtonStr.begin(), muteButtonStr.end(),
				wMuteButtonStr.begin());
			wcscpy_s(thumbButtons[0].szTip,
				sizeof(thumbButtons[0].szTip) /
				sizeof(thumbButtons[0].szTip[0]),
				wMuteButtonStr.c_str());

			taskbarList->ThumbBarUpdateButtons(window, 1,
				thumbButtons);
		}
	}

	void MainWindow::updateButton_Click() {
		radioStop();

		EnableWindow(stationsLbox, FALSE);
		EnableWindow(stationImg, FALSE);
		EnableWindow(statusLbl, FALSE);
		EnableWindow(allStationsLbox, FALSE);
		EnableWindow(languageCbox, FALSE);
		EnableWindow(updateInfoLbl, FALSE);
		EnableWindow(updateBtn, FALSE);
		EnableWindow(dontUpdateBtn, FALSE);
		EnableWindow(volumePbar, FALSE);
		EnableWindow(updateInfoEd, FALSE);
		EnableWindow(window, FALSE);

		if (leftPanelSlideStatus != INETR_WSS_Retracted) {
			leftPanelSlideStatus = INETR_WSS_Retracting;
			SetTimer(window, slideTimerId, slideSpeed,
				nullptr);
		}
		retractBottomPanel();

		ShowWindow(updatingLbl, SW_SHOW);

		downloadUpdates();
	}

	void MainWindow::dontUpdateButton_Click() {
		retractBottomPanel();
		if (bottom2PanelSlideStatus == INETR_WSS_Expanded)
			retractBottom2Panel();
	}


	void MainWindow::mouseScroll(int16_t delta) {
		float rDelta = static_cast<float>(delta) /
			static_cast<float>(WHEEL_DELTA);
		float nVolume = userConfig.RadioVolume + (rDelta * 0.1f);
		nVolume = (nVolume > 1.0f) ? 1.0f : ((nVolume < 0.0f) ? 0.0f :
			nVolume);
		radioSetVolume(nVolume);
	}
}
