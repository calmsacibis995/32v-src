struct map
{
	short	m_size;
	unsigned short m_addr;
};

struct map coremap[CMAPSIZ];	/* space for core allocation */
struct map swapmap[SMAPSIZ];	/* space for swap allocation */
/*
	uba
*/
char bdpwant , umrwant ;
struct map ubamap[UAMSIZ];
struct map bdpmap[15];
