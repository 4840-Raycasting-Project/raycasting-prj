/*
 * Avalon memory-mapped peripheral that generates VGA
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * Coompleted by: Souryadeep Sen (UNI: ss6400)
 */

module vga_ball(input logic        clk,
	        input logic 	   reset,
		input logic [7:0]  writedata,		
		input logic 	   write,
		input 		   chipselect,
		input logic [2:0]  address,		

		output logic [7:0] VGA_R, VGA_G, VGA_B,
		output logic 	   VGA_CLK, VGA_HS, VGA_VS,
		                   VGA_BLANK_n,
		output logic 	   VGA_SYNC_n);

   logic [10:0]	   hcount;
   logic [9:0]     vcount;

   logic [7:0] 	   background_r, background_g, background_b;

   logic [7:0]     pos_xl, pos_xu;    
   logic [7:0]     pos_yl, pos_yu;

   logic [7:0]     zero_level_0;
   //logic [7:0]     zero_level_0_copy;

   logic [7:0]     arr[15:0] = '{8'h00, 8'h00, 8'h18, 8'h38, 8'h78, 8'h18, 8'h18, 8'h18, 8'h18, 8'h18, 8'h18, 8'h7e, 8'h00, 8'h00, 8'h00, 8'h00};
	
   vga_counters counters(.clk50(clk), .*);


   always_ff @(posedge clk)
     if (reset) begin
	background_r <= 8'h0;
	background_g <= 8'h0;
	background_b <= 8'h80;
	{pos_xu[1:0],pos_xl} <= 10'h28;	/*set a default location for the ball*/
	{pos_yu[1:0],pos_yl} <= 10'h28;
	zero_level_0 <= 8'b0000_0000;
	i <= 0;
     end else if (chipselect && write) begin
       case (address)
	 3'h0 : background_r <= writedata;
	 3'h1 : background_g <= writedata;
	 3'h2 : background_b <= writedata;
	 3'h3 : pos_yl <= writedata;
	 3'h4 : pos_yu <= writedata;
	 3'h5 : pos_xl <= writedata;
	 3'h6 : pos_xu <= writedata;
       endcase
     end else if ((vcount - {pos_yu[1:0],pos_yl}) >=0 && (vcount - {pos_yu[1:0],pos_yl}) < 16 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) < 0 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) >= 8) begin
	zero_level_0 <= arr[(vcount - {pos_yu[1:0],pos_yl})];
     end else if ((hcount[10:1] - {pos_xu[1:0],pos_xl}) >=0 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) < 8) begin
	zero_level_0 <= zero_level_0 >> 1;
     end

   always_comb begin
      {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};
      if (VGA_BLANK_n ) begin
	//if (vcount == {pos_yu[1:0],pos_yl} && (zero_level_0 & 8'b00000001) == 8'b00000001 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) >=0 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) < 8) begin 
	// {VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
	//end
	if ((vcount - {pos_yu[1:0],pos_yl}) >=0 && (vcount - {pos_yu[1:0],pos_yl}) < 16  && (hcount[10:1] - {pos_xu[1:0],pos_xl}) >=0 && (hcount[10:1] - {pos_xu[1:0],pos_xl}) < 8) begin 
	 if ((zero_level_0 & 8'b00000001) == 8'b00000001) begin
	 	{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
	 end else begin
	 	{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
	 end
	end
	else
	  {VGA_R, VGA_G, VGA_B} =
	     {background_r, background_g, background_b};
	end
   end

	       
endmodule

module vga_counters(
 input logic 	     clk50, reset,
 output logic [10:0] hcount,  // hcount[10:1] is pixel column
 output logic [9:0]  vcount,  // vcount[9:0] is pixel row
 output logic 	     VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n);

/*
 * 640 X 480 VGA timing for a 50 MHz clock: one pixel every other cycle
 * 
 * HCOUNT 1599 0             1279       1599 0
 *             _______________              ________
 * ___________|    Video      |____________|  Video
 * 
 * 
 * |SYNC| BP |<-- HACTIVE -->|FP|SYNC| BP |<-- HACTIVE
 *       _______________________      _____________
 * |____|       VGA_HS          |____|
 */
   // Parameters for hcount
   parameter HACTIVE      = 11'd 1280,
             HFRONT_PORCH = 11'd 32,
             HSYNC        = 11'd 192,
             HBACK_PORCH  = 11'd 96,   
             HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC +
                            HBACK_PORCH; // 1600
   
   // Parameters for vcount
   parameter VACTIVE      = 10'd 480,
             VFRONT_PORCH = 10'd 10,
             VSYNC        = 10'd 2,
             VBACK_PORCH  = 10'd 33,
             VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC +
                            VBACK_PORCH; // 525

   logic endOfLine;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          hcount <= 0;
     else if (endOfLine) hcount <= 0;
     else  	         hcount <= hcount + 11'd 1;

   assign endOfLine = hcount == HTOTAL - 1;
       
   logic endOfField;
   
   always_ff @(posedge clk50 or posedge reset)
     if (reset)          vcount <= 0;
     else if (endOfLine)
       if (endOfField)   vcount <= 0;
       else              vcount <= vcount + 10'd 1;

   assign endOfField = vcount == VTOTAL - 1;

   // Horizontal sync: from 8'h520 to 8'h5DF (8'h57F)
   // 101 0010 0000 to 101 1101 1111
   assign VGA_HS = !( (hcount[10:8] == 3'b101) &
		      !(hcount[7:5] == 3'b111));
   assign VGA_VS = !( vcount[9:1] == (VACTIVE + VFRONT_PORCH) / 2);

   assign VGA_SYNC_n = 1'b0; // For putting sync on the green signal; unused
   
   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480
   // 110 0011 1111  1599	       10 0000 1100  524
   assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
			!( vcount[9] | (vcount[8:5] == 4'b1111) );

   /* VGA_CLK is 25 MHz
    *             __    __    __
    * clk50    __|  |__|  |__|
    *        
    *             _____       __
    * hcount[0]__|     |_____|
    */
   assign VGA_CLK = hcount[0]; // 25 MHz clock: rising edge sensitive
   
endmodule
