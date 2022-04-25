/*
 * Avalon memory-mapped peripheral that generates VGA signal from 
 * column data in ray casting context.
 *
 * Adam Carpentieri AC4409
 * Columbia University

    TODO:
    3. Ceiling and floor gradients / colors
    4. Blackout screen toggle
    5. Text module
    
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
    logic [9:0]     vcount, vcount_1_ahead;

    logic [1:0]      cur_col_write_stage = 2'h0; //which of 3 write stages per column
    logic [1:0]      col_module_index_to_read = 2'b00; //which columns module to read
    logic [1:0]      col_module_index_to_write = 2'b01;  //which columns module to write to
    logic [2:0]      col_write = 3'b0;
    logic            new_columns_ready = 1'b0;

    logic [9:0]      cur_col_first_write_stage_data;
    logic [15:0]     cur_col_second_write_stage_data;
    logic [15:0]     cur_col_third_write_stage_data;

    logic [9:0]  colnum [2:0];
    logic [41:0] new_coldata [2:0];
    logic [41:0] col_data [2:0];
    logic [15:0] new_sfdata[2:0];
    logic [15:0] sf_data [2:0];

    logic [2:0] texture_type_select = 1'b0;
    logic [5:0] texture_row_select = 6'b0;
    logic [5:0] texture_col_select = 6'b0;
    logic [23:0] cur_texture_rgb_vals = {8'hff, 8'hff, 8'hff}; //output


    logic [2:0] pixel_type = 3'b0; //0: ceiling, 1: wall, 2: floor
    logic       pixel_wall_dir = 1'b0;

    logic freeze_pipeline = 1'b0; //freeze the pixel pipeline (during vga_blank_n generally)

    columns columns0(clk, reset, col_write[0], colnum[0], new_sfdata[0], new_coldata[0], sf_data[0], col_data[0]),
            columns1(clk, reset, col_write[1], colnum[1], new_sfdata[1], new_coldata[1], sf_data[1], col_data[1]),
            columns2(clk, reset, col_write[2], colnum[2], new_sfdata[2], new_coldata[2], sf_data[2], col_data[2]);

    textures textures0 (texture_type_select, 
        texture_row_select, 
        texture_col_select, 
        cur_texture_rgb_vals
    );

    vga_counters counters(.clk50(clk), .*);

    always_ff @(posedge clk) begin

        if (reset) begin

            {colnum[0], colnum[1], colnum[2]} <= 30'h0;
            cur_col_write_stage <= 1'h0;
            col_module_index_to_read <= 2'b00; 
            col_module_index_to_write <= 2'b01;
            col_write <= 3'b0;

        end else if (chipselect && write) begin
            
            //1st write stage
            if(!cur_col_write_stage) begin
            
                cur_col_first_write_stage_data <= writedata[9:0];
                col_write <= 3'b0;
                cur_col_write_stage <= cur_col_write_stage + 2'h1;

            //second write stage
            end else if(cur_col_write_stage == 2'h1) begin
                cur_col_second_write_stage_data <= writedata;
                cur_col_write_stage <= cur_col_write_stage + 2'h1;

            //third write stage
            end else if(cur_col_write_stage == 2'h2) begin
                cur_col_third_write_stage_data <= writedata;
                cur_col_write_stage <= cur_col_write_stage + 2'h1;    

            //fourth write stage
            end else begin

                if(colnum[col_module_index_to_write] == 10'h27F) begin //639
                    
                    col_module_index_to_write <= 2'b11 ^ col_module_index_to_write ^ col_module_index_to_read;
                    colnum[2'b11 ^ col_module_index_to_write ^ col_module_index_to_read] <= 10'b0;
                    new_columns_ready <= 1'b1;
                end
                else
                    colnum[col_module_index_to_write] <= colnum[col_module_index_to_write] + 10'b1; //increment col num
                
                new_coldata[col_module_index_to_write] <= {cur_col_third_write_stage_data, cur_col_second_write_stage_data, cur_col_first_write_stage_data};
                new_sfdata[col_module_index_to_write] <= writedata;
                col_write[col_module_index_to_write] <= 1'h1;
                cur_col_write_stage <= 2'h0;
                
            end
        end
        else
            col_write <= 3'b0;

    //pixel pipeline
   // always_ff @(posedge clk)

        if(hcount == 11'h4fc) //1276 (638)
            freeze_pipeline <= 1'b1;

        else if(hcount == 11'h63d && (vcount < 10'h1e0 || vcount == 10'h20d)) //1597, 480, 525
            freeze_pipeline <= 1'b0; 

    //pipeline stage 1 - retrieve column data
   // always_ff @(posedge clk)
                       
        if(!freeze_pipeline) 
            colnum[col_module_index_to_read] <= hcount < 11'h4fc //1276
                ? hcount[10:1] + 10'h2
                : hcount[10:1] - 10'h27e;

    //(pipeline stage 2 is just waiting for column data)

    //pipeline stage 3 - use column data to set texture registers
    //always_ff @(posedge clk) 
        
        //ceil
        if(vcount_1_ahead < col_data[col_module_index_to_read][41:26] && !col_data[col_module_index_to_read][41]) //top of wall
            pixel_type <= 2'h0;
        
        //floor
        else if(vcount_1_ahead > ($signed(col_data[col_module_index_to_read][41:26]) + col_data[col_module_index_to_read][25:10]))
            pixel_type <= 2'h2;
        
        else begin //wall

            pixel_type <= 2'h1;
            pixel_wall_dir <= col_data[col_module_index_to_read][9];

            texture_type_select <= col_data[col_module_index_to_read][8:6];
            texture_col_select <= col_data[col_module_index_to_read][5:0];
            texture_row_select <= ((vcount_1_ahead - $signed(col_data[col_module_index_to_read][41:26])) * sf_data[col_module_index_to_read]) >> 4'h9;
        end


    //swap out column module to read from if new avail and in between frames
    //always_ff @(posedge clk)

        if(vcount == 10'h1e0 && new_columns_ready) begin //480

            new_columns_ready <= 1'b0;
            col_module_index_to_read <= 2'b11 ^ col_module_index_to_read ^ col_module_index_to_write;
        end

    end
    always_comb begin

        //"pipeline" stage 4 (current clock)

        vcount_1_ahead = hcount > 11'h63f //1599
            ? vcount > 10'h1df
                ? 10'h0
                : vcount + 10'h1
            : vcount; 

        {VGA_R, VGA_G, VGA_B} = {8'h0, 8'h0, 8'h0};

        if (VGA_BLANK_n) begin 

            if(pixel_type == 2'h0) //ceil
                {VGA_R, VGA_G, VGA_B} = {(vcount[8:1] + 8'h00), (vcount[8:1] + 8'h00), 8'hff};

            else if(pixel_type == 2'h2) //floor TODO gradient
                {VGA_R, VGA_G, VGA_B} = {8'h40, 8'h40, 8'h40};

            else if(!pixel_wall_dir) //wall faded
                {VGA_R, VGA_G, VGA_B} = {(cur_texture_rgb_vals[23:16]>>1), (cur_texture_rgb_vals[15:8]>>1), (cur_texture_rgb_vals[7:0]>>1)};

            else //wall full brightness
                {VGA_R, VGA_G, VGA_B} = cur_texture_rgb_vals;
        end 
    end

endmodule

module columns(
    input logic clk, reset, write,
    input logic [9:0] col_num,
    input logic [15:0] new_sf_data,
    input logic [41:0] new_col_data,
    output logic [15:0] sf_data,
    output logic [41:0] col_data
);

    //declare array https://www.chipverify.com/verilog/verilog-arrays
    logic [41:0] columns [639:0];
    logic [15:0] sfs [639:0]; //scaling factors

    integer i;

    initial begin
        for (i=10'h0; i<10'h280; i=i+10'h1) begin 
            columns[i] <= 42'b0;
            sfs[i] <= 16'b0;
        end
    end

    always_ff @(posedge clk) begin
        if(write) begin

            columns[col_num] <= new_col_data;
            col_data <= new_col_data;     
        end else  
            col_data <= columns[col_num]; 
    end

    always_ff @(posedge clk) begin
        if(write) begin

            sfs[col_num] <= new_sf_data;
            sf_data <= new_sf_data;     
        end else  
            sf_data <= sfs[col_num]; 
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

    assign texture_data = textures[{texture_type, row, col}];

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