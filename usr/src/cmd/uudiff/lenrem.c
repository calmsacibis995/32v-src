main(argc,argv)
	char *argv[];
{
/* args: 1 is name to fix, 2 is this directory */
if (argv[1][0] == '/')
	printf("%s\n",argv[1]);
else
	printf("%s/%s\n", argv[2], argv[1]);
}
