#include "head.h"

/* decode() - read a line from standard input and decode it */

decode() {
	register char c, *q;
	char *p;
	integ = scallf = reflag = 0;
	proc[0] = cmd = args[0] = var[0] = '\0';
	argsp = args;
	
	p = readline(stdin);
	
/*
	if (*p == '/'  &&  *(p+1) == '\n') {
		cmd = '/';
		return(0);
	}
*/
	
	if (eqany(*p, "/?")) {	/* regular expression */
		c = *p;
		redir = (c == '/');
		reflag = 1;
		p++;
		if (*p == '\n' || *p == c) return(0);
		q = re;
		while(*p != c && *p != '\n') *q++ = *p++;
		*q = '\0';
		return(0);
	}
	
	if (*p == '!') { /* shell escape */
		for (q = p; *q != '\n'; q++) ;
		*q = '\0';
		system(p+1);
		return(0);
	}
	
	if (*p == '\n') {
		cmd = '\n';
		return(0);
	}
	
	while (*p != '\n') {	/* decode item by item */
	
		if (number(*p)) {	/* decimal number */
			if(integ) {
				error("Too many numbers");
				return(1);
			}
			integ = readint(&p);
			continue;
		}
		
		if (varchar(*p) || eqany(*p, COMMANDS)) { 
					/* proc, variable or command */
			if (cmd != '\0') {
				p = cpall(args, p);
				continue;
			}
			q = p;
			while (varchar(*q) || number(*q) || eqany(*q,COMMANDS))
				q++;
			if (*q == '(') {	/* procedure call */
				if (proc[0] != '\0') {
					error("Too many procedure calls");
					return(1);
				}
				scallf = 1;
				p = cpname(proc, p);
				p = cpall(args, p);
				continue;
			}
			if (*q == ':') {	/* procedure name */
				p = cpname(proc, p);
				continue;
			}
			if (*q == '$') {	/* variable name */
				p = cpname(var, p);
				continue;
			}
			if ((q-p == 1 && eqany(*p,COMMANDS) && 
				(proc[0]=='\0' || *p == 'b')) ||
				integ || eqany(*p, "+-")) 
							{  /* command */
				cmd = *p++;
				continue;
			}
			/* otherwise, its a variable */
			if (var[0] != '\0') {
				error("Too many variable names");
				return(1);
			}
			p = cpname(var, p);
			if (*p == '\n') {
				cmd = '/';
				continue;
			}
			if (cmd == '\0') cmd = *p ? *p : '/';
			p++;
			p = cpall(args,p);
			continue;
		}
		p++;	/* otherwise ignore p */
	}
	return(0);
}
