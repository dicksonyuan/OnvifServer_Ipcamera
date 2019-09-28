#include <stdio.h>
#include <string.h>
#include "packbits.h"

int PackBitsDecode(unsigned char* rawcp
                   , int rawcc
                   , tidata_t op
                   , tsize_t opSize
                   , tsample_t s)
{
	char* bp;
	tsize_t cc;
	long n;
	int b;
	tsize_t occ = opSize;
	(void) s;   // define but not use
	bp = (char*)rawcp; //(char*) tif->tif_rawcp;
	cc = rawcc;//tif->tif_rawcc;

	while (cc > 0 && (long)occ > 0)
	{
		n = (long) * bp++, cc--;

		/*
		 * Watch out for compilers that
		 * don't sign extend chars...
		 */
		if (n >= 128)
		{
			n -= 256;
		}

		if (n < 0)  		/* replicate next byte -n+1 times */
		{
			if (n == -128)  	/* nop */
			{
				continue;
			}

			n = -n + 1;

			if( occ < n )
			{
#if 0
				TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
				               "PackBitsDecode: discarding %ld bytes "
				               "to avoid buffer overrun",
				               n - occ);
#endif
				n = occ;
			}

			occ -= n;
			b = *bp++, cc--;

			while (n-- > 0)
			{
				*op++ = (tidataval_t) b;
			}
		}
		else  		/* copy next n+1 bytes literally */
		{
			if (occ < n + 1)
			{
#if 0
				TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
				               "PackBitsDecode: discarding %ld bytes "
				               "to avoid buffer overrun",
				               n - occ + 1);
#endif
				n = occ - 1;
			}

			memcpy(op, bp, ++n);//_TIFFmemcpy(op, bp, ++n);
			op += n;
			occ -= n;
			bp += n;
			cc -= n;
		}
	}

	//rawcp = (tidata_t) bp;
	//rawcc = cc;
	//tif->tif_rawcp = (tidata_t) bp;
	//tif->tif_rawcc = cc;
#if 0

	if (occ > 0)
	{
#if 0
		TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
		             "PackBitsDecode: Not enough data for scanline %ld",
		             (long) tif->tif_row);
#endif
		return (0);
	}

	return (1);
#endif
	return (opSize - occ);
}

int PackBitsEncode(unsigned char* rawcp
                   , int rawdatasize
                   , int rawcc
                   , tidata_t buf
                   , tsize_t cc
                   , tsample_t s)
{
	unsigned char* bp = (unsigned char*) buf;
	tidata_t op, ep, lastliteral;
	long n, slop;
	int b;
	enum { BASE, LITERAL, RUN, LITERAL_RUN } state;
	(void) s;
	op = rawcp;
	ep = rawcp + rawdatasize;
	state = BASE;
	lastliteral = 0;

	while (cc > 0)
	{
		/*
		 * Find the longest string of identical bytes.
		 */
		b = *bp++, cc--, n = 1;

		for (; cc > 0 && b == *bp; cc--, bp++)
		{
			n++;
		}

again:

		if (op + 2 >= ep)  		/* insure space for new data */
		{
			/*
			 * Be careful about writing the last
			 * literal.  Must write up to that point
			 * and then copy the remainder to the
			 * front of the buffer.
			 */
			if (state == LITERAL || state == LITERAL_RUN)
			{
				slop = op - lastliteral;
				rawcc += (lastliteral - rawcp);
				//if (!TIFFFlushData1(tif))
				//	return (-1);
				op = rawcp;

				while (slop-- > 0)
				{
					*op++ = *lastliteral++;
				}

				lastliteral = rawcp;
			}
			else
			{
				rawcc += op - rawcp;
				//if (!TIFFFlushData1(tif))
				//	return (-1);
				op = rawcp;
			}
		}

		switch (state)
		{
			case BASE:		/* initial state, set run/literal */
				if (n > 1)
				{
					state = RUN;

					if (n > 128)
					{
						*op++ = (tidata) - 127;
						*op++ = (tidataval_t) b;
						n -= 128;
						goto again;
					}

					*op++ = (tidataval_t)(-(n - 1));
					*op++ = (tidataval_t) b;
				}
				else
				{
					lastliteral = op;
					*op++ = 0;
					*op++ = (tidataval_t) b;
					state = LITERAL;
				}

				break;

			case LITERAL:		/* last object was literal string */
				if (n > 1)
				{
					state = LITERAL_RUN;

					if (n > 128)
					{
						*op++ = (tidata) - 127;
						*op++ = (tidataval_t) b;
						n -= 128;
						goto again;
					}

					*op++ = (tidataval_t)(-(n - 1));	/* encode run */
					*op++ = (tidataval_t) b;
				}
				else  			/* extend literal */
				{
					if (++(*lastliteral) == 127)
					{
						state = BASE;
					}

					*op++ = (tidataval_t) b;
				}

				break;

			case RUN:		/* last object was run */
				if (n > 1)
				{
					if (n > 128)
					{
						*op++ = (tidata) - 127;
						*op++ = (tidataval_t) b;
						n -= 128;
						goto again;
					}

					*op++ = (tidataval_t)(-(n - 1));
					*op++ = (tidataval_t) b;
				}
				else
				{
					lastliteral = op;
					*op++ = 0;
					*op++ = (tidataval_t) b;
					state = LITERAL;
				}

				break;

			case LITERAL_RUN:	/* literal followed by a run */

				/*
				 * Check to see if previous run should
				 * be converted to a literal, in which
				 * case we convert literal-run-literal
				 * to a single literal.
				 */
				if (n == 1 && op[-2] == (tidata) - 1 &&
				        *lastliteral < 126)
				{
					state = (((*lastliteral) += 2) == 127 ?
					         BASE : LITERAL);
					op[-2] = op[-1];	/* replicate */
				}
				else
				{
					state = RUN;
				}

				goto again;
		}
	}

	//rawcc += op - rawcp;
	//rawcp = op;
	return (op - rawcp);
}


