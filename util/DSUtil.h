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
#ifndef _DSUTIL_H_
#define _DSUTIL_H_

#include <Global.h>

#define DSUtil_SUCCESS 0

#define DSUtil_ERR_QUEUE_EMPTY 1
#define DSUtil_ERR_QUEUE_FULL 2

#define DSUtil_ERR_STACK_OVERFLOW 3
#define DSUtil_ERR_STACK_UNDERFLOW 4

#define DSUtil_SLL_OUT_OF_MEM 5
#define DSUtil_SLL_NFOUND 6

typedef struct
{
	unsigned* buffer ;
	unsigned short readEnd ;
	unsigned short writeEnd ;
	unsigned size ;
} DSUtil_Queue ;

void DSUtil_InitializeQueue(DSUtil_Queue *queue, unsigned uiAddress, unsigned uiSize) ;
byte DSUtil_ReadFromQueue(DSUtil_Queue *queue, unsigned *data) ;
byte DSUtil_WriteToQueue(DSUtil_Queue *queue, unsigned data) ;

typedef struct
{
	unsigned* pData ;
	int size ;
	int top ;
} DSUtil_Stack ;

void DSUtil_InitializeStack(DSUtil_Stack *stack, unsigned uiAddress, int size) ;
byte DSUtil_PushOnStack(DSUtil_Stack *stack, unsigned value) ;
byte DSUtil_PopFromStack(DSUtil_Stack *stack, unsigned* value) ;

struct DSUtil_SLLNode
{
	unsigned val ;
	struct DSUtil_SLLNode* pNext ;
} ;

typedef struct DSUtil_SLLNode DSUtil_SLLNode ;

struct DSUtil_SLL
{
	struct DSUtil_SLLNode* pFirst ;	
	struct DSUtil_SLLNode* pLast ;	
} ;

typedef struct DSUtil_SLL DSUtil_SLL ;

void DSUtil_InitializeSLL(DSUtil_SLL* pSLL) ;
byte DSUtil_InsertSLL(DSUtil_SLL* pSLL, unsigned val) ;
byte DSUtil_DeleteSLL(DSUtil_SLL* pSLL, unsigned val) ;
byte DSUtil_IsPresentSLL(DSUtil_SLL* pSLL, unsigned val) ;

#endif
