/*
 * HMACRIPEMD160.cs - Implementation of the
 *		"System.Security.Cryptography.HMACRIPEMD160" class.
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

namespace System.Security.Cryptography
{

#if CONFIG_CRYPTO && CONFIG_FRAMEWORK_1_2

using Platform;

public class HMACRIPEMD160 : HMAC
{
	// Constructors.
	public HMACRIPEMD160()
			{
				HashName = "RIPEMD160";
				HashSizeValue = 160;
				byte[] key = new byte [64];
				CryptoMethods.GenerateRandom(key, 0, 64);
			}
	public HMACRIPEMD160(byte[] rgbKey)
			{
				HashName = "RIPEMD160";
				HashSizeValue = 160;
				Key = rgbKey;
			}

	// Destructor.
	~HMACRIPEMD160()
			{
				Dispose(false);
			}

}; // class HMACRIPEMD160

#endif // CONFIG_CRYPTO && CONFIG_FRAMEWORK_1_2

}; // namespace System.Security.Cryptography
