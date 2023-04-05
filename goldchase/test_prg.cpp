//version 1.1
#include<stdio.h>
#include<sys/mman.h>
#include<sys/stat.h> /* For mode constants */
#include<fcntl.h>    /* For O_ constants */
#include <iostream>
#include<unistd.h>
#include<sys/mman.h>
#include <semaphore.h>
#include <stdlib.h>
#include "goldchase.h"
#include "Map.h"
#include<fstream>
#include<string>
using namespace std;
#include "goldchase.h"
#include "Map.h"
#include "memory"
#include "player.h"


struct goldMine {
  unsigned short rows;
  unsigned short cols;
  player players[5];
  unsigned char map[0];

  //something here for the map itself. an array of unsigned characters
};
  
int main(int argc,char* argv[])
{
	sem_t* mysemaphore=nullptr;
	int syscall_result;
  unsigned char active_plr_bit;
  goldMine* thegoldmine;
	unsigned short num_gold, rows=0,cols=0;
	int shared_mem_fd;
	int current_byte=0;
  string line;
  int returned_value;
  shared_ptr<Map> pointer_to_rendering_map;
  int playerLocation = 0;
  bool won = 0;

  srand(time(NULL));

	if(argc==2){
		mysemaphore=sem_open("/goldchase_semaphore", O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR, 1);
		if(mysemaphore==SEM_FAILED)
  		{
    			if(errno==EEXIST)
           			std::cerr << "The game is already running. Do not provide a map file.\n";
         		else //any other kind of semaphore open failure
           			perror("Create & open semaphore");
         		exit(1);
  		}
  		syscall_result=sem_wait(mysemaphore); //"grab" or "take" a semaphore
  		if(syscall_result==-1) { 
              perror("Failed sem_wait"); 
              return 2; 
        }
  		
  		ifstream mapfile(argv[1]); //open the file
  		if (mapfile.is_open() && mapfile.good()) {
  			getline(mapfile,line);
  			num_gold=stoi(line);
  			rows=0, cols=0;
        		while (getline(mapfile, line)){
            		++rows;
    				cols=line.length()>cols ? line.length() : cols;
        		}
        
    		} else {
        		std::cerr << "Unable to open file please check file.\n";
        		syscall_result=sem_post(mysemaphore);
        		sem_close(mysemaphore);
        		exit(1);
    		}
    		shared_mem_fd=shm_open("/goldmemory", O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    		if(shared_mem_fd==-1){
          cout <<"issue in shared memory" << endl;
    			syscall_result=sem_post(mysemaphore);
        		sem_close(mysemaphore);
        		exit(1);
    		}
    		ftruncate(shared_mem_fd,sizeof(goldMine)+rows*cols);
    		thegoldmine=(goldMine*)mmap(nullptr, sizeof(goldMine)+rows*cols, PROT_READ|PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
    		thegoldmine->rows = rows;
            thegoldmine->cols = cols;
            thegoldmine->players[0].setplayerName(G_PLR0);
            thegoldmine->players[0].setFlag(1);
            active_plr_bit = thegoldmine->players[0].getplayerName();
            thegoldmine->players[1].setplayerName(G_PLR1);
            thegoldmine->players[1].setFlag(0);
            thegoldmine->players[2].setplayerName(G_PLR2);
            thegoldmine->players[2].setFlag(0);
            thegoldmine->players[3].setplayerName(G_PLR3);
            thegoldmine->players[3].setFlag(0);
            thegoldmine->players[4].setplayerName(G_PLR4);
            thegoldmine->players[4].setFlag(0);
            // ifstream mapfile(argv[1]);
            mapfile.close();
            mapfile.open(argv[1]);
  		      getline(mapfile, line); //discard num gold
            for(int cur_row=0; cur_row<rows; ++cur_row)
            {
                getline(mapfile, line);
                for(int cur_col=0; cur_col<line.length(); ++cur_col)
                    if(line[cur_col]=='*')
                        thegoldmine->map[cur_row*cols+cur_col]=G_WALL;
            }
            mapfile.close();

            for(int i = 0; i< num_gold-1;i++){
              int falseGold = 0;
              do{
                falseGold=rand() % (rows*cols) + 1;
              }
              while(thegoldmine->map[falseGold]==G_WALL || thegoldmine->map[falseGold]==G_FOOL );
              thegoldmine->map[falseGold] = G_FOOL;
            }
            int trueGold = 0;
            do{
                trueGold=rand() % (rows*cols) + 1;
              }
              while(thegoldmine->map[trueGold]==G_WALL || thegoldmine->map[trueGold]==G_FOOL );
              thegoldmine->map[trueGold] = G_GOLD;
            
            playerLocation = 0;
            do{
                playerLocation=rand() % (rows*cols) + 1;
            }
            while(thegoldmine->map[playerLocation]==G_WALL || thegoldmine->map[playerLocation]==G_FOOL );
            thegoldmine->map[playerLocation] = active_plr_bit;
            //pointer_to_rendering_map=make_unique<Map>(thegoldmine->map, rows, cols);
            // shm_unlink("/go");
            syscall_result=sem_post(mysemaphore); //release the semaphore
            if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
            /*Map goldMine1(thegoldmine->map,rows,cols);
            goldMine1.postNotice("This is a notice");
            while(goldMine1.getKey()!='Q');
           */
	}
	
	if(argc==1){
    mysemaphore=sem_open("/goldchase_semaphore", O_RDWR);
    if(mysemaphore==SEM_FAILED)
  		{
    			if(errno==ENOENT)
           			std::cerr << "Nobody is playing! Provide a map file to start the game.\n";
         		else //any other kind of semaphore open failure
           			perror("Open semaphore");
         		exit(1);
  		}

    syscall_result=sem_wait(mysemaphore);
    if(syscall_result==-1) { 
      perror("Failed sem_wait"); 
      return 2; 
    }

    shared_mem_fd=shm_open("/goldmemory", O_RDWR, S_IRUSR|S_IWUSR); 
    if(shared_mem_fd==-1){
          cout <<"issue in shared memory" << endl;
    			syscall_result=sem_post(mysemaphore);
        	sem_close(mysemaphore);
        	exit(1);
    }
   
	
    returned_value = read(shared_mem_fd, &rows, sizeof(rows));
    if(returned_value==-1) { 
      perror("Reading rows"); 
      exit(1); 
    }
    returned_value = read(shared_mem_fd, &cols, sizeof(cols));
    if(returned_value==-1) { 
      perror("Reading cols"); 
      exit(1); 
    }
    thegoldmine=(goldMine*)mmap(nullptr, sizeof(goldMine)+rows*cols, PROT_READ|PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
    bool flag = false;
    for (int i = 0; i< 5; i++){

    	if (thegoldmine->players[i].getFlag() == 0){
        flag = true;
    		thegoldmine->players[i].setFlag(1);
    		active_plr_bit = thegoldmine->players[i].getplayerName();
    		break;
    	}
    }
    if(flag == false){
      syscall_result=sem_post(mysemaphore); //release the semaphore
      if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
      cout<<"Can't have more than 5 players in the Game!";
      exit(1);
    }
   playerLocation = 0;
            do{
                playerLocation=rand() % (rows*cols) + 1;

            }
            while(thegoldmine->map[playerLocation]==G_WALL || thegoldmine->map[playerLocation]==G_FOOL );
    thegoldmine->map[playerLocation] = active_plr_bit;
    syscall_result=sem_post(mysemaphore); //release the semaphore 
   // pointer_to_rendering_map=make_shared<Map>(thegoldmine->map, rows, cols);
    // Map goldMine2(thegoldmine1->map,row,col);
    /*goldMine2.postNotice("This is a notice");
    while(goldMine2.getKey()!='Q');

	*/

}


// pointer_to_rendering_map=make_shared<Map>(thegoldmine->map,rows,cols);
try {
  pointer_to_rendering_map=std::make_unique<Map>(thegoldmine->map, thegoldmine->rows, thegoldmine->cols);
} catch(const std::exception& e) {
  cerr << e.what() << '\n';
  // if no players remain, close and delete
  // shared memory and semaphore
  if( (sizeof(thegoldmine->players) & ~active_plr_bit) == 0 )
  {
    close(shared_mem_fd);
    shm_unlink("/goldchase_semaphore");
    sem_close(mysemaphore);
    sem_unlink("/goldmemory");
  }
  exit(1);
} //end catch
pointer_to_rendering_map->postNotice("This is a notice");
char prev_bit = 0;

while(1){
  int key_pressed = pointer_to_rendering_map->getKey();

        if(key_pressed == 'h')
        {
            if(playerLocation-1 >= 0  && thegoldmine->map[playerLocation-1] != G_WALL )
            {
                syscall_result=sem_wait(mysemaphore);
                if(syscall_result==-1) { 
                  perror("Failed sem_wait"); 
                  return 2; 
                }

                thegoldmine->map[playerLocation] = prev_bit;
                if(thegoldmine->map[playerLocation-1] == G_GOLD)
                {
                  // syscall_result=sem_wait(mysemaphore);
                  //   if(syscall_result==-1) { 
                  //   perror("Failed sem_wait"); 
                  //   return 2; 
                  // }
                    thegoldmine->map[playerLocation-1] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("You won the game!");
                    won = 1;
                    playerLocation -= 1;
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation-1] == G_FOOL)
                {
                  // syscall_result=sem_wait(mysemaphore);
                  //   if(syscall_result==-1) { 
                  //   perror("Failed sem_wait"); 
                  //   return 2; 
                  // }
                    thegoldmine->map[playerLocation-1] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("This was a fool's gold, try again!");
                    playerLocation -= 1;
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);

                    continue;
                }
                else if(thegoldmine->map[playerLocation-1] != 0)
                    prev_bit = thegoldmine->map[playerLocation-1];
                else
                    prev_bit = 0;
                thegoldmine->map[playerLocation-1] = active_plr_bit;
                playerLocation -= 1;
                syscall_result=sem_post(mysemaphore);
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }

            }
            else if(won && playerLocation%cols == 0)
                break;
        }
        else if(key_pressed == 'j')
        {
            if(playerLocation+cols < rows*cols && thegoldmine->map[playerLocation+cols] != G_WALL)
            {
                syscall_result=sem_wait(mysemaphore);
                    if(syscall_result==-1) { 
                    perror("Failed sem_wait"); 
                    return 2; 
                  }
                thegoldmine->map[playerLocation] = prev_bit;
                if(thegoldmine->map[playerLocation+cols] == G_GOLD)
                {
                    // syscall_result=sem_wait(mysemaphore);
                    // if(syscall_result==-1) { 
                    // perror("Failed sem_wait"); 
                    // return 2; 
                    // }
                    thegoldmine->map[playerLocation+cols] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("You won the game!");
                    won = 1;
                    playerLocation += cols; 
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation+cols] == G_FOOL)
                {
                    // syscall_result=sem_wait(mysemaphore);
                    // if(syscall_result==-1) { 
                    // perror("Failed sem_wait"); 
                    // return 2; 
                    // }
                    thegoldmine->map[playerLocation+cols] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("This was a fool's gold, try again!");
                    playerLocation += cols; 
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation+cols] != 0)
                    prev_bit = thegoldmine->map[playerLocation+cols];
                else
                    prev_bit = 0;
                thegoldmine->map[playerLocation+cols] = active_plr_bit;
                playerLocation += cols; 
                syscall_result=sem_post(mysemaphore);
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
            }
            else if(won && playerLocation+cols > rows*cols)
                break;
        }
        else if(key_pressed == 'k')
        {
            if(playerLocation-cols >= 0 && thegoldmine->map[playerLocation-cols] != G_WALL)
            {
                  syscall_result=sem_wait(mysemaphore);
                    if(syscall_result==-1) { 
                    perror("Failed sem_wait"); 
                    return 2; 
                  }
                thegoldmine->map[playerLocation] = prev_bit;
                if(thegoldmine->map[playerLocation-cols] == G_GOLD)
                {
                    thegoldmine->map[playerLocation-cols] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("You won the game!");
                    won = 1;
                    playerLocation -= cols;
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore); 
                    continue;
                }
                else if(thegoldmine->map[playerLocation-cols] == G_FOOL)
                {
                    thegoldmine->map[playerLocation-cols] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("This was a fool's gold, try again!");
                    playerLocation -= cols; 
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation-cols] != 0)
                    prev_bit = thegoldmine->map[playerLocation-cols];
                else
                    prev_bit = 0;
                thegoldmine->map[playerLocation-cols] = active_plr_bit;
                playerLocation -= cols; 
                syscall_result=sem_post(mysemaphore);
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
            }
            else if(won && playerLocation-cols < 0)
                break;
        }
        else if(key_pressed == 'l')
        {
            if(playerLocation+1 < rows*cols  && thegoldmine->map[playerLocation+1] != G_WALL)
            {
                syscall_result=sem_wait(mysemaphore);
                if(syscall_result==-1) { 
                perror("Failed sem_wait"); 
                return 2; 
                }
                thegoldmine->map[playerLocation] = prev_bit;
                if(thegoldmine->map[playerLocation+1] == G_GOLD)
                {
                    thegoldmine->map[playerLocation+1] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("You won the game!");
                    won = 1;
                    playerLocation += 1; 
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation+1] == G_FOOL)
                {
                    thegoldmine->map[playerLocation+1] = active_plr_bit;
                    pointer_to_rendering_map->drawMap();
                    pointer_to_rendering_map->postNotice("This was a fool's gold, try again!");
                    playerLocation += 1; 
                    syscall_result=sem_post(mysemaphore); //release the semaphore
                    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
                    continue;
                }
                else if(thegoldmine->map[playerLocation+1] != 0)
                    prev_bit = thegoldmine->map[playerLocation+1];
                else
                    prev_bit = 0;
                thegoldmine->map[playerLocation+1] = active_plr_bit;
                playerLocation += 1; 
              syscall_result=sem_post(mysemaphore);
              if(syscall_result==-1) { perror("Failed sem_post"); return 2; }

                
            }
            else if(won && (playerLocation+1)%cols == 0 )
                break;
        }
        pointer_to_rendering_map->drawMap();

        if(key_pressed == 'q'){
          thegoldmine->map[playerLocation] = 0;
           
            for (int i = 0; i < 5; i++){
              if(thegoldmine->players[i].getplayerName() == active_plr_bit){
                thegoldmine->players[i].setFlag(0);
              }
            }
             break;
             exit(0);

        }     
}
  thegoldmine->map[playerLocation] = 0;
  bool exitflag = false;
  for (int i = 0; i<5; i++){
  
  if(thegoldmine->players[i].getFlag()==1){
  exitflag = true;
  }
  }
  if(!exitflag){
        close(shared_mem_fd);
        shm_unlink("/goldmemory");
        sem_close(mysemaphore);
        sem_unlink("/goldchase_semaphore");
}

}

