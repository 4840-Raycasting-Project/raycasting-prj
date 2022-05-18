## Welcome to the 4840 Raycasting Project repo for spring semester 2022  

Members/Collaborators:  
- Adam Carpentieri (MS CS @ Columbia)
- Souryadeep Sen (MS CE @ Columbia)  


### Block Diagram

![alt text](https://github.com/4840-Raycasting-Project/raycasting-prj/blob/master/block_diagram.png)


### Tools required (On a Linux machine)

- Software: 
	  - gcc: sudo apt install gcc  
	  - make: sudo apt-get install build-essential  
	  - linux headers: wget http://www.cs.columbia.edu/~sedwards/classes/2019/4840spring/linuxheaders4.19.0.tar.gz  
		  - tar Pzxf linuxheaders4.19.0.tar.gz  
		  - ls /usr/src/linuxheaders4.19.0  
	  - insmod/rmmod: sudo apt install -y kmod
	  - your favorite text editor (vim/gvim/nano/sublime...)
- Hardware:
	  - Quartus Prime 21.1 fpga design software: https://fpgasoftware.intel.com/21.1/?edition=lite
	  - Follow lab3 slides to build the vga module 
	  - Verilator/GTKwave: for simulation, if you desire   

### Hardware required for game  

- Intel DE1 SOC development kit
- Any USB keyboard
- USB controller (Dragonrise controller: vendor ID: 0x0079)
- VGA cable
- Micro USB

### Compiling the code  

- Software: https://github.com/4840-Raycasting-Project/raycasting-prj/blob/master/software/README
- Hardware: Please follow lab3 guide


### Inside the game  

![alt text](https://github.com/4840-Raycasting-Project/raycasting-prj/blob/master/game_start.jpg)  

![alt text](https://github.com/4840-Raycasting-Project/raycasting-prj/blob/master/maze_end.jpg)

