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
#ifndef _USER_MANAGER_H_
#define _USER_MANAGER_H_

#include <Global.h>
#include <string.h>
#include <map.h>

#define MAX_USER_LENGTH				32
#define MIN_USER_LENGTH				6
#define MAX_HOME_DIR_LEN			64

#define ROOT_USER_ID			0
#define DERIVE_FROM_PARENT		(-1)

enum USER_TYPES
{	
	NO_USER,
	SUPER_USER,
	NORMAL_USER
};

class User
{
  public:
    User(const upan::string& name,
         const upan::string& password,
         const upan::string& homeDirPath,
         USER_TYPES type) :
         _name(name),
         _password(password),
         _homeDirPath(homeDirPath),
         _type(type)
     {
     }

    const upan::string& Name() const { return _name; }
    const upan::string& Password() const { return _password; }
    const upan::string& HomeDirPath() const { return _homeDirPath; }
    USER_TYPES Type() const { return _type; }

  private:
    upan::string _name;
    upan::string _password;
    upan::string _homeDirPath;
    USER_TYPES   _type;
};

class UserManager
{
  private:
    UserManager();

  public:
    typedef upan::map<upan::string, User*> UserMap;
    static UserManager& Instance()
    {
      static UserManager instance;
      return instance;
    }

    bool Create(const upan::string& name, const upan::string& password, const upan::string& homeDirPath, USER_TYPES type);
    bool Delete(const upan::string& name);
    const User* GetUserEntryByName(const upan::string& name) const;
    const UserMap& GetUserList() const { return _users; }
 
  private:
    void CreateNewUserList();
    void InitializeDefaultUserList();
    void WriteUserList();
    bool LoadUserList();
    bool ValidateName(const upan::string& name);

    const upan::string _userListFileName;
    UserMap _users;
};

#endif
