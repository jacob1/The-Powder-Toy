#include <cstring>
#include <stddef.h>
#include "Particle.h"

int Particle_GetOffset(const char * key, int * format)
{
	int offset;
	if (!strcmp(key, "type"))
	{
		offset = offsetof(particle, type);
		*format = 2;
	}
	else if (!strcmp(key, "life"))
	{
		offset = offsetof(particle, life);
		*format = 0;
	}
	else if (!strcmp(key, "ctype"))
	{
		offset = offsetof(particle, ctype);
		*format = 0;
	}
	else if (!strcmp(key, "temp"))
	{
		offset = offsetof(particle, temp);
		*format = 1;
	}
	else if (!strcmp(key, "tmp"))
	{
		offset = offsetof(particle, tmp);
		*format = 0;
	}
	else if (!strcmp(key, "tmp2"))
	{
		offset = offsetof(particle, tmp2);
		*format = 0;
	}
	else if (!strcmp(key, "vy"))
	{
		offset = offsetof(particle, vy);
		*format = 1;
	}
	else if (!strcmp(key, "vx"))
	{
		offset = offsetof(particle, vx);
		*format = 1;
	}
	else if (!strcmp(key, "x"))
	{
		offset = offsetof(particle, x);
		*format = 1;
	}
	else if (!strcmp(key, "y")) {
		offset = offsetof(particle, y);
		*format = 1;
	}
	else if (!strcmp(key, "dcolour"))
	{
		offset = offsetof(particle, dcolour);
		*format = 3;
	}
	else if (!strcmp(key, "dcolour"))
	{
		offset = offsetof(particle, dcolour);
		*format = 3;
	}
	else if (!strcmp(key, "flags"))
	{
		offset = offsetof(particle, flags);
		*format = 3;
	}
	else if (!strcmp(key, "pavg0"))
	{
		offset = offsetof(particle, pavg[0]);
		*format = 1;
	}
	else if (!strcmp(key, "pavg1"))
	{
		offset = offsetof(particle, pavg[1]);
		*format = 1;
	}
	else
	{
		offset = -1;
	}
	return offset;
}