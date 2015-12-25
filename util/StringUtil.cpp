/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa_prajwal@yahoo.co.in'
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */
# include <StringUtil.h>
# include <GenericUtil.h>

unsigned String_Copy(char* dest, const char* src)
{
	unsigned i ;

	for(i = 0; src[i] != '\0'; i++)
		dest[i] = src[i] ;
	dest[i] = '\0' ;

	return i ;
}

unsigned String_CanCat(char* dest, const char* src)
{
	unsigned i, j ;
	
	for(i = 0; dest[i] != '\0'; i++) ;

	for(j = 0; src[j] != '\0'; j++)
		dest[i++] = src[j] ;
	dest[i] = '\0' ;

	return i ;
}

short String_Compare(const char* s1, const char* s2)
{
	unsigned i ;

	for(i = 0; s1[i] != '\0' && s2[i] != '\0'; i++)
	{
		if(s1[i] != s2[i])
		{
			if(s1[i] < s2[i])
				return -1 ;
			else
				return 1 ;
		}
	}
	
	if(s1[i] == '\0' && s2[i] != '\0')
		return -1 ;
	else if(s1[i] != '\0' && s2[i] == '\0')
		return 1 ;
	
	return 0 ;
}

void String_RawCopy(byte* dest, const byte* src, const unsigned uiLen)
{
	unsigned i ;
	for(i = 0; i < uiLen; i++)
		dest[i] = src[i] ;	
}

int String_Length(const char* str)
{
	int i ;
	for(i = 0; str[i] != '\0'; i++) ;
	return i ;
}

void String_Tokenize(const char* src, char chToken, int* iListSize, StringTokenizer& tokenizer)
{
	int index ;
	int iStartIndex = 0 ;
	int iTokenIndex = 0 ;

	for(index = 0; src[index] != '\0'; index++)
	{
		if(src[index] == chToken)
		{
			if((index - iStartIndex) > 0)
			{
				tokenizer(iTokenIndex, src + iStartIndex, (index - iStartIndex)) ;
				iTokenIndex++ ;
			}

			iStartIndex = index + 1 ;
		}
	}

	if((index - iStartIndex) > 0)
	{
		tokenizer(iTokenIndex, src + iStartIndex, (index - iStartIndex)) ;
		iTokenIndex++ ;
	}

	*iListSize = iTokenIndex ;
}

void String_Reverse(char* str)
{	
	unsigned len = String_Length(str) ;
	unsigned i ;
	char temp ;

	for(i = 0; i < len / 2; i++)
	{
		temp = str[i] ;
		str[i] = str[len - i - 1] ;
		str[len - i - 1] = temp ;
	}	
}

upan::string ToString(unsigned uiNumber)
{
  char strNumber[128];
	unsigned i = 0;

	do
	{
		strNumber[i++] = (uiNumber % 10) + 0x30;
		uiNumber /= 10;
    if(i == 128)
      return "";
	}
	while(uiNumber) ;

	strNumber[i] = '\0';
	String_Reverse(strNumber);
  return strNumber;
}

byte String_ConvertStringToNumber(unsigned* uiNumber, char* strNumber)
{
	__volatile__ int i, pow ;

	*uiNumber = 0 ;
	pow = 0 ;

	for(i = String_Length(strNumber) - 1; i >= 0; i--)
	{
		if(!isdigit(strNumber[i]))
			return false ;

		*uiNumber += (strNumber[i] - 0x30) * GenericUtil_Power(10, pow++) ;
	}

	return true ;
}

short String_NCompare(const char* s1, const char* s2, unsigned len)
{
	unsigned i ;

	for(i = 0; i < len && s1[i] != '\0' && s2[i] != '\0'; i++)
	{
		if(s1[i] != s2[i])
		{
			if(s1[i] < s2[i])
				return -1 ;
			else
				return 1 ;
		}
	}
	
	if(i == len)
		return 0 ;

	if(s1[i] == '\0' && s2[i] != '\0')
		return -1 ;
	else if(s1[i] != '\0' && s2[i] == '\0')
		return 1 ;
	
	return 0 ;
}

byte String_IsSubStr(const char* szMainString, const char* szSubString)
{
	unsigned i ;
	byte bFound = false;
	unsigned uiMainStringLen = String_Length(szMainString) ;
	unsigned uiSubStringLen = String_Length(szSubString) ;

	for(i = 0; i < uiMainStringLen; i++)
	{
		if((uiMainStringLen - i) < uiSubStringLen)
			break ;

		if(String_NCompare(szMainString + i, szSubString, uiSubStringLen) == 0)
		{
			bFound = true ;
			break ;
		}
	}

	return bFound;
}

int String_Chr(const char* szStr, char ch)
{
	int len = String_Length(szStr) ;
	int i ;
	
	for(i = 0; i < len; i++)
	{
		if(szStr[i] == ch)
			return i ;
	}

	return -1 ;
}
