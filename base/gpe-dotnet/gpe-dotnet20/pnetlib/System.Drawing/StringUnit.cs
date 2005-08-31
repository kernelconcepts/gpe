/*
 * StringUnit.cs - Implementation of the
 *			"System.Drawing.StringUnit" class.
 *
 * Copyright (C) 2003  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

namespace System.Drawing
{

public enum StringUnit
{
	World		= 0,
	Display		= 1,
	Pixel		= 2,
	Point		= 3,
	Inch		= 4,
	Document	= 5,
	Millimeter	= 6,
	Em			= 32

}; // enum StringUnit
	
}; // namespace System.Drawing
