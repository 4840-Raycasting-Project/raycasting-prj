/*
 * Avalon memory-mapped peripheral that generates VGA signal from 
 * column data in ray casting context.
 *
 * Adam Carpentieri AC4409
 * Columbia University
 */

module column_decoder(input logic clk,
	    input logic 	   reset,
		input logic 	   write,
		input 		   chipselect,
        input logic [15:0]  writedata,

		output logic [7:0] VGA_R, VGA_G, VGA_B,
		output logic 	   VGA_CLK, VGA_HS, VGA_VS,
		                   VGA_BLANK_n,
		output logic 	   VGA_SYNC_n);

    logic [10:0]	hcount;
    logic [9:0]     vcount;

    logic            cur_col_write_stage = 1'h0; //1st or second stage of writing per column
    logic [1:0]      col_module_index_to_read = 2'b00; //which columns module to read
    logic [1:0]      col_module_index_to_write = 2'b01;  //which columns module to write to
    logic [2:0]      col_write;
    logic            new_columns_ready = 1'b0;

    logic [12:0]     cur_col_first_write_stage_data;

    logic [9:0]  colnum [2:0];
    logic [27:0] new_coldata [2:0];
    logic [27:0] col_data [2:0];

    logic [2:0] texture_type_select;
    logic [5:0] texture_row_select;
    logic [5:0] texture_col_select;
    logic [23:0] cur_texture_rgb_vals;

    logic [8:0]  sf_wall_height;
    logic [16:0] scaling_factor;

    logic [2:0] s2_pixel_type = 3'b0; //0: ceiling, 1: wall, 2: floor
    logic       s2_pixel_wall_dir = 1'b0;
    logic [2:0] s2_texture_type;
    logic [5:0] s2_texture_offset;
    logic [8:0] s2_top_of_wall; 

    logic [2:0]  s3_pixel_type = 3'b0; //0: ceiling, 1: wall, 2: floor
    logic        s3_pixel_wall_dir = 1'b0;

    logic [2:0]  s4_pixel_type = 3'b0; //0: ceiling, 1: wall, 2: floor
    logic        s4_pixel_wall_dir = 1'b0;  

    logic [23:0] next_pixel = 24'b0; //texture data in the on deck circle

    logic freeze_pipeline = 1'b0; //freeze the pixel pipeline (during vga_blank_n generally)

    columns columns0(clk, reset, col_write[0], colnum[0], new_coldata[0], col_data[0]),
            columns1(clk, reset, col_write[1], colnum[1], new_coldata[1], col_data[1]),
            columns2(clk, reset, col_write[2], colnum[2], new_coldata[2], col_data[2]);

    textures textures0 (texture_type_select, 
        texture_row_select, 
        texture_col_select, 
        cur_texture_rgb_vals
    );

    scaling_factors sf0 (
        sf_wall_height,
        scaling_factor
    );

    vga_counters counters(.clk50(clk), .*);

    always_ff @(posedge clk) begin

        if (reset) begin

            {colnum[0], colnum[1], colnum[2]} <= 30'h0;
            cur_col_write_stage <= 1'h0;
            col_module_index_to_read <= 2'b00; 
            col_module_index_to_write <= 2'b01;

        end else if (chipselect && write) begin

            if(!cur_col_write_stage) begin
            
                cur_col_first_write_stage_data <= writedata[12:0];
                col_write[col_module_index_to_write] <= 1'b0;
       
            end else begin
                
                new_coldata[col_module_index_to_write] <= {cur_col_first_write_stage_data, writedata[14:0]};
                col_write[col_module_index_to_write] <= 1'b1;
                colnum[col_module_index_to_write] <= colnum[col_module_index_to_write] + 10'b00_0000_0001; //increment col num
            end

            cur_col_write_stage <= cur_col_write_stage + 1'b1; //flips to 1 or 0

        end else if(colnum[col_module_index_to_write] == 10'h280) begin //640th column indicates writing has finished
            
            col_module_index_to_write <= 2'b11 ^ col_module_index_to_write ^ col_module_index_to_read;
            colnum[2'b11 ^ col_module_index_to_write ^ col_module_index_to_read] = 10'b0; //hope this works or else need new technique
            new_columns_ready <= 1'b1;
        end

    //pixel pipeline
   // always_ff @(posedge clk)

        if(hcount == 11'h4fc) //1276
            freeze_pipeline <= 1'b1;

        else if(hcount == 11'h63b && vcount < 10'h1e0) //1596, 480
            freeze_pipeline <= 1'b0; 

    //pipeline stage 1
   // always_ff @(posedge clk)
                       
        if(!freeze_pipeline)
            colnum[col_module_index_to_read] <= hcount[10:1];

    //pipeline stage 2
    //always_ff @(posedge clk) 
        
        //ceil
        if(vcount < col_data[col_module_index_to_read][27:19])
            s2_pixel_type <= 2'h0;
        
        //floor
        else if(vcount > (col_data[col_module_index_to_read][27:19] + col_data[col_module_index_to_read][14:6]))
            s2_pixel_type <= 2'h2;

        else begin //wall

            s2_pixel_type <= 2'h1;
            s2_pixel_wall_dir <= col_data[col_module_index_to_read][18];
            s2_texture_type <= col_data[col_module_index_to_read][17:15];
            s2_texture_offset <= col_data[col_module_index_to_read][5:0];
            s2_top_of_wall <= col_data[col_module_index_to_read][27:19];

            sf_wall_height <= col_data[col_module_index_to_read][14:6]; //load scaling factor
        end

    //pipeline stage 3
    //always_ff @(posedge clk)  

        if(s2_pixel_type == 2'h1) begin //wall

            s3_pixel_type <= s2_pixel_type;

            s3_pixel_wall_dir <= s2_pixel_wall_dir;
           
            texture_type_select <= s2_texture_type;
            texture_col_select <= s2_texture_offset;
            texture_row_select <= ((vcount - s2_top_of_wall) * scaling_factor) >> 4'ha;
        
        end else
            s3_pixel_type <= s2_pixel_type;

    //pipeline stage 4
   // always_ff @(posedge clk)    

        if(s3_pixel_type == 2'h1) begin //wall

            s4_pixel_type <= s3_pixel_type;
           
            next_pixel <= cur_texture_rgb_vals;
            s4_pixel_wall_dir <= s3_pixel_wall_dir;
        
        end else
            s4_pixel_type <= s3_pixel_type;

    //swap out column module to read from if new avail and in between frames
    //always_ff @(posedge clk)

        if(vcount == 10'h1e0 && new_columns_ready) begin //480

            new_columns_ready <= 1'b0;
            col_module_index_to_read <= 2'b11 ^ col_module_index_to_read ^ col_module_index_to_write;
        end

    end
    always_comb begin

        {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};

        if (!VGA_BLANK_n) //seems wrong
          {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0}; //just give it something so verilog doesn't complain (pins are actually not read)
        else begin

            if(s4_pixel_type == 2'h0) //ceil
                {VGA_R, VGA_G, VGA_B} = {8'h32, 8'h32, 8'h32};
            else if(s4_pixel_type == 2'h2) //floor
                {VGA_R, VGA_G, VGA_B} = {8'ha, 8'ha, 8'ha};
            else if(!s4_pixel_wall_dir) //wall faded
                {VGA_R, VGA_G, VGA_B} = {(next_pixel[23:16]>>1), (next_pixel[15:8]>>1), (next_pixel[7:0]>>1)};
            else //wall full brightness
                {VGA_R, VGA_G, VGA_B} = next_pixel;
        end
    end

endmodule

module columns(
    input logic clk50, reset, write,
    input logic [9:0] col_num,
    input logic [27:0] new_col_data,
    output logic [27:0] col_data
);

    //declare array https://www.chipverify.com/verilog/verilog-arrays
    logic [27:0] columns [639:0];

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

//load preprocessed scaling factors
module scaling_factors(
    input logic [8:0] wall_height,
    output logic [16:0] scaling_factor
);

    logic [16:0] scaling_factors [639:0];
    integer i;

    initial begin
        //$display("Loading scaling factors.");
        $readmemb("sf.mem", scaling_factors);
    end

    always_comb begin
        scaling_factor = scaling_factors[wall_height];
    end


endmodule

//types: 0: bluestone, 1: colorstone, 2: eagle, 3: greystone, 4: mossy, 5: purplestone, 6: redbrick, 7: wood
module textures(
    input logic [2:0] texture_type,
    input logic [5:0] row,
    input logic [5:0] col,
    output logic [23:0] texture_data
);

    logic [27:0] textures [32767:0]; //texture type, row num, col num

    //https://projectf.io/posts/initialize-memory-in-verilog/
    initial begin
        //$display("Loading texures.");
        $readmemh("textures.mem", textures);
    end

    always_comb begin
        texture_data = textures[{texture_type, row, col}];
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