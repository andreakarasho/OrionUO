/****************************************************************************
**
** MultiMap.cpp
**
** Copyright (C) September 2015 Hotride
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*****************************************************************************
*/
//----------------------------------------------------------------------------
#include "stdafx.h"

TMultiMap *MultiMap = NULL;
//----------------------------------------------------------------------------
TMultiMap::TMultiMap()
{
	PBYTE address = (PBYTE)FileManager.MultiMap.Address;
	
	m_Width = *(PDWORD)address;
	address += 4;

	m_Height = *(PDWORD)address;
	address += 4;

	if (m_Width < 1 || m_Height < 1)
	{
		TPRINT("Failed to load bounds from MultiMap.rle\n");
		m_Width = 0;
		m_Height = 0;
	}
}
//----------------------------------------------------------------------------
TMultiMap::~TMultiMap()
{
}
//----------------------------------------------------------------------------
void TMultiMap::LoadMap(TGumpMap *gump)
{
	if (m_Width < 1 || m_Height < 1)
	{
		TPRINT("Failed to load bounds from MultiMap.rle\n");
		return;
	}

	int mapSize = gump->Width * gump->Height;
	PBYTE byteMap = new BYTE[mapSize];
	memset(&byteMap[0], 0, mapSize);

	int startX = gump->StartX / 2;
	int endX = gump->EndX / 2;

	int widthDivizor = endX - startX;

	if (!widthDivizor)
		widthDivizor++;

	int startY = gump->StartY / 2;
	int endY = gump->EndY / 2;

	int heightDivizor = endY - startY;

	if (!heightDivizor)
		heightDivizor++;

	int width = (gump->Width << 8) / widthDivizor;
	int height = (gump->Height << 8) / heightDivizor;

	int x = 0;
	int y = 0;

	int maxPixelValue = 1;

	PBYTE address = (PBYTE)FileManager.MultiMap.Address;
	PBYTE end = address + FileManager.MultiMap.Size;

	address += 8;

	while (address < end)
	{
		BYTE pic = *address++;
		BYTE size = pic & 0x7F;

		bool colored = (pic & 0x80);

		int startHeight = startY * height;
		int currentHeight = y * height;
		int posY = gump->Width * ((currentHeight - startHeight) >> 8);

		IFOR(i, 0, size)
		{
			if (colored && x >= startX && x < endX && y >= startY && y < endY)
			{
				int position = posY + ((width * (x - startX)) >> 8);

				BYTE &pixel = byteMap[position];

				if (pixel < 0xFF)
				{
					if (pixel == maxPixelValue)
						maxPixelValue++;

					pixel++;
				}
			}

			x++;

			if (x >= m_Width)
			{
				x = 0;
				y++;
				currentHeight += height;
				posY = gump->Width * ((currentHeight - startHeight) >> 8);
			}

			//if (y >= m_Height)
			//	break;
		}

		//if (y >= m_Height)
		//	break;
	}

	if (maxPixelValue >= 1)
	{
		PWORD huesData = (PWORD)((PBYTE)ColorManager->GetHuesRangePointer() + 30800); // color = 0x015C

		PWORD colorTable = new WORD[maxPixelValue];
		int colorOffset = 31 * maxPixelValue;

		IFOR(i, 0, maxPixelValue)
		{
			colorOffset -= 31;
			colorTable[i] = 0x8000 | huesData[colorOffset / maxPixelValue];
		}

		PWORD wordMap = new WORD[mapSize];

		IFOR(i, 0, mapSize)
		{
			BYTE &pic = byteMap[i];

			wordMap[i] = (pic ? colorTable[pic - 1] : 0);
		}

		GLuint tex = 0;
		g_GL.BindTexture16(tex, gump->Width, gump->Height, wordMap);
		gump->Texture = tex;

		delete wordMap;
		delete colorTable;
	}

	delete byteMap;
}
//----------------------------------------------------------------------------
bool TMultiMap::LoadFacet(TGumpMap *gump, int facet)
{
	if (facet < 0 || facet > 5)
	{
		TPRINT("Invalid facet index: %i\n", facet);

		return false;
	}

	TMappedHeader &file = FileManager.FacetMul[facet];
	file.Ptr = (PBYTE)file.Address;

	if (!file.Size)
	{
		TPRINT("Facet %i is not loaded!\n", facet);

		return false;
	}

	WORD mapWidth = file.ReadWord();
	WORD mapHeight = file.ReadWord();

	//TPRINT("Facet w:%i h:%i\n", mapWidth, mapHeight);

	int startX = gump->StartX / 2;
	int endX = gump->EndX / 2;

	int startY = gump->StartY / 2;
	int endY = gump->EndY / 2;

	int width = endX - startX;
	int height = endY - startY;

	PWORD map = new WORD[width * height];

	IFOR(y, 0, m_Height)
	{
		int x = 0;
		int colorCount = file.ReadInt() / 3;

		IFOR(i, 0, colorCount)
		{
			int size = file.ReadByte();
			WORD color = 0x8000 | file.ReadWord();

			IFOR(j, 0, size)
			{
				if ((x >= startX && x < endX) && (y >= startY && y < endY))
					map[((y - startY) * width) + (x - startX)] = color;

				x++;
			}
		}
	}

	GLuint tex = 0;
	g_GL.BindTexture16(tex, width, height, map);
	gump->Texture = tex;



	/*int mapSize = width * height;
	PWORD map = new WORD[mapSize];
	PWORD ptr = map;

	IFOR(y, 0, height)
	{
		int colorCount = file.ReadInt() / 3;

		IFOR(i, 0, colorCount)
		{
			int size = file.ReadByte();
			WORD color = 0x8000 | file.ReadWord();

			IFOR(j, 0, size)
				*ptr++ = color;
		}
	}

	GLuint facetTexture = 0;
	g_GL.BindTexture16(facetTexture, width, height, map);*/

	return true;
}
//----------------------------------------------------------------------------