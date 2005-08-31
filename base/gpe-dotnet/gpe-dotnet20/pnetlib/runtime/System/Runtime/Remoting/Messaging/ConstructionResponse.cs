/*
 * ConstructionResponse.cs - Implementation of the
 *			"System.Runtime.Remoting.Messaging.ConstructionResponse" class.
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

namespace System.Runtime.Remoting.Messaging
{

#if CONFIG_REMOTING

using System.Collections;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Remoting.Activation;

[Serializable]
[CLSCompliant(false)]
public class ConstructionResponse : MethodResponse, IConstructionReturnMessage,
									IMethodMessage, IMessage
{
	// Constructors.
	public ConstructionResponse(Header[] h1, IMethodCallMessage mcm)
			: base(h1, mcm) {}
	internal ConstructionResponse(IMethodReturnMessage mrm) : base(mrm) {}

	// Override the properties from the base class.
	public override IDictionary Properties
			{
				get
				{
					return base.Properties;
				}
			}

}; // class ConstructionResponse

#endif // CONFIG_REMOTING

}; // namespace System.Runtime.Remoting.Messaging
