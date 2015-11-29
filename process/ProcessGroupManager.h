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
#ifndef _PROCESS_GROUP_MANAGER_H_
#define _PROCESS_GROUP_MANAGER_H_

#define ProcessGroupManager_SUCCESS				0
#define ProcessGroupManager_FAILURE				1
#define ProcessGroupManager_ERR_NO_FREE_PGAS	2

# include <Global.h>
# include <DisplayManager.h>
# include <Atomic.h>

#define MAX_NO_PROCESS_GROUP 20

typedef struct FGProcessList FGProcessList ;

struct FGProcessList 
{
	int iProcessID ;
	FGProcessList* pNext ;
} ;

typedef struct
{
	int				iProcessCount ;
	byte			bFree ;
	FGProcessList	fgProcessListHead ;
	DisplayBuffer		videoBuffer ;
} ProcessGroup ;

#define NO_PROCESS_GROUP_ID -1

class ProcessGroupManager
{
  private:
    ProcessGroupManager();

  public:
    static ProcessGroupManager& Instance()
    {
      static ProcessGroupManager instance;
      return instance;
    }

    int GetProcessCount(int iProcessGroupID);
    int CreateProcessGroup(bool isFGProcessGroup);
    void DestroyProcessGroup(int iProcessGroupID);
    void PutOnFGProcessList(int iProcessID);
    void RemoveFromFGProcessList(int iProcessID);
    void AddProcessToGroup(int iProcessID);
    void RemoveProcessFromGroup(int iProcessID);
    void SwitchFGProcessGroup(int iProcessID);
    bool IsFGProcessGroup(int iProcessGroupID);
    bool IsFGProcess(int iProcessGroupID, int iProcessID);
    ProcessGroup& GetProcessGroup(int iProcessGroupID)
    {
      return _groups[iProcessGroupID];
    }
    const ProcessGroup& GetFGProcessGroup()
    {
	    return GetProcessGroup(_iFGProcessGroup);
    }

  private:
    int GetFreePGAS();
    void FreePGAS(int iProcessGroupID);

    Mutex _mutex;
    int _iFGProcessGroup;
    ProcessGroup* _groups;
};

#endif
