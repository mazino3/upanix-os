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

upan::vector<upan::string> tokenize(const char* src, char chToken) {
  int index;
  int iStartIndex = 0;
  upan::vector<upan::string> tokens;

  for(index = 0; src[index] != '\0'; index++) {
    if(src[index] == chToken) {
      if((index - iStartIndex) > 0) {
        upan::string token(src + iStartIndex, (index - iStartIndex));
        tokens.push_back(token);
      }
      iStartIndex = index + 1 ;
    }
  }

  if((index - iStartIndex) > 0) {
    upan::string token(src + iStartIndex, (index - iStartIndex));
    tokens.push_back(token);
  }
  return tokens;
}

int String_Chr(const char* szStr, char ch)
{
	int len = strlen(szStr) ;
	int i ;
	
	for(i = 0; i < len; i++)
	{
		if(szStr[i] == ch)
			return i ;
	}

	return -1 ;
}
