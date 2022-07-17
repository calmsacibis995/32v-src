/*
 *	Memory special file
 *	minor device 0 is physical memory
 *	minor device 1 is kernel memory 
 *	minor device 2 is EOF/RATHOLE
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/page.h"
#include "../h/mtpr.h"

int mmapinuse, mmapwant;

mmread(dev)
{
	register char *p;
	register c;
	extern int *mmap;
	extern char *vmmap;

	if(minor(dev) == 2)
		return;
	if( minor(dev) == 1 ) {
		if( kernacc(u.u_offset, u.u_count, B_READ)
	    && !copyout(u.u_offset, u.u_base, u.u_count) ) {
		c = u.u_count;
		u.u_count = 0;
		u.u_base += c;
		u.u_offset += c;
	} else 
		u.u_error = EFAULT;
	return;
	}

	if( minor(dev) == 0) {

		spl7();
		while( mmapinuse ) {
			mmapwant++;
			sleep(&mmapwant, PRIBIO);
		}
		mmapinuse = 1;
		spl0();
		while (u.u_count > 0 && u.u_error == 0) {
			c = (int) u.u_offset >> 9;
			if( c < 0 || c > PHYSPAGES ) {
				u.u_error = EFAULT;
				break;
			}
			*mmap = c | (PG_V | PG_KR);
			mtpr(TBIS, vmmap);
			copyout(vmmap+((int)u.u_offset & 0x1ff), u.u_base, c=min(512-((int)u.u_offset & 0x1ff), u.u_count));
			u.u_count -= c;
			u.u_base += c;
			u.u_offset += c;
		}
		spl7();
		mmapinuse = 0;
		if( mmapwant ) {
			mmapwant = 0;
			wakeup( &mmapwant );
		}
		spl0();
	return;
	}
 
if (minor(dev) == 3) { /* UNIBUS access */
	if ((!kernacc(u.u_offset,u.u_count,B_READ)) ||
	(!useracc(u.u_base,u.u_count,B_READ)) ||
	UNIcpy(u.u_offset,u.u_base,u.u_count,B_READ))
		u.u_error = EFAULT;
	else {
		u.u_offset += u.u_count;
		u.u_base += u.u_count;
		u.u_count = 0;
		}
	return;
	}
}

mmwrite(dev)
{
	register char *p;
	register c;

	if(minor(dev) == 2) {
		c = u.u_count;
		u.u_count = 0;
		u.u_offset += c;
		return;
	}
	/*  kernel virt mem */
	if (minor(dev) == 1) {
		if ((!kernacc(u.u_offset,u.u_count,B_WRITE)) || copyin(u.u_base,u.u_offset,u.u_count))
			u.u_error = EFAULT;
		else {
			u.u_offset += u.u_count;
			u.u_base += u.u_count;
			u.u_count = 0 ;
			}
		return;
		}
 
	if (minor(dev) == 3) { /* UNIBUS access */
		if ((!kernacc(u.u_offset,u.u_count,B_WRITE)) ||
		(!useracc(u.u_base,u.u_count,B_WRITE)) ||
		UNIcpy(u.u_offset,u.u_base,u.u_count,B_WRITE))
			u.u_error = EFAULT;
		else {
			u.u_offset += u.u_count;
			u.u_base += u.u_count;
			u.u_count = 0;
			}
		return;
		}
}
