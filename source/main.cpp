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
#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>
#include <gl2d.h>

#include "graphics/graphics.h"

#include "ndsLoaderArm9.h"
#include "fileBrowse.h"

#include "hbmenu_banner.h"

#include "iconTitle.h"
#include "graphics/fontHandler.h"

using namespace std;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

char filePath[PATH_MAX];

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	extern u64 *fake_heap_end;
	*fake_heap_end = 0;

	defaultExceptionHandler();
	std::string filename;

	graphicsInit();
	fontInit();
	iconTitleInit();
	/*while (1)
	{
		//swiWaitForVBlank();
		while (REG_DISPCAPCNT & DCAP_ENABLE);
		startRendering(true);
		glBegin2D();
		{
			glBoxFilledGradient(0, 0, 255, 191, RGB15(25, 0, 0), RGB15(23, 20, 0), RGB15(25, 0, 0), RGB15(23, 20, 0));
			drawIcon();
		}
		glEnd2D();
		glFlush(0);
		swiWaitForVBlank();
	}*/
	fontInit();

#ifndef EMULATE_FILES

	if (!fatInitDefault()) {
		iprintf ("fatinitDefault failed!\n");
		stop();
	}

#endif
	
	keysSetRepeat(25,5);

	vector<string> extensionList;
	extensionList.push_back(".nds");
	extensionList.push_back(".argv");
	srand(time(NULL));

	while(1) {

		//Navigates to the file to launch
		filename = browseForFile(extensionList);

		////////////////////////////////////
		// Launch the item
#ifndef EMULATE_FILES
		// Construct a command line
		getcwd (filePath, PATH_MAX);
		int pathLen = strlen(filePath);
		vector<char*> argarray;

		if ( strcasecmp (filename.c_str() + filename.size() - 5, ".argv") == 0) {

			FILE *argfile = fopen(filename.c_str(),"rb");
			char str[PATH_MAX], *pstr;
			const char seps[]= "\n\r\t ";

			while( fgets(str, PATH_MAX, argfile) ) {
				// Find comment and end string there
				if( (pstr = strchr(str, '#')) )
					*pstr= '\0';

				// Tokenize arguments
				pstr= strtok(str, seps);

				while( pstr != NULL ) {
					argarray.push_back(strdup(pstr));
					pstr= strtok(NULL, seps);
				}
			}
			fclose(argfile);
			filename = argarray.at(0);
		} else {
			argarray.push_back(strdup(filename.c_str()));
		}

		if ( strcasecmp (filename.c_str() + filename.size() - 4, ".nds") != 0 || argarray.size() == 0 ) {
			iprintf("no nds file specified\n");
		} else {
			char *name = argarray.at(0);
			strcpy (filePath + pathLen, name);
			free(argarray.at(0));
			argarray.at(0) = filePath;
			iprintf ("Running %s with %d parameters\n", argarray[0], argarray.size());
			int err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0]);
			iprintf ("Start failed. Error %i\n", err);

		}

		while(argarray.size() !=0 ) {
			free(argarray.at(0));
			argarray.erase(argarray.begin());
		}

		while (1) {
			swiWaitForVBlank();
			scanKeys();
			if (!(keysHeld() & KEY_A)) break;
		}
#endif
	}

	return 0;
}
