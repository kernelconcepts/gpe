// GtkSharp.Generation.GenerationInfo.cs - Generation information class.
//
// Author: Mike Kestner <mkestner@ximian.com>
//
// Copyright (c) 2003-2005 Novell Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of version 2 of the GNU General Public
// License as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.


namespace GtkSharp.Generation {

	using System;
	using System.Collections;
	using System.IO;
	using System.Xml;

	public class GenerationInfo {
		
		string dir;
		string custom_dir;
		string assembly_name;
		string glue_filename;
		string gluelib_name;
		StreamWriter sw;

		public GenerationInfo (XmlElement ns)
		{
			string ns_name = ns.GetAttribute ("name");
			char sep = Path.DirectorySeparatorChar;
			dir = ".." + sep + ns_name.ToLower () + sep + "generated";
			custom_dir = ".." + sep + ns_name.ToLower ();
			assembly_name = ns_name.ToLower () + "-sharp";
			gluelib_name = "";
			glue_filename = "";
		}

		public GenerationInfo (string dir, string assembly_name) : this (dir, dir, assembly_name, "", "") {}

		public GenerationInfo (string dir, string custom_dir, string assembly_name, string glue_filename, string gluelib_name)
		{
			this.dir = dir;
			this.custom_dir = custom_dir;
			this.assembly_name = assembly_name;
			this.glue_filename = glue_filename;
			this.gluelib_name = gluelib_name;
		}

		public string AssemblyName {
			get {
				return assembly_name;
			}
		}

		public string CustomDir {
			get {
				return custom_dir;
			}
		}

		public string Dir {
			get {
				return dir;
			}
		}

		public string GluelibName {
			get {
				return gluelib_name;
			}
		}

		public bool GlueEnabled {
			get {
				return gluelib_name != String.Empty && glue_filename != String.Empty;
			}
		}

		public string GlueFilename {
			get {
				return glue_filename;
			}
		}

		StreamWriter glue_sw = null;
		public StreamWriter GlueWriter {
			get {
				if (!GlueEnabled)
					return null;

				if (glue_sw == null) {
					FileStream stream = new FileStream (glue_filename, FileMode.Create, FileAccess.Write);
					glue_sw = new StreamWriter (stream);
			
					glue_sw.WriteLine ("// This file was generated by the Gtk# code generator.");
					glue_sw.WriteLine ("// Any changes made will be lost if regenerated.");
					glue_sw.WriteLine ();
				}

				return glue_sw;
			}
		}

		public StreamWriter Writer {
			get {
				return sw;
			}
			set {
				sw = value;
			}
		}

		public void CloseGlueWriter ()
		{
			if (glue_sw != null)
				glue_sw.Close ();
		}

		string member;
		public string CurrentMember {
			get {
				return typename + "." + member;
			}
			set {
				member = value;
			}
		}

		string typename;
		public string CurrentType {
			get {
				return typename;
			}
			set {
				typename = value;
			}
		}

		public StreamWriter OpenStream (string name) 
		{
			char sep = Path.DirectorySeparatorChar;
			if (!Directory.Exists(dir))
				Directory.CreateDirectory(dir);
			string filename = dir + sep + name + ".cs";
			
			FileStream stream = new FileStream (filename, FileMode.Create, FileAccess.Write);
			StreamWriter sw = new StreamWriter (stream);
			
			sw.WriteLine ("// This file was generated by the Gtk# code generator.");
			sw.WriteLine ("// Any changes made will be lost if regenerated.");
			sw.WriteLine ();

			return sw;
		}
	}
}

