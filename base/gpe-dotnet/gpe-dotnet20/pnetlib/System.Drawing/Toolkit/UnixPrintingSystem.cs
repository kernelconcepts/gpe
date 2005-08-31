/*
 * UnixPrintingSystem.cs - Implementation of the
 *			"System.Drawing.Toolkit.UnixPrintingSystem" class.
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

namespace System.Drawing.Toolkit
{

// Printing system object that pipes Postscript data through lpr/lp.
// Currently we only support one printer, defined by the "PRINTER"
// environment variable, and defaulting to the name "lp".

internal sealed class UnixPrintingSystem : IToolkitPrintingSystem
{

	// Get the default printer name on this system.
	public String DefaultPrinterName
			{
				get
				{
					String printer;
					printer = Environment.GetEnvironmentVariable("PRINTER");
					if(printer != null && printer != String.Empty)
					{
						return printer;
					}
					else
					{
						return "lp";
					}
				}
			}

	// Get a list of all installed printers on this system.
	public String[] InstalledPrinters
			{
				get
				{
					return new String [] {DefaultPrinterName};
				}
			}

	// Get the printer control object for a specific printer.
	// Returns null if the printer name is not recognised.
	public IToolkitPrinter GetPrinter(String name)
			{
				if(name == DefaultPrinterName)
				{
					return new UnixPrinter(name);
				}
				else if(name == null)
				{
					return new UnixPrinter(DefaultPrinterName);
				}
				else
				{
					return null;
				}
			}

}; // class UnixPrintingSystem

}; // namespace System.Drawing.Toolkit
