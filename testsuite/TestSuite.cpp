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
#include <Atomic.h>
#include <stdio.h>
#include <TestSuite.h>
#include <list.h>
#include <DMM.h>
#include <BTree.h>
#include <PIT.h>
#include <map.h>

#define TEST_ERROR printf("\n Error %d", __LINE__)
#define ASSERT_CONDITION(COND) if(!(COND)) { TEST_ERROR; return false; }
#define ASSERT(AV, COMP, EV) \
	if(!((AV) COMP (EV))) \
	{\
		TEST_ERROR ; \
		printf("\n Actual Value: %d, Expected Value: %d", AV, EV) ; \
		return false ; \
	}

#define ASSERT_ACT(AV, COMP, EV, ACT) \
	if(!((AV) COMP (EV))) \
	{\
		TEST_ERROR ; \
		printf("\n Actual Value: %d, Expected Value: %d", AV, EV) ; \
		ACT ; \
	}

TestSuite::TestSuite()
{
}

void TestSuite::Run()
{
	unsigned uiHeapSize = DMM_GetKernelHeapSize() ;

	Assert(TestAtomicSwap()) ;
	Assert(TestListDS()) ;
	Assert(TestListDSPtr()) ;
	Assert(TestBTree1()) ;
	Assert(TestBTree2()) ;
	Assert(TestBTree3()) ;
  Assert(TestMap());

	printf("\n") ;

	printf("\n Heap Size: (Before, After) = (%d, %d)\n", uiHeapSize, DMM_GetKernelHeapSize()) ;
}

void TestSuite::Assert(bool b)
{
	if(b)
		printf(" Success") ;
	else
		printf(" Failed") ;
}

bool TestSuite::TestAtomicSwap()
{
	printf("\n Running TestAtomicSwap...") ;
	Mutex m ;

	ASSERT_CONDITION(m.m_iLock == 0) ;
	ASSERT_CONDITION(Atomic::Swap(m.m_iLock, 1) == 0) ;
	ASSERT_CONDITION(m.m_iLock == 1) ;
	ASSERT_CONDITION(Atomic::Swap(m.m_iLock, 0) == 1) ;
	ASSERT_CONDITION(m.m_iLock == 0) ;
	ASSERT_CONDITION(Atomic::Swap(m.m_iLock, 1) == 0) ;
	ASSERT_CONDITION(m.m_iLock == 1) ;

	return true ;
}

bool TestSuite::TestListDS()
{
	printf("\n Running TestListDS...") ;

	upan::list<int> l ;

	l.push_back(100) ;
	l.push_back(200) ;
	l.push_back(300) ;

	l.insert(++l.begin(), 400) ;

	int arr[ ] = { 100, 400, 200, 300 } ;
	int i = 0 ;

  for(auto it : l)
		ASSERT_CONDITION(arr[ i++ ] == it) ;

	ASSERT_CONDITION(l.size() == 4) ;
	ASSERT_CONDITION(l.front() == 100) ;
	l.pop_front() ;
	ASSERT_CONDITION(l.size() == 3) ;
	ASSERT_CONDITION(l.front() == 400);
	ASSERT_CONDITION(l.back() == 300);

	return true ;
}

bool TestSuite::TestListDSPtr()
{
	printf("\n Running TestListDS PTR...") ;

	upan::list<int*> l ;

	l.push_back(new int(100)) ;
	l.push_back(new int(200)) ;
	l.push_back(new int(300)) ;

	l.insert(++l.begin(), new int(400)) ;

	int arr[ ] = { 100, 400, 200, 300 } ;
	int i = 0 ;

  for(auto it : l) 
		ASSERT_CONDITION(arr[ i++ ] == *it);

	ASSERT_CONDITION(l.size() == 4) ;
  int* v = l.front();
	ASSERT_CONDITION(*v == 100) ;
	l.pop_front();
	delete v ;
	ASSERT_CONDITION(l.size() == 3);
	ASSERT_CONDITION(*l.front() == 400);
	ASSERT_CONDITION(*l.back() == 300);

	i = 1;
  for(auto it = l.begin(); it != l.end();)
	{
		ASSERT_CONDITION(arr[ i++ ] == **it);
		delete *it;
    l.erase(it++);
	}

	ASSERT_CONDITION(l.size() == 0);
	return true ;
}

class TestKey : public BTreeKey
{
	public:
		int m_key ;

	public:
		TestKey(int key) : m_key(key) { }

		virtual bool operator<(const BTreeKey& rKey) const
		{
			const TestKey& r = static_cast<const TestKey&>(rKey) ;

			return m_key < r.m_key ;
		}

		virtual void Visit() { printf("\n Key: %d", m_key) ; } 
} ;

class TestValue : public BTreeValue
{
	public:
		TestValue(int val) : m_value(val) { }
		~TestValue() { }

		virtual void Visit() { printf("\n Value: %d", m_value) ; }

		int m_value ;
} ;

class TestInOrderVisitor : public BTree::InOrderVisitor
{
	public:
		mutable int m_bAbort ;
		mutable int m_iCount ;
		const int MaxElements ;
		int* m_arrKey ;
		int* m_arrVal ;

		TestInOrderVisitor(int n, int* k, int* v) : m_bAbort(false), m_iCount(0), MaxElements(n), m_arrKey(k), m_arrVal(v)  { }

		void operator()(const BTreeKey& rKey, BTreeValue* pValue)
		{
			const TestKey& key = static_cast<const TestKey&>(rKey) ;
			const TestValue* value = static_cast<const TestValue*>(pValue) ;

			ASSERT_ACT(m_iCount, <, MaxElements, m_bAbort = true) ;
			ASSERT_ACT(key.m_key, ==, m_arrKey[ m_iCount ], m_bAbort = true) ;
			ASSERT_ACT(value->m_value, ==, m_arrVal[ m_iCount ], m_bAbort = true) ;

			m_iCount++ ;
		}

		bool Abort() const { return m_bAbort ; }
} ;

extern int iNewCount ;

bool TestSuite::TestBTree1()
{
	printf("\n Running TestBTree Case1...") ;

	iNewCount = 0 ;
	{
		BTree tree ;
		BTree::DestroyKeyValue d ;
		tree.SetDestoryKeyValueCallBack(&d) ;
		int k[102], v[102] ;

		for(int i = 0; i < 102; i++) 
		{
			TestKey* pKey = new TestKey(i) ;
			TestValue* pValue = new TestValue(i * 10) ;

			k[i] = i ;
			v[i] = i * 10 ;

			ASSERT(tree.Insert(pKey, pValue), ==, true) ;
		}

		int key = 63 ;
		TestValue* pValue = (TestValue*)tree.Find(TestKey(key)) ;
		ASSERT(pValue, !=, NULL) ;
		ASSERT(pValue->m_value, ==, 630) ;

		TestInOrderVisitor t(102, k, v) ;
		tree.InOrderTraverse(t) ;
	}
	ASSERT(iNewCount, ==, 0) ;

	return true ;
}

bool TestSuite::TestBTree2()
{
	printf("\n Running TestBTree Case2...") ;

	iNewCount = 0 ;
	{
		BTree tree ;
		BTree::DestroyKeyValue d ;
		tree.SetDestoryKeyValueCallBack(&d) ;
		const int N = 522 ;
		int k[N], v[N] ;

		for(int i = 0; i < N; i++) 
		{
			k[i] = PIT_GetClockCount() % N ;
			v[i] = (PIT_GetClockCount() * 3) % N ;

			TestKey* pKey = new TestKey(k[i]) ;
			TestValue* pValue = new TestValue(v[i]) ;

			ASSERT(tree.Insert(pKey, pValue), ==, true) ;
		}

		for(int i = 0; i < N; i++)
			for(int j = i + 1; j < N; j++)
			{
				if(k[i] > k[j])
				{
					int t = k[i] ;
					k[i] = k[j] ;
					k[j] = t ;

					t = v[i] ;
					v[i] = v[j] ;
					v[j] = v[i] ;
				}
			}

		TestInOrderVisitor t(N, k, v) ;
		tree.InOrderTraverse(t) ;
	}
	ASSERT(iNewCount, ==, 0) ;

	return true ;
}

static void remove_arr(int* arr, int size, int index)
{
	for(int i = index; i < size-1; i++)
		arr[i] = arr[i+1] ;
}

bool TestSuite::TestBTree3()
{
	printf("\n Running TestBTree Case3...") ;
	iNewCount = 0 ;
	{
		BTree tree ;
		BTree::DestroyKeyValue d ;
		tree.SetDestoryKeyValueCallBack(&d) ;
		const int N = 322 ;
		int k[N], v[N] ;

		for(int i = 0; i < N; i++) 
		{
			k[i] = i ;
			v[i] = i * 10 ;

			TestKey* pKey = new TestKey(k[i]) ;
			TestValue* pValue = new TestValue(v[i]) ;

			ASSERT(tree.Insert(pKey, pValue), ==, true) ;
		}

		TestInOrderVisitor tiv(N, k, v) ;
		tree.InOrderTraverse(tiv) ;

		int size = N ;
		int pos1 = N - 122 ;
		TestKey t1(k[pos1]) ;
		ASSERT(tree.Delete(t1), ==, true) ;

		remove_arr(k, size, pos1) ;
		remove_arr(v, size, pos1) ;
		--size ;

		TestInOrderVisitor tiv1(size, k, v) ;
		tree.InOrderTraverse(tiv1) ;

		for(int i = 0; i < 200; i++)
		{
			int pos = PIT_GetClockCount() % size ;
			TestKey t(k[pos]) ;
			ASSERT_ACT(tree.Delete(t), ==, true, printf("\n %d, %d, %d", i, pos, k[pos]) ; return false ;) ;

			remove_arr(k, size, pos) ;
			remove_arr(v, size, pos) ;
			--size ;
		}

		TestInOrderVisitor tiv2(size, k, v) ;
		tree.InOrderTraverse(tiv2) ;
	}
	ASSERT(iNewCount, ==, 0) ;

	return true ;
}

bool TestSuite::TestMap()
{
  printf("\n Running TestMap...");
  upan::map<int, int> m;
  for(int i = 0; i < 1000; ++i)
    m.insert(upan::pair<int, int>(i, i));
  for(int i = 0; i < 1000; ++i)
    ASSERT_CONDITION(m[i] == i);
  for(int i = 0; i < 2000; ++i)
    m[i] = i;
  for(int i = 0; i < 2000; ++i)
  {
    auto it = m.find(i);
    ASSERT_CONDITION(it != m.end());
    ASSERT_CONDITION((*it).first == i);
    ASSERT_CONDITION(it->second == i);
  }
  int i = 0;
  for(const auto& it : m)
  {
    ASSERT_CONDITION(it.first == i);
    ASSERT_CONDITION(it.second == i);
    ++i;
  }
  ASSERT_CONDITION(i == 2000);
  return true;
}

