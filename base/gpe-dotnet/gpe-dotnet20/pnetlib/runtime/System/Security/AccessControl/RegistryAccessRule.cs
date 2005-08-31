/*
 * RegistryAccessRule.cs - Implementation of the
 *			"System.Security.AccessControl.RegistryAccessRule" class.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
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

namespace System.Security.AccessControl
{

#if CONFIG_ACCESS_CONTROL

using System.Security.Principal;
using System.Runtime.InteropServices;

#if !ECMA_COMPAT
[ComVisible(false)]
#endif
public sealed class RegistryAccessRule : AccessRule
{
	// Constructor.
	public RegistryAccessRule
				(IdentityReference identity, RegistryRights registryRights,
				 AccessControlType type)
			: base(identity, (int)registryRights, false,
				   InheritanceFlags.None, PropagationFlags.None, type) {}
	public RegistryAccessRule
				(IdentityReference identity, RegistryRights registryRights,
				 InheritanceFlags inheritanceFlags,
				 PropagationFlags propagationFlags, AccessControlType type)
			: base(identity, (int)registryRights, false,
				   inheritanceFlags, propagationFlags, type) {}
	public RegistryAccessRule
				(String identity, RegistryRights registryRights,
				 AccessControlType type)
			: base(IdentityReference.IdentityFromName(identity),
				   (int)registryRights, false, InheritanceFlags.None,
				   PropagationFlags.None, type) {}
	public RegistryAccessRule
				(String identity, RegistryRights registryRights,
				 InheritanceFlags inheritanceFlags,
				 PropagationFlags propagationFlags, AccessControlType type)
			: base(IdentityReference.IdentityFromName(identity),
				   (int)registryRights, false, inheritanceFlags,
				   propagationFlags, type) {}

	// Get this object's properties.
	public RegistryRights RegistryRights
			{
				get
				{
					return (RegistryRights)AccessMask;
				}
			}

}; // class RegistryAccessRule

#endif // CONFIG_ACCESS_CONTROL

}; // namespace System.Security.AccessControl
