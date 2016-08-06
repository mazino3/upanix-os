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
#ifndef _TIMER_TASK_H_
#define _TIMER_TASK_H_

#include <ProcessManager.h>

template <typename T>
class TimerTask
{
  using CallBackFunction = bool (T::*)();
  private:
    TimerTask(unsigned uiTimeInMilliSec, T& callBackObj, CallBackFunction callBackFunction) 
      : _uiTimeInMilliSec(uiTimeInMilliSec), _callBackObj(callBackObj), _callBackFunction(callBackFunction)
    {
    }

    unsigned TimeOut() const { return _uiTimeInMilliSec; }

    bool CallBack()
    {
      return (_callBackObj.*_callBackFunction)();
    }

    static void SystemTimer(TimerTask<T>* t, unsigned unused)
    {
      do
      {
        ProcessManager::Instance().Sleep(t->TimeOut()) ;
      } while(t->CallBack());
      delete t;
      ProcessManager_EXIT();
    }

  private:
    unsigned _uiTimeInMilliSec;
    T& _callBackObj;
    CallBackFunction _callBackFunction;

  public:
    static void Start(const char* szName, unsigned uiTimeInMilliSec, T& callBackObj, CallBackFunction callBackFunction)
    {
      auto t = new TimerTask<T>(uiTimeInMilliSec, callBackObj, callBackFunction);
      int pid;
      UNUSED unsigned unused = 0;
      ProcessManager::Instance().CreateKernelImage((unsigned)&SystemTimer, NO_PROCESS_ID, false, (unsigned)t, unused, &pid, szName) ;
    }

};

#endif
