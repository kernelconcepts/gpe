/*
 * DesigntimeLicenseContextSerializer.cs - Implementation of the
 *		"System.ComponentModel.Design.DesigntimeLicenseContextSerializer" class.
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

namespace System.ComponentModel.Design
{

#if CONFIG_SERIALIZATION && CONFIG_COMPONENT_MODEL_DESIGN

using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

public class DesigntimeLicenseContextSerializer
{
	// Cannot instantiate this class.
	private DesigntimeLicenseContextSerializer() {}

	// Serialize a license key to a stream.
	public static void Serialize(Stream o, String cryptoKey,
		     					 DesigntimeLicenseContext context)
			{
				BinaryFormatter formatter = new BinaryFormatter();
				formatter.Serialize
					(o, new Object [] {cryptoKey, context.keys});
			}

}; // class DesigntimeLicenseContextSerializer

#endif // CONFIG_SERIALIZATION && CONFIG_COMPONENT_MODEL_DESIGN

}; // namespace System.ComponentModel.Design
