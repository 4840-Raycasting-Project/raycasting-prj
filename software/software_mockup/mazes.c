#include "mazes.h"

//types: B: bluestone, C: colorstone, E: eagle, G: greystone, M: mossy, P: purplestone, R: redbrick, W: wood

maze_t mazes[3] = {
	{ 12, 12, 144, "CESPR",
		{
			G,G,G,G,G,G,G,G,G,G,G,G,
			G,O,O,O,O,O,O,O,O,O,O,G,
			G,O,O,O,O,O,O,O,O,O,O,G,
			G,O,O,O,O,O,O,O,G,O,O,G,
			G,O,O,G,O,G,O,O,G,O,O,G,
			G,O,O,G,O,G,G,O,G,O,O,G,
			G,O,O,G,O,O,G,O,G,O,O,G,
			G,O,O,O,G,O,G,O,G,G,G,G,
			G,O,O,O,G,O,G,O,O,O,O,G,
			G,O,O,O,G,G,G,G,G,O,O,G,
			G,O,O,O,O,O,O,O,O,O,O,E,
			G,G,G,G,G,G,G,G,G,G,G,G
		}
	},
	{ 24, 24, 576, "MUDD",
		{
			W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,
			W,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,W,
			W,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,W,
			W,O,O,O,O,O,O,O,O,O,O,O,W,O,W,W,W,W,W,W,O,O,O,W,
			W,O,O,O,O,W,O,O,O,O,W,O,W,O,O,O,O,O,O,W,O,O,O,W,
			W,O,O,W,O,W,W,O,W,O,W,O,W,O,O,O,O,O,O,W,O,O,O,W,
			W,O,O,W,O,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,O,O,O,W,
			W,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,W,
			W,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,W,W,
			W,O,O,O,W,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,W,
			W,O,O,O,W,O,W,O,W,O,W,O,W,O,O,O,O,O,O,O,O,O,O,W,
			W,O,O,O,W,O,W,O,W,O,W,O,W,O,W,W,W,W,W,W,O,W,W,W,
			W,O,W,O,W,O,W,O,W,O,O,O,W,O,O,O,O,O,W,O,O,O,O,W,
			W,O,W,O,W,O,W,O,W,W,W,W,W,W,W,W,O,O,W,O,O,O,O,W,
			W,O,W,O,W,O,W,O,O,O,O,O,O,W,O,W,O,O,W,O,O,O,O,W,
			W,O,W,O,W,O,O,W,W,W,W,W,W,W,O,W,O,O,W,O,O,O,O,W,
			W,O,W,O,W,O,O,W,O,O,O,O,O,O,O,W,O,O,W,W,W,W,W,W,
			W,O,O,O,W,O,O,W,O,O,O,O,O,O,O,W,W,O,O,O,O,O,O,W,
			W,O,O,O,W,W,W,W,O,O,O,W,O,O,O,O,O,R,O,O,O,O,O,W,
			W,O,W,O,O,O,O,O,O,O,O,W,O,O,O,W,O,W,O,O,O,O,O,E,
			W,O,W,O,O,O,O,O,O,O,O,W,O,O,O,W,O,W,O,O,O,O,O,W,
			W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W
		}
	},
	{ 36, 36, 1296, "PUPIN",
		
		{
			M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,
			M,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,M,
			M,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,M,
			M,O,O,O,O,O,O,O,O,O,O,O,M,O,M,M,M,M,M,M,O,O,O,O,O,O,M,M,M,M,M,M,M,M,O,M,
			M,O,O,O,O,M,O,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,
			M,O,O,M,O,M,M,O,M,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,M,M,M,M,M,M,M,M,
			M,O,O,M,O,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,M,O,O,O,M,O,O,M,O,M,M,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,M,M,M,M,M,M,O,O,M,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,M,M,M,M,M,M,M,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,O,O,O,O,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,M,O,M,O,O,M,M,M,M,M,M,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,M,M,M,M,M,O,O,O,M,O,O,M,O,O,M,O,O,M,
			M,O,O,O,M,O,O,M,O,O,O,O,O,O,O,M,M,O,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,M,
			M,O,O,O,M,M,M,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,M,
			M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,M,M,M,
			M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,M,M,M,M,M,O,O,M,O,O,O,O,O,M,
			M,M,M,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,O,O,O,O,O,O,O,O,O,O,O,M,M,M,M,M,O,O,M,O,O,M,
			M,O,O,O,M,O,M,O,M,O,M,O,M,O,M,M,M,M,M,M,O,M,M,O,O,M,O,O,O,O,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,M,M,M,M,M,M,M,M,O,O,M,O,O,O,O,O,O,O,O,O,O,O,O,O,M,O,O,M,
			M,O,M,O,M,O,M,O,O,O,O,O,O,M,O,M,O,O,M,O,O,O,O,O,O,M,M,M,M,M,M,M,M,M,M,M,
			M,O,M,O,M,O,O,M,M,M,M,M,M,M,O,M,O,O,M,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,M,
			M,O,M,O,M,O,O,M,O,O,O,O,O,O,O,M,O,O,O,M,M,M,M,M,M,M,O,O,O,O,O,O,O,O,O,M,
			M,O,O,O,M,O,O,M,O,O,O,O,O,O,O,M,M,O,O,O,O,O,O,O,O,O,M,O,O,O,O,O,O,O,O,M,
			M,O,O,O,M,M,M,M,O,O,O,M,O,O,O,O,O,M,O,O,O,O,O,O,O,O,O,O,M,M,M,O,O,O,O,M,
			M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,M,O,O,O,O,M,
			M,O,M,O,O,O,O,O,O,O,O,M,O,O,O,M,O,M,O,O,O,O,O,O,M,O,O,O,O,O,M,O,O,O,O,E,
			M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M,M
		}
	}

};
