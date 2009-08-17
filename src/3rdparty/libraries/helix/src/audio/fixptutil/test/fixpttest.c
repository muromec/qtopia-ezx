#include <stdio.h>
#include <stdlib.h>

#define TIMING 1
#include "math64.h"

int NTESTS = 100000 ;
#define N 1024

int a[N], b[N], c[N] ;
int tests, i;

void randomize(int a[])
{
	int i ;
	for (i=0; i < N; i++)
		a[i] = (rand() << 16) ;
}

int test_window(void)
{
	for (i = 0; i < N; i++)
	{
		c[i] = MulShift32(a[i], b[i]) ;
	}
	for (i = 0; i < N; i++)
	{
		if (c[i] != (a[i] >> 16) * (b[i] >> 16))
		{
			return 1 ;
		}
	}
	return 0 ;
}

void time_window(void)
{
	printf("time window: ");
	TICK() ;
	for (tests=0; tests < NTESTS; tests++)
	{
		for (i = 0; i < N; i++)
		{
			c[i] = MulShift32(a[i], b[i]) ;
		}
	}
	TOCK(N*NTESTS) ;
	return ;
}

int test_commutator(void)
{
	for (i = 0; i < N; i++)
	{
		c[i] = MulShift32(a[i], b[i]) - MulShift32(b[i],a[i]) ;
	}
	for (i = 0; i < N; i++)
	{
		if (c[i] != 0)
		{
			return 1 ;
		}
	}
	return 0 ;
}

void time_commutator(void)
{
	printf("time commutator: ");
	TICK() ;
	for (tests=0; tests < NTESTS; tests++)
	{
		for (i = 0; i < N; i++)
		{
			c[i] = MulShift32(a[i], b[i]) - MulShift32(b[i],a[i]) ;
		}
	}
	TOCK(N * NTESTS) ;
}

int test_smul(void)
{
	for (tests=0; tests < NTESTS; tests++)
	{
		int b = rand() << 16 ;
		for (i = 0; i < N; i++)
		{
			c[i] = MulShift32(a[i], b) ;
		}
		for (i = 0; i < N; i++)
		{
			if (c[i] != (a[i] >> 16) * (b >> 16))
			{
				return 1 ;
			}
		}
	}
	return 0 ;
}

void time_smul(void)
{
	printf("time left mul: ");
	TICK() ;
	for (tests=0; tests < NTESTS; tests++)
	{
		int b = rand() << 16 ;
		for (i = 0; i < N; i++)
		{
			c[i] = MulShift32(a[i], b) ;
		}
	}
	TOCK(NTESTS * N) ;

	printf("time right mul: ");
	TICK() ;
	for (tests=0; tests < NTESTS; tests++)
	{
		int b = rand() << 16 ;
		for (i = 0; i < N; i++)
		{
			c[i] = MulShift32(b, a[i]) ;
		}
	}
	TOCK(NTESTS * N) ;
}

int test_mulshiftN(void)
{
	for (tests=0; tests < NTESTS; tests++)
	{
		int a = (rand() & 0xffff) << 15 ;
		int b = (rand() & 0xffff) << 14 ;
		int a32 = MulShift32(a,b) ;
		int a31 = MulShift31(a,b) ;
		int a30 = MulShift30(a,b) ;

		int b32 = MulShiftN(a,b,32) ;
		int b31 = MulShiftN(a,b,31) ;
		int b30 = MulShiftN(a,b,30) ;

		if (a32 != a30 >> 2 || a31 != a30 >> 1 ||
		    (b30 ^ a30) > 3 || (b31 ^ a31) > 3)
			return 1 ;
	}
	return 0 ;
}

void time_muldiv(void)
{
	printf("time MulDiv64: ");
	TICK() ;
	for (tests=0; tests < NTESTS; tests++)
	{
		for (i = 0; i < N; i++)
		{
			c[i] = MulDiv64(a[i],b[i],(1UL<<31)-1) ;
		}
	}
	TOCK(NTESTS * N) ;
}

int main(int ac, char *av[])
{
	char *msg[2] = {"passed","failed"} ;
	int err ;

	randomize(a) ;
	randomize(b) ;

	printf("commutator test: %s\n", msg[err = test_commutator()]);
	if (err)
		return 20 ;

	printf("smul test: %s\n", msg[err = test_smul()]);
	if (err)
		return 20 ;

	printf("window test: %s\n", msg[err = test_window()]);
	if (err)
		return 20 ;

	printf("mulshiftN test: %s\n", msg[err = test_mulshiftN()]);
	if (err)
		return 20 ;

	time_commutator() ;
	time_smul() ;
	time_window() ;
	time_muldiv() ;
	return 0 ;
}

