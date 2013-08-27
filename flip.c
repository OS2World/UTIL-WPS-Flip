/* VIEW WITH TAB STOPS = 4 */
#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#define  _MEERROR_H_
#include <mmioos2.h>
#include <dive.h>
#include <fourcc.h>

int main(int argc, char **argv)
{
	HAB 			hab;
	HDIVE			hdive;
	DIVE_CAPS		caps;
	LONG			banklines;
	LONG			lines;
	LONG			banklinesleft;
	LONG			linesize;
	LONG			banknum;
	LONG			half;
	LONG			currline;
	LONG			copyline;
	double			v;
	double			shrink;
	double			step;
	LONG			steps;
	unsigned char	*video;
	unsigned char	*buffer;
	unsigned char	*ptr;
	RECTL			rcl;

	/* Number of frames
	 */
	if(argc == 1 || (steps = atol(argv[1])) == 0)
		steps = 50;

	hab = WinInitialize(0L);

	/* Query DIVE capabilities on this system
	 */
	memset(&caps, 0, sizeof(caps));
	caps.ulStructLen = sizeof(caps);
	DiveQueryCaps(&caps, DIVE_BUFFER_SCREEN);

	if(! caps.fScreenDirect || DiveOpen(&hdive, FALSE, &video) != 0)
		WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
			"Cannot open DIVE", "Flip", 0, MB_OK | MB_MOVEABLE);
	else
	{
		/* A few useful parameters
		 */
		banklines = caps.fBankSwitched ?
						caps.ulApertureSize / caps.ulScanLineBytes :
						caps.ulVerticalResolution;
		linesize = caps.ulHorizontalResolution * (caps.ulDepth / 8);

		if((buffer = malloc(caps.ulVerticalResolution * linesize)) != 0)
		{
			/* Copy screen content
			 */
			rcl.yBottom = 0;
			rcl.yTop = caps.ulVerticalResolution - 1;
			rcl.xLeft = 0;
			rcl.xRight = caps.ulHorizontalResolution - 1;

			/* Access screen memory
			 */
			if(DiveAcquireFrameBuffer(hdive, &rcl) == 0)
			{
				DiveSwitchBank(hdive, banknum = 0);
				ptr = video;
				banklinesleft = banklines;
				for(lines = 0; lines < caps.ulVerticalResolution; lines++)
				{
					/* The lines in the buffer are numbered
					 * from 0 (top) to caps.ulVerticalResoultion - 1 (bottom)
					 */
					memcpy(buffer + lines * linesize, ptr, linesize);

					ptr += caps.ulScanLineBytes;

					/* Do a bank switch when all the lines in the
					 * current bank have been accessed.
					 */
					if(--banklinesleft == 0)
					{
						banklinesleft = banklines;
						ptr = video;
						DiveSwitchBank(hdive, ++banknum);
					}
				}

				/* Flip screen
				 */
				half = caps.ulVerticalResolution / 2;
				step = 6.28 / (double)steps;
				for(v = 0; v < 6.28; v += step)
				{
					/* Calculate shrink factor, except for
					 * last step to put everything back in place
					 */
					shrink = (v + step < 6.28) ? (1. / cos(v)) : 1.;

					DiveSwitchBank(hdive, banknum = 0);
					ptr = video;
					banklinesleft = banklines;
					currline = half;
					for(lines = 0; lines < caps.ulVerticalResolution; lines++)
					{
						/* The line to copy is obtained considering the
						 * lines on the screen numbered from +half (top)
						 * to -half (bottom), line 0 being the center one.
						 * The currline variable does that. Multiplying
						 * the current line by the shrink factor, the source
						 * line is obtained in the same representation.
						 * The subtraction puts it back in the 0 to
						 * caps.ulVerticalResoultion - 1 scale.
						 */
						copyline = half - ((double)currline * shrink);

						/* If the source line is off screen the target
						 * line is filled with zeros (hoping it's black
						 * or the palette index for black).
						 */
						if(copyline >= 0 && copyline < caps.ulVerticalResolution)
							memcpy(ptr, buffer + copyline * linesize, linesize);
						else
							memset(ptr, 0, linesize);
						currline--;

						ptr += caps.ulScanLineBytes;

						/* Do a bank switch when all the lines in the
						 * current bank have been accessed.
						 */
						if(--banklinesleft == 0)
						{
							banklinesleft = banklines;
							ptr = video;
							DiveSwitchBank(hdive, ++banknum);
						}
					}
				}

				/* Restore screen memory access
				 */
				DiveDeacquireFrameBuffer(hdive);
			}
			else
				WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
					"Cannot acquire frame buffer", "Flip", 0, MB_OK | MB_MOVEABLE);

			free(buffer);
		}
		else
			WinMessageBox(HWND_DESKTOP, HWND_DESKTOP,
				"Cannot allocate memory for video image", "Flip", 0, MB_OK | MB_MOVEABLE);

		DiveClose(hdive);
	}
	 
	WinTerminate(hab);

	return 0;
}

