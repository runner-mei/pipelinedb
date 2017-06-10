/*-------------------------------------------------------------------------
 *
 * assert.c
 *	  Assert code.
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2013-2015, PipelineDB
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/error/assert.c
 *
 * NOTE
 *	  This should eventually work with elog()
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#if _WIN32
#include <unistd.h>
#endif
#include "execinfo.h"
#include "miscadmin.h"
#include "tcop/tcopprot.h"

#ifdef _WIN32
#include <Windows.h>

void printStack( void )
{
     unsigned int   i;
     void         * stack[ 100 ];
     unsigned short frames;
     SYMBOL_INFO  * symbol;
     HANDLE         process;

     process = GetCurrentProcess();

     SymInitialize( process, NULL, TRUE );

     frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
     symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
     symbol->MaxNameLen   = 255;
     symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

     for( i = 0; i < frames; i++ )
     {
         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

         printf( "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address );
     }

     free( symbol );
}
#endif

/*
 * ExceptionalCondition - Handles the failure of an Assert()
 */
void
ExceptionalCondition(const char *conditionName,
					 const char *errorType,
					 const char *fileName,
					 int lineNumber)
{
	void *array[32];
	size_t size;

	if (!PointerIsValid(conditionName)
		|| !PointerIsValid(fileName)
		|| !PointerIsValid(errorType))
		write_stderr("TRAP: ExceptionalCondition: bad arguments\n");
	else
	{
		write_stderr("TRAP: %s(\"%s\", File: \"%s\", Line: %d, PID: %d, Query: %s)\n",
					 errorType, conditionName,
					 fileName, lineNumber, getpid(), debug_query_string);
	}

	/* dump stack trace */
	fprintf(stderr, "Assertion failure (PID %d)\n", MyProcPid);
	fprintf(stderr, "version: %s\n", PIPELINE_VERSION_STR);
	fprintf(stderr, "query: %s\n", debug_query_string);
	fprintf(stderr, "backtrace:\n");
#ifdef _WIN32
	printStack();
#else
	size = backtrace(array, 32);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif
	/* Usually this shouldn't be needed, but make sure the msg went out */
	fflush(stderr);

#ifdef SLEEP_ON_ASSERT

	/*
	 * It would be nice to use pg_usleep() here, but only does 2000 sec or 33
	 * minutes, which seems too short.
	 */
	sleep(1000000);
#endif

	abort();
}
