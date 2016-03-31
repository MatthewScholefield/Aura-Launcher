/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include "fileBrowse.h"
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <dirent.h>

#include <nds.h>
#include <gl2d.h>

#include "iconTitle.h"
#include "graphics/fontHandler.h"
#include "graphics/graphics.h"
#include "graphics/FontGraphic.h"
#include "graphics/TextPane.h"
#include "SwitchState.h"

#define SCREEN_COLS 32
#define ENTRIES_PER_SCREEN 15
#define ENTRIES_START_ROW 3
#define ENTRY_PAGE_LENGTH 10

using namespace std;

struct DirEntry
{
	string name;
	string visibleName;
	bool isDirectory;
};

TextEntry *pathText = nullptr;
char *path = new char[PATH_MAX];

#ifdef EMULATE_FILES
#define chdir(a) chdirFake(a)
void chdirFake(const char *dir)
{
	string pathStr(path);
	string dirStr(dir);
	if (dirStr == "..")
	{
		pathStr.resize(pathStr.find_last_of("/"));
		pathStr.resize(pathStr.find_last_of("/") + 1);
	}
	else
	{
		pathStr += dirStr;
		pathStr += "/";
	}
	strcpy(path, pathStr.c_str());
}
#endif

bool nameEndsWith(const string& name, const vector<string> extensionList)
{

	if (name.size() == 0) return false;

	if (extensionList.size() == 0) return true;

	for (int i = 0; i < (int) extensionList.size(); i++)
	{
		const string ext = extensionList.at(i);
		if (strcasecmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0) return true;
	}
	return false;
}

bool dirEntryPredicate(const DirEntry& lhs, const DirEntry& rhs)
{

	if (!lhs.isDirectory && rhs.isDirectory)
	{
		return false;
	}
	if (lhs.isDirectory && !rhs.isDirectory)
	{
		return true;
	}
	return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
}

void getDirectoryContents(vector<DirEntry>& dirContents, const vector<string> extensionList)
{

	dirContents.clear();

#ifdef EMULATE_FILES

	string vowels = "aeiou";
	string consonants = "bcdfghjklmnpqrstvwxyz";

	DirEntry first;
	first.name = "AAA First";
	first.visibleName = "[AAA First]";
	first.isDirectory = true;
	dirContents.push_back(first);

	for (int i = 0; i < rand() % 10 + 5; ++i)
	{
		ostringstream fileName;
		DirEntry dirEntry;
		for (int j = 0; j < rand() % 15 + 4; ++j)
			fileName << (j % 2 == 0 ? consonants[rand() % consonants.size()]
				: vowels[rand() % vowels.size()]);
		dirEntry.name = fileName.str();
		if ((dirEntry.isDirectory = rand() % 2))
			dirEntry.visibleName = "[" + dirEntry.name + "]";
		else
			dirEntry.visibleName = dirEntry.name;
		dirContents.push_back(dirEntry);
	}
	DirEntry last;
	last.name = "ZZZ Last";
	last.visibleName = last.name;
	last.isDirectory = false;
	dirContents.push_back(last);

#else

	struct stat st;
	DIR *pdir = opendir(".");

	if (pdir == NULL)
	{
		iprintf("Unable to open the directory.\n");
	}
	else
	{

		while (true)
		{
			DirEntry dirEntry;

			struct dirent* pent = readdir(pdir);
			if (pent == NULL) break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;
			dirEntry.isDirectory = (st.st_mode & S_IFDIR) ? true : false;

			if (dirEntry.isDirectory)
				dirEntry.visibleName = "[" + dirEntry.name + "]";
			else
				dirEntry.visibleName = dirEntry.name;


			if (dirEntry.name.compare(".") != 0 && (dirEntry.isDirectory || nameEndsWith(dirEntry.name, extensionList)))
				dirContents.push_back(dirEntry);

		}

		closedir(pdir);
	}

#endif

	sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
}

void getDirectoryContents(vector<DirEntry>& dirContents)
{
	vector<string> extensionList;
	getDirectoryContents(dirContents, extensionList);
}

void updatePath()
{
#ifndef EMULATE_FILES
	getcwd(path, PATH_MAX);
#else
	if (strlen(path) < 1)
	{
		path[0] = '/';
		path[1] = '\0';
	}
#endif
	if (pathText == nullptr)
	{
		printLarge(false, 2 * FONT_SX, 1 * FONT_SY, path);
		pathText = getPreviousTextEntry(false);
		pathText->y = 100;
		pathText->immune = true;
	}
}

string browseForFile(const vector<string> extensionList)
{
	int pressed = 0;
	int screenOffset = 0;
	int fileOffset = 0;
	SwitchState scrn(3);
	vector<DirEntry> dirContents[scrn.SIZE];

	getDirectoryContents(dirContents[scrn], extensionList);
	clearText(false);
	updatePath();
	TextPane *pane = &createTextPane(20, 3 + ENTRIES_START_ROW*FONT_SY, ENTRIES_PER_SCREEN);
	for (auto &i : dirContents[scrn])
		pane->addLine(i.visibleName.c_str());
	pane->createDefaultEntries();
	pane->slideTransition(true);

	printSmall(false, 12 - 16, 4 + 10 * (fileOffset - screenOffset + ENTRIES_START_ROW), ">");
	TextEntry *cursor = getPreviousTextEntry(false);
	cursor->fade = TextEntry::FadeType::IN;
	cursor->finalX += 16;

	while (true)
	{
		cursor->finalY = 4 + 10 * (fileOffset - screenOffset + ENTRIES_START_ROW);
		cursor->delay = TextEntry::ACTIVE;

		iconTitleUpdate(dirContents[scrn].at(fileOffset).isDirectory, dirContents[scrn].at(fileOffset).name.c_str());

		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do
		{
			scanKeys();
			pressed = keysDownRepeat();
			swiWaitForVBlank();
		}
		while (!pressed);
		if (cursor->fade == TextEntry::FadeType::IN)
		{
			cursor->fade = TextEntry::FadeType::NONE;
			cursor->invAccel = 4;
		}

		if (pressed & KEY_UP) fileOffset -= 1;
		if (pressed & KEY_DOWN) fileOffset += 1;
		if (pressed & KEY_LEFT) fileOffset -= ENTRY_PAGE_LENGTH;
		if (pressed & KEY_RIGHT) fileOffset += ENTRY_PAGE_LENGTH;

		if (fileOffset < 0)
		{
			fileOffset = dirContents[scrn].size() - 1;
			screenOffset = max(0, fileOffset - ENTRIES_PER_SCREEN + 1); // Wrap around to bottom of list
			pane->scroll(false);
		}
		else if (fileOffset > ((int) dirContents[scrn].size() - 1))
		{
			screenOffset = fileOffset = 0; // Wrap around to top of list;
			pane->scroll(true);
		}
		else if (fileOffset < screenOffset)
		{
			screenOffset = fileOffset;
			pane->scroll(false);
		}
		else if (fileOffset > screenOffset + ENTRIES_PER_SCREEN - 1)
		{
			screenOffset = fileOffset - ENTRIES_PER_SCREEN + 1;
			pane->scroll(true);
		}

		if (pressed & KEY_A)
		{
			DirEntry* entry = &dirContents[scrn].at(fileOffset);
			if (entry->isDirectory)
			{
				// Enter selected directory
				chdir(entry->name.c_str());
				updatePath();
				pane->slideTransition(false, false, 0, fileOffset - screenOffset);
				pane = &createTextPane(20, 3 + ENTRIES_START_ROW*FONT_SY, ENTRIES_PER_SCREEN);
				getDirectoryContents(dirContents[++scrn], extensionList);
				for (auto &i : dirContents[scrn])
					pane->addLine(i.visibleName.c_str());
				pane->createDefaultEntries();
				pane->slideTransition(true, false, 20);
				screenOffset = 0;
				fileOffset = 0;
			}
			else
			{
				pane->slideTransition(false, true, 0, fileOffset - screenOffset);
				// Return the chosen file
				waitForPanesToClear();
				return entry->name;
			}
		}

		if (pressed & KEY_B && strlen(path) > 2)
		{
			// Go up a directory
			chdir("..");
			updatePath();
			pane->slideTransition(false, true);
			pane = &createTextPane(20, 3 + ENTRIES_START_ROW*FONT_SY, ENTRIES_PER_SCREEN);
			getDirectoryContents(dirContents[++scrn], extensionList);
			for (auto &i : dirContents[scrn])
				pane->addLine(i.visibleName.c_str());
			pane->createDefaultEntries();
			pane->slideTransition(true, true, 20);
			screenOffset = 0;
			fileOffset = 0;
		}
	}
}
