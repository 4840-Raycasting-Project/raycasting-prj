#include "mazes.h"

//types: B: bluestone, C: colorstone, E: eagle, G: greystone, M: mossy, P: purplestone, R: redbrick, W: wood

maze_t mazes[3] = {
	{ 12, 12, 144, "CESPR",
		{
			G,G,G,G,G,G,G,G,G,G,G,G,
			G,O,O,O,O,O,O,O,O,O,O,G,
			G,O,O,O,O,O,O,O,O,O,O,G,
			G,O,O,O,O,O,O,O,C,O,O,G,
			G,O,O,C,O,C,O,O,C,O,O,G,
			G,O,O,C,O,C,C,O,C,O,O,G,
			G,O,O,C,O,O,C,O,C,O,O,G,
			G,O,O,O,C,O,C,O,C,C,C,G,
			G,O,O,O,C,O,C,O,O,O,O,G,
			G,O,O,O,C,C,C,C,C,O,O,G,
			G,O,O,O,O,O,O,O,O,O,O,E,
			G,G,G,G,G,G,G,G,G,G,G,G
		}
	},
	{ 24, 24, 576, "MUDD",
		{
			R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
			R,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,R,
			R,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,R,
			R,O,O,O,O,O,O,O,O,O,O,O,W,O,W,W,W,W,W,W,O,O,O,R,
			R,O,O,O,O,W,O,O,O,O,W,O,W,O,O,O,O,O,O,W,O,O,O,R,
			R,O,O,W,O,W,W,O,W,O,W,O,W,O,O,O,O,O,O,W,O,O,O,R,
			R,O,O,W,O,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,O,O,O,R,
			R,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,R,
			R,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,W,R,
			R,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,R,
			R,O,O,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,O,O,R,
			R,O,O,O,W,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,O,W,W,R,
			R,O,W,O,W,O,W,O,W,O,O,O,W,O,O,O,O,O,W,O,O,O,O,R,
			R,O,W,O,W,O,W,O,W,W,W,W,W,W,W,W,O,O,W,O,O,O,O,R,
			R,O,W,O,W,O,W,O,O,O,O,O,O,W,O,W,O,O,W,O,O,O,O,R,
			R,O,W,O,W,O,O,W,W,W,W,W,W,W,O,W,O,O,W,O,O,O,O,R,
			R,O,W,O,W,O,O,W,O,O,O,O,O,O,O,W,O,O,W,W,W,W,W,R,
			R,O,O,O,W,O,O,W,O,O,O,O,O,O,O,W,W,O,O,O,O,O,O,R,
			R,O,O,O,W,W,W,W,O,O,O,W,O,O,O,O,O,R,O,O,O,O,O,R,
			R,O,W,O,O,O,O,O,O,O,O,W,O,O,O,W,O,W,O,O,O,O,O,E,
			R,O,W,O,O,O,O,O,O,O,O,W,O,O,O,W,O,W,O,O,O,O,O,R,
			R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R
		}
	},
	{ 36, 36, 1296, "PUPIN",
		
		{
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,
			B,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,B,
			B,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,B,
			B,O,O,O,O,O,O,O,O,O,O,O,M,O,M,M,M,M,M,M,O,O,O,O,O,O,M,M,M,M,M,M,M,M,O,B,
			B,O,O,O,O,M,O,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,B,
			B,O,O,M,O,M,M,O,M,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,M,M,M,M,M,M,M,B,
			B,O,O,M,O,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,M,O,O,O,M,O,O,M,O,M,M,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,M,M,M,M,M,M,O,O,M,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,M,M,M,M,M,M,M,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,O,O,O,O,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,M,O,M,O,O,M,M,M,M,M,M,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,M,M,M,M,O,O,O,M,O,O,M,O,O,M,O,O,B,
			B,O,O,O,M,O,O,M,O,O,O,O,O,O,O,M,M,O,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,B,
			B,O,O,O,M,M,M,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,B,
			B,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,M,M,B,
			B,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,M,M,M,M,M,O,O,M,O,O,O,O,O,B,
			B,M,M,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,O,M,M,M,M,M,O,O,M,O,O,B,
			B,O,O,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,M,M,O,O,M,O,O,O,O,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,M,M,M,M,M,M,M,M,O,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,B,
			B,O,M,O,M,O,M,O,O,O,O,O,O,M,O,M,O,O,M,O,O,O,O,O,O,M,M,M,M,M,M,M,M,M,M,B,
			B,O,M,O,M,O,O,M,M,M,M,M,M,M,O,M,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,B,
			B,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,O,M,M,M,M,M,M,M,O,O,O,O,O,O,O,O,O,B,
			B,O,O,O,M,O,O,M,O,O,O,O,O,O,O,M,M,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,B,
			B,O,O,O,M,M,M,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,O,M,M,M,O,O,O,O,B,
			B,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,M,O,O,O,O,B,
			B,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,M,O,O,O,O,E,
			B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B
		}
	}

};