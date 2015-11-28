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
#include <DMM.h>
#include <DSUtil.h>

void DSUtil_InitializeQueue(DSUtil_Queue *queue, unsigned uiAddress, unsigned uiSize)
{
	queue->buffer = (unsigned*)uiAddress ;
	queue->readEnd = 0 ;
	queue->writeEnd = 0 ;
	queue->size = uiSize ;
}

byte DSUtil_ReadFromQueue(DSUtil_Queue *queue, unsigned *data)
{
	if(queue->readEnd == queue->writeEnd)
		return DSUtil_ERR_QUEUE_EMPTY ;

//	if((unsigned)queue->buffer == GetQA(14))
//	{
//		KC::MDisplay().Number("\n RQW: ", queue->writeEnd) ;
//		KC::MDisplay().Number(" QR: ", queue->readEnd) ;
//	}
//
	*data = queue->buffer[queue->readEnd] ;
	queue->readEnd = (queue->readEnd + 1) % queue->size ;

	return DSUtil_SUCCESS ;
}

byte DSUtil_WriteToQueue(DSUtil_Queue *queue, unsigned data)
{
	if(((queue->writeEnd + 1) % queue->size) == queue->readEnd)
			return DSUtil_ERR_QUEUE_FULL ;
			
//	if((unsigned)queue->buffer == GetQA(14))
//	{
//		KC::MDisplay().Number("\n QW: ", queue->writeEnd) ;
//		KC::MDisplay().Number(" QR: ", queue->readEnd) ;
//	}

	queue->buffer[queue->writeEnd] = data ;
	queue->writeEnd = (queue->writeEnd + 1) % queue->size ;
	
	return DSUtil_SUCCESS ;
}

void DSUtil_InitializeStack(DSUtil_Stack *stack, unsigned uiAddress, int size)
{
	stack->pData = (unsigned*)uiAddress ;
	stack->size = size ;
	stack->top = -1 ;
}

byte DSUtil_PushOnStack(DSUtil_Stack *stack, unsigned value)
{
	if(stack->top == stack->size)
		return DSUtil_ERR_STACK_OVERFLOW ;

	stack->pData[++stack->top] = value ;

	return DSUtil_SUCCESS ;
}

byte DSUtil_PopFromStack(DSUtil_Stack *stack, unsigned* value)
{
	if(stack->top < 0)
		return DSUtil_ERR_STACK_UNDERFLOW ;

	*value = stack->pData[stack->top--] ;

	return DSUtil_SUCCESS ;
}

void DSUtil_InitializeSLL(DSUtil_SLL* pSLL)
{
	pSLL->pFirst = NULL ;
	pSLL->pLast = NULL ;
}

byte DSUtil_InsertSLL(DSUtil_SLL* pSLL, unsigned val)
{
	DSUtil_SLLNode* pCur = (DSUtil_SLLNode*)DMM_AllocateForKernel(sizeof(DSUtil_SLLNode)) ;
	if(pCur == NULL)
		return DSUtil_SLL_OUT_OF_MEM ;

	pCur->val = val ;
	pCur->pNext = NULL ;

	if(pSLL->pFirst == NULL)
	{
		pSLL->pFirst = pSLL->pLast = pCur ;
	}
	else
	{
		pSLL->pLast->pNext = pCur ;
		pSLL->pLast = pCur ;
	}

	return DSUtil_SUCCESS ;
}

byte DSUtil_DeleteSLL(DSUtil_SLL* pSLL, unsigned val)
{
	DSUtil_SLLNode *pPrev, *pCur ;
	for(pPrev = pSLL->pFirst, pCur = pSLL->pFirst; pCur != NULL; pPrev = pCur, pCur = pCur->pNext)
	{
		if(pCur->val == val)
		{
			pPrev->pNext = pCur->pNext ;

			if(pCur == pSLL->pLast)
			{
				pSLL->pLast = pPrev ;
			}

			if(pCur == pSLL->pFirst)
			{
				pSLL->pFirst = pCur->pNext ;
			}
			
			DMM_DeAllocateForKernel((unsigned)pCur) ;	
			return DSUtil_SUCCESS ;
		}
	}

	return DSUtil_SLL_NFOUND ;
}

byte DSUtil_IsPresentSLL(DSUtil_SLL* pSLL, unsigned val)
{
	DSUtil_SLLNode *pCur ;

	for(pCur = pSLL->pFirst; pCur != NULL; pCur = pCur->pNext)
	{
		if(pCur->val == val)
		{
			return DSUtil_SUCCESS ;
		}
	}

	return DSUtil_SLL_NFOUND ;
}

