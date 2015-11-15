/*-----------------------------------------------------------------
 Copyright (C) 2015
	Matthew Scholefield

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
#pragma once

class TextEntry
{
public:
	static const int ACTIVE = 0;
	static const int COMPLETE = -1;
	static const int PRECISION = 10;

	enum class FadeType
	{
		NONE,
		IN,
		OUT
	};

	bool large;
	bool immune; //Does not clear
	FadeType fade;
	int initX, initY, x, y, finalX, finalY;
	int invAccel;
	int delay;
	int polyID;
	const char *message;

	int calcAlpha();
	bool update(); //Returns true to delete itself

	TextEntry(bool large, int x, int y, const char *message);
};