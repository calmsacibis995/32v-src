#
/*
 *
 *	UNIX debugger
 *
 */

#include "head.h"
#include <a.out.h>
struct user u;
#include <stdio.h>


MSG		NOFORK;
MSG		ENDPCS;
MSG		BADWAIT;

ADDR		sigint;
ADDR		sigqit;

/* breakpoints */
BKPTR		bkpthead;


REGLIST reglist [] = {
		"p1lr", P1LR,
		"p1br",P1BR,
		"p0lr", P0LR,
		"p0br",P0BR,
		"ksp",KSP,
		"esp",ESP,
		"ssp",SSP,
		"psl", PSL,
		"pc", PC,
		"usp",USP,
		"fp", FP,
		"ap", AP,
		"r11", R11,
		"r10", R10,
		"r9", R9,
		"r8", R8,
		"r7", R7,
		"r6", R6,
		"r5", R5,
		"r4", R4,
		"r3", R3,
		"r2", R2,
		"r1", R1,
		"r0", R0,
};


CHAR		lastc;

INT		fcor;
INT		fsym;
STRING		errflg;
int		errno;
INT		signo;

L_INT		dot;
STRING		symfil;
INT		wtflag;
INT		pid;
INT		adrflg;
L_INT		loopcnt;






getsig(sig)
{	return(sig);
}

ADDR userpc = 1;

runpcs(runmode,execsig)
{
	REG BKPTR	bkpt;
	IF adrflg THEN userpc=dot; FI
/*
	printf("%s: running\n", symfil);
*/
	WHILE --loopcnt>=0
	DO
		if (debug) printf("\ncontinue %x %d\n",userpc,execsig);
		IF runmode==SINGLE
		THEN delbp(); /* hardware handles single-stepping */
		ELSE /* continuing from a breakpoint is hard */
			IF bkpt=scanbkpt(userpc)
			THEN execbkpt(bkpt,execsig); execsig=0;
			FI
			setbp();
		FI
		ptrace(runmode,pid,userpc,execsig);
		bpwait(); chkerr(); execsig=0; delbp(); readregs();

		IF (signo==0) ANDF (bkpt=scanbkpt(userpc))
		THEN /* stopped by BPT instruction */
			if (debug) printf("\n BPT code; '%s'%o'%o'%d",
				bkpt->comm,bkpt->comm[0],EOR,bkpt->flag);
			dot=bkpt->loc;
			IF bkpt->flag==BKPTEXEC
			ORF ((bkpt->flag=BKPTEXEC)
				ANDF bkpt->comm[0]!=EOR
				ANDF --bkpt->count)
			THEN execbkpt(bkpt,execsig); execsig=0; loopcnt++;
			ELSE bkpt->count=bkpt->initcnt;
			FI
		ELSE execsig=signo;
		FI
	OD
 		if (debug) printf("Returning from runpcs\n");
}

#define BPOUT 0
#define BPIN 1
INT bpstate = BPOUT;

endpcs()
{
	REG BKPTR	bkptr;
 		if (debug) printf("Entering endpcs with pid=%d\n");
	IF pid
	THEN ptrace(EXIT,pid,0,0); pid=0; userpc=1;
	     FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	     DO IF bkptr->flag
		THEN bkptr->flag=BKPTSET;
		FI
	     OD
	FI
	bpstate=BPOUT;
}

setup()
{
	close(fsym); fsym = -1;
	IF (pid = fork()) == 0
	THEN ptrace(SETTRC,0,0,0);
	     signal(SIGINT,sigint); signal(SIGQUIT,sigqit);
 		if (debug) printf("About to doexec  pid=%d\n",pid);
	     doexec(); exit(0);
	ELIF pid == -1
	THEN error(NOFORK);
	ELSE bpwait(); readregs(); /******** lp[0]=EOR; lp[1]=0; */
	if (debug) printf("About to open symfil = %s\n", symfil);
	     fsym=open(symfil,wtflag);
	     IF errflg
	     THEN printf("%s: cannot execute\n",symfil);
 		if (debug) printf("%d %s\n", errflg, errflg);
		  endpcs();
	     FI
	FI
	bpstate=BPOUT;
}

execbkpt(bkptr,execsig)
BKPTR	bkptr;
{
	if (debug) printf("exbkpt: %d\n",bkptr->count);
	delbp();
	ptrace(SINGLE,pid,bkptr->loc,execsig);
	bkptr->flag=BKPTSET;
	bpwait(); chkerr(); readregs();
}

extern STRING environ;

doexec()
{
	STRING		argl[MAXARG];
	CHAR		args[LINSIZ];
	STRING		p, *ap, filnam;
	ap=argl; p=args;
	*ap++=symfil;
	REP	IF rdc()==EOR THEN break; FI
		*ap = p;
		WHILE lastc!=EOR ANDF lastc!=SP ANDF lastc!=TB DO *p++=lastc; readchar(); OD
		*p++=0; filnam = *ap+1;
		IF **ap=='<'
		THEN	close(0);
			IF open(filnam,0)<0
			THEN	printf("%s: cannot open\n",filnam); exit(0);
			FI
		ELIF **ap=='>'
		THEN	close(1);
			IF creat(filnam,0666)<0
			THEN	printf("%s: cannot create\n",filnam); exit(0);
			FI
		ELSE	ap++;
		FI
	PER lastc!=EOR DONE
	*ap++=0;
	if (debug) printf("About to exect(%s, %d, %d)\n",symfil,argl,environ);
	exect(symfil, argl, environ);
	perror("Returned from exect");
}

BKPTR	scanbkpt(adr)
ADDR adr;
{
	REG BKPTR	bkptr;
	FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
	DO IF bkptr->flag ANDF bkptr->loc==adr
	   THEN break;
	   FI
	OD
	return(bkptr);
}

delbp()
{
	REG ADDR	a;
	REG BKPTR	bkptr;
	IF bpstate!=BPOUT
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO	IF bkptr->flag
			THEN a=bkptr->loc;
				ptrace(WIUSER,pid,a,
					(bkptr->ins&0xFF)|(ptrace(RIUSER,pid,a,0)&~0xFF));
			FI
		OD
		bpstate=BPOUT;
	FI
}

setbp()
{
	REG ADDR		a;
	REG BKPTR	bkptr;

	IF bpstate!=BPIN
	THEN
		FOR bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt
		DO IF bkptr->flag
		   THEN a = bkptr->loc;
			bkptr->ins = ptrace(RIUSER, pid, a, 0);
			ptrace(WIUSER, pid, a, BPT | (bkptr->ins&~0xFF));
			IF errno
			THEN error("cannot set breakpoint: ");
			     printf("%s:%d @ %d\n", adrtoprocp(dot)->pname,
				adrtolineno(dot), dot);
/********
			     psymoff(bkptr->loc,ISYM,"\n");
*/
			FI
		   FI
		OD
		bpstate=BPIN;
	FI
}

bpwait()
{
	REG ADDR w;
	ADDR stat;

	signal(SIGINT, 1);
	if (debug) printf("Waiting for pid %d\n",pid);
	WHILE (w = wait(&stat))!=pid ANDF w != -1 DONE
	if (debug) printf("Ending wait\n");
	if (debug) printf("w = %d; pid = %d; stat = %o;\n", w,pid,stat);
	signal(SIGINT,sigint);
	IF w == -1
	THEN pid=0;
	     errflg=BADWAIT;
	ELIF (stat & 0177) != 0177
	THEN IF signo = stat&0177
	     THEN sigprint();
	     FI
	     IF stat&0200
	     THEN error(" - core dumped");
		  close(fcor);
		  setcor();
	     FI
	     pid=0;
	     errflg=ENDPCS;
	ELSE signo = stat>>8;
    	     if (debug) printf("PC = %d, dbsubn = %d\n",
		ptrace(RUREGS, pid, PC, 0), extaddr("_dbsubn")); 
	     IF signo!=SIGTRC ANDF
		ptrace(RUREGS, pid, PC, 0) != extaddr("_dbsubn")
	     THEN sigprint();
	     ELSE signo=0;
	     FI
	FI
}

readregs()
{
	/*get REG values from pcs*/
	REG i;
	FOR i=24; --i>=0; 
	DO *(ADDR *)(((ADDR)&u)+reglist[i].roffs) =
		    ptrace(RUREGS, pid, reglist[i].roffs, 0);
	OD
 	userpc= *(ADDR *)(((ADDR)&u)+PC);
}

rdc()
{	REP	readchar();
	PER	lastc==SP ORF lastc==TB
	DONE
	return(lastc);
}

readchar() {
	lastc = *argsp++;
	if (lastc == '\0') lastc = '\n';
}
