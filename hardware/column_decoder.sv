/*
 * Avalon memory-mapped peripheral that generates VGA signal from 
 * column data in ray casting context.
 *
 * Adam Carpentieri AC4409
 * Columbia University
 */

module column_decoder(input logic        clk,
	        input logic 	   reset,
		input logic [15:0]  writedata,
		input logic 	   write,
		input 		   chipselect,

		output logic [7:0] VGA_R, VGA_G, VGA_B,
		output logic 	   VGA_CLK, VGA_HS, VGA_VS,
		                   VGA_BLANK_n,
		output logic 	   VGA_SYNC_n);

    logic [10:0]	hcount;
    logic [9:0]     vcount;

    logic            cur_col_write_stage = 1'h0; //1st or second stage of writing per column
    logic [1:0]      col_module_index_to_output = 2'b00; //columns module to display
    logic [1:0]      col_module_index_to_write = 2'b01;  //columns module to write to
    logic [2:0]      col_write;
    logic            new_columns_ready = 1'b0;

    logic [12:0]     cur_col_first_write_stage_data;

    logic [9:0]  colnum [1:0];
    logic [27:0] new_coldata [1:0];
    logic [27:0] col_data [1:0];

    logic [2:0] texture_type_select;
    logic [8:0] texture_row_select;
    logic [9:0] texture_col_select;
    logic [23:0] cur_texture_rgb_vals;

    columns columns0(clk, reset, col_write[0], colnum[0], new_coldata[0], col_data[0]),
            columns1(clk, reset, col_write[1], colnum[1], new_coldata[1], col_data[1]),
            columns2(clk, reset, col_write[2], colnum[2], new_coldata[2], col_data[2]);

    textures textures0 (texture_type_select, 
        texture_row_select, 
        texture_col_select, 
        cur_texture_rgb_vals
    );

    vga_counters counters(.clk50(clk), .*);

    always_ff @(posedge clk)

        if (reset) begin

            colnum <= 30'h0;
            cur_col_write_stage <= 1'h0;
            col_module_index_to_output <= 2'b00; 
            col_module_index_to_write <= 2'b01;

        end else if (chipselect && write) begin

            //if(writedata == 16'b1111_1111_1111_1111)

            if(!cur_col_write_stage) begin
            
                cur_col_first_write_stage_data <= writedata[12:0];
                col_write[col_module_index_to_write] <= 1'b0;
            
            end else begin
                
                new_coldata[col_module_index_to_write] <= {cur_col_first_write_stage_data, writedata[14:0]};
                col_write[col_module_index_to_write] <= 1'b1;
                colnum[col_module_index_to_write] <= colnum[col_module_index_to_write] + 10'b00_0000_0001; //increment col num
            end

            cur_col_write_stage <= cur_col_write_stage + 1'b1; //flips to 1 or 0

        end else if(colnum[col_module_index_to_write] == 10'h281) begin //641st column indicates writing has finished
            
            col_module_index_to_write <= (2'b11 ^ col_module_index_to_write ^ col_module_index_to_output);
            colnum[2'b11 ^ col_module_index_to_write ^ col_module_index_to_output] = 10'b0; //hope this works or else need new technique
            new_columns_ready <= 1;

        end


        //TODO pixel pipeline
    //always_ff @(posedge clk)


    always_comb begin


        {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};


        if (!VGA_BLANK_n) //seems wrong
          {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};

    end

endmodule


module columns(
    input logic clk50, reset, write,
    input logic [9:0] col_num,
    input logic [27:0] new_col_data,
    output logic [27:0] col_data
);

    //declare array https://www.chipverify.com/verilog/verilog-arrays
    logic [27:0] columns [9:0];

    //write if necc + reset zeroes it all out
    integer i;

    always_ff @(posedge clk50)
        if (reset) begin
            for (i=10'h0; i<10'h280; i=i+10'h1) 
                columns[i] <= 28'b00;

        end else if(write) begin
            columns[col_num] <= new_col_data;
        end  

    always_comb begin
        col_data = columns[col_num];
    end

endmodule

//types: 1: bluestone, 2: colorstone, 3: eagle, 4: greystone, 5: mossy, 6: purplestone, 7: redbrick, 8: wood
module textures(
    input logic [2:0] texture_type ,
    input logic [5:0] row,
    input logic [5:0] col,
    output logic [23:0] texture_data
);

    logic  [27:0] textures [5:0] [5:0] [2:0]; //row num, col num, texture type

    initial begin
        $display("Loading texures.");
        $readmemh("textures.mem", textures);
    end

    always_comb begin
        texture_data = columns[col][row][texture_type];
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

   // Horizontal sync: from 0x520 to 0x5DF (0x57F)
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