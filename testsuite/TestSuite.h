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
#ifndef _TESTSUITE_H_
#define _TESTSUITE_H_

class TestSuite
{
	public:
		TestSuite() ;
		void Run() ;

	private:
		void Assert(bool b) ;
		bool TestAtomicSwap() ;
		bool TestListDS() ;
		bool TestListDSPtr() ;
		bool TestBTree1() ;
		bool TestBTree2() ;
		bool TestBTree3() ;
    bool TestMap();
} ;

#endif
