#pragma once

#define BROJ_KANDIDATA 5

typedef struct request_st
{
	char id[30];
	int choice;
}REQUEST;

typedef struct votes_st
{
	int votes[3];
}VOTES;