#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "Common.h"

typedef struct vote_st
{
	REQUEST r;
	time_t t;
}VOTE;

typedef struct node_st
{
	VOTE v;
	struct node_st* next;
}NODE;

typedef struct voteTimerParams_st
{
	NODE** head;
	CRITICAL_SECTION* cs;
}VOTE_TIMER_PARAMS;



void Init(NODE** head, CRITICAL_SECTION cs)
{
	EnterCriticalSection(&cs);
	*head = NULL;
	LeaveCriticalSection(&cs);
}

void Enqueue(NODE** head, VOTE v, CRITICAL_SECTION cs)
{
	NODE* newEl = (NODE*)malloc(sizeof(NODE));
	if (newEl == NULL)
	{
		return;
	}
	newEl->v = v;
	newEl->next = NULL;

	newEl->next = *head;

	EnterCriticalSection(&cs);
	*head = newEl;
	LeaveCriticalSection(&cs);
}

VOTE Dequeue(NODE** head, CRITICAL_SECTION cs)
{
	NODE* temp = *head;
	if (temp == NULL)
	{
		VOTE v;
		v.r.choice = -1;
		return v;
	}
	if (temp->next == NULL)
	{
		VOTE ret = temp->v;

		EnterCriticalSection(&cs);
		free(temp);
		*head = NULL;
		LeaveCriticalSection(&cs);

		return ret;
	}
	NODE* prethodni = temp;
	temp = temp->next;
	while (temp->next != NULL)
	{
		prethodni = temp;
		temp = temp->next;
	}
	VOTE ret = temp->v;
	EnterCriticalSection(&cs);
	free(temp);
	prethodni->next = NULL;
	LeaveCriticalSection(&cs);

	return ret;
}