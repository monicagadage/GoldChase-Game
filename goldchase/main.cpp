//version 1.1
#include<stdio.h>
#include<sys/mman.h>
#include<sys/stat.h> /* For mode constants */
#include<fcntl.h>    /* For O_ constants */
#include <iostream>
#include<unistd.h>
#include<sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#include <stdlib.h>
#include "goldchase.h"
#include "Map.h"
#include<fstream>
#include<string>
#include <mqueue.h>
using namespace std;
#include "goldchase.h"
#include "Map.h"
#include "memory"
#include "player.h"
#include <cstring>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#define MODES S_IROTH| S_IWOTH| S_IRUSR| S_IWUSR |S_IRGRP |S_IWGRP

#define sysout(x) write(99, x, strlen(x))

struct goldMine {
  unsigned short rows;
  unsigned short cols;
  unsigned char player_name[5];
  pid_t players_pids[5];
  unsigned char map[0];
  string mqname[5];

  //something here for the map itself. an array of unsigned characters
};

        shared_ptr<Map> pointer_to_rendering_map;
        bool won = 0;
        static mqd_t readqueue_fd;
        sem_t* mysemaphore=nullptr;
        int syscall_result;
        unsigned char active_plr_bit;
        goldMine* thegoldmine;
        unsigned short num_gold=0, rows=0,cols=0;
        int shared_mem_fd;
        int current_byte=0;
        string line;
        int returned_value;
        std::string mq_name;
       
        int playerLocation = 0;
  
void notifyAll()
{
	pid_t pid = getpid();
	int exit_condition = 0;
	for(int i = 0; i < 5; i++)
	{
		if(thegoldmine -> players_pids[i] != 0 && thegoldmine -> players_pids[i] != pid)
		{
			if(kill(thegoldmine->players_pids[i],SIGUSR1) == -1)  					
				perror("Kill Error 3");
			 
		}
		 
	}
}

string getPlayer( goldMine* thegoldmine )
{
	pid_t pid = getpid();
	int i = 0;
	for(i = 0; i < 5; i++)
	{
		if(pid == thegoldmine -> players_pids[i])
		{
			break;
		}	
	}
	string to_return = to_string((i + 1));
	return to_return;
}

void writeMessage(string msg,int playerNumber)
{
	string write_mqname;
	if(playerNumber == 0)
		write_mqname = "/ply1";
	else if(playerNumber == 1)
		write_mqname = "/ply2";
	else if(playerNumber == 2)
			write_mqname = "/ply3";
	else if(playerNumber == 3)
			write_mqname = "/ply4";
	else if(playerNumber == 4)
			write_mqname = "/ply5";

	mqd_t writequeue_fd;
	if((writequeue_fd =  mq_open(write_mqname.c_str(), O_WRONLY|O_NONBLOCK)) == -1)
	{
		perror("mq open error at write mq");
		exit(1);
	}
	char message_text[121];
	memset(message_text , 0 ,121);
	strncpy(message_text, msg.c_str(), 121);
	if(mq_send(writequeue_fd, message_text, strlen(message_text), 0) == -1)
	{
		perror("mqsend error");
		exit(1);
	}
	mq_close(writequeue_fd);
}
void read_message(int)
{
    if(pointer_to_rendering_map == NULL)
		return;
    
	struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fd, &mq_notification_event);
	int err;
	char msg[121];
	memset(msg, 0, 121);//set all characters to '\0'
	while((err=mq_receive(readqueue_fd, msg, 120, NULL))!=-1)
	{
		pointer_to_rendering_map -> postNotice(msg);
		memset(msg, 0, 121);//set all characters to '\0'
	}
	if(errno!=EAGAIN)
	{
		perror("mq_receive");
		exit(1);
	}
}
void exit_cleanup_handler(int)
{
	won = true;
	return;
}

void broadcast(string bMessage,bool flag)
{
	int mask = 0;
	for(int i = 0; i < 5;i++)
	{
		if(thegoldmine -> players_pids[i] != 0)
			mask++;
	}
		
	if(mask != 1 && mask != 0)
	{
		string finalMsg = "";
		string number = getPlayer(thegoldmine);
	
		if(flag == false)
		{
			bMessage = pointer_to_rendering_map -> getMessage(); 
			finalMsg = "Player " + number + " says: " + bMessage;
		}
		else
			finalMsg = "Player " + number + " has won!!!";
		pid_t pid = getpid();
		if(bMessage != "" || flag)
		{
			for(int i = 0; i < 5; i++)
			{
				if(thegoldmine -> players_pids[i] != 0 && thegoldmine -> players_pids[i] != pid)
				{
					writeMessage(finalMsg,i);
				}
			}
		}
		else
		{
			pointer_to_rendering_map -> postNotice("Please enter message");
		}	
	}
	else if(flag == false)
	{
		pointer_to_rendering_map -> postNotice("No player!"); 
	}
}
void updateMapHandler(int)
{
	if(pointer_to_rendering_map)
		pointer_to_rendering_map -> drawMap();
}

int main(int argc,char* argv[])
{

        struct sigaction exit_handler;
		exit_handler.sa_handler = exit_cleanup_handler;
		sigemptyset(&exit_handler.sa_mask);
		exit_handler.sa_flags = 0;
		
		sigaction(SIGINT,&exit_handler,NULL);
		sigaction(SIGTERM,&exit_handler,NULL);
		sigaction(SIGHUP,&exit_handler,NULL);


		
		struct sigaction updateMap;
		updateMap.sa_handler =  updateMapHandler;
		sigemptyset(&updateMap.sa_mask);
		updateMap.sa_flags = 0;
		updateMap.sa_restorer = NULL;

		sigaction(SIGUSR1, &updateMap, NULL);

        struct sigaction action_to_take;
        action_to_take.sa_handler = read_message;
        sigemptyset(&action_to_take.sa_mask);
        action_to_take.sa_flags = 0;
        action_to_take.sa_restorer = NULL;
        
        sigaction(SIGUSR2, &action_to_take, NULL);

        struct mq_attr mq_attributes;
        mq_attributes.mq_flags=0;
        mq_attributes.mq_maxmsg=10;
        mq_attributes.mq_msgsize=120;

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
        
    	} else{
        		std::cerr << "Unable to open file please check file.\n";
        		syscall_result=sem_post(mysemaphore);
        		sem_close(mysemaphore);
            sem_unlink("/goldchase_semaphore");
        		exit(1);
    	}
    	
        shared_mem_fd=shm_open("/goldmemory", O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);
    	if(shared_mem_fd==-1){

            cout <<"issue in shared memory" << endl;
    		syscall_result=sem_post(mysemaphore);
        	sem_close(mysemaphore);
            sem_unlink("/goldchase_semaphore");
        		exit(1);
    	}
    	
        syscall_result= ftruncate(shared_mem_fd,sizeof(goldMine)+rows*cols);

        if(syscall_result==-1) { 
            perror("Failed truncate"); 
            syscall_result=sem_post(mysemaphore);
            close(shared_mem_fd);
            sem_unlink("/goldchase_semaphore");
            sem_close(mysemaphore);
            shm_unlink("/goldmemory");
            exit(1);

        return 2; 
        }  

    		thegoldmine=(goldMine*)mmap(nullptr, sizeof(goldMine)+rows*cols, PROT_READ|PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
    		thegoldmine->rows = rows;
            thegoldmine->cols = cols;
            for(int i = 0;i < 5;i++)
	        {
		        thegoldmine->players_pids[i] = 0;
               
	        }

            thegoldmine->players_pids[0] = getpid();
            active_plr_bit = G_PLR0;
		    mq_name = "/ply1";
            thegoldmine->player_name[0] = G_PLR0;
            thegoldmine->mqname[0] =  "/ply1";
            thegoldmine->player_name[1] = G_PLR1;
            thegoldmine->mqname[1] =  "/ply2";
            thegoldmine->player_name[2] = G_PLR2;
            thegoldmine->mqname[2] =  "/ply3";
            thegoldmine->player_name[3] = G_PLR3;
            thegoldmine->mqname[3] =  "/ply4";
            thegoldmine->player_name[5] = G_PLR4;
            thegoldmine->mqname[4] =  "/ply5";
            
            // thegoldmine->players[0].setplayerName(G_PLR0);
            // thegoldmine->players[0].setFlag(1);
            // active_plr_bit = thegoldmine->players[0].getplayerName();
            // thegoldmine->players[1].setplayerName(G_PLR1);
            // thegoldmine->players[1].setFlag(0);
            // thegoldmine->players[2].setplayerName(G_PLR2);
            // thegoldmine->players[2].setFlag(0);
            // thegoldmine->players[3].setplayerName(G_PLR3);
            // thegoldmine->players[3].setFlag(0);
            // thegoldmine->players[4].setplayerName(G_PLR4);
            // thegoldmine->players[4].setFlag(0);
            mapfile.close();
            mapfile.open(argv[1]);

  		    getline(mapfile, line); //discard num gold
            
            for(int cur_row=0; cur_row<rows; ++cur_row)
            {
                getline(mapfile, line);
                write(99, &line, 100);
                sysout("testing alpha");
                for(int cur_col=0; cur_col<line.length(); ++cur_col)
                    if(line[cur_col]=='*')
                        thegoldmine->map[cur_row*cols+cur_col]=G_WALL;
                    else if(line[cur_col]==' ')
                        thegoldmine->map[cur_row*cols+cur_col]=0;
            }

            mapfile.close();
            if(num_gold == 0){
                int falseGold = 0;
                int trueGold = 0;

            }
            else{
            for(int i = 0; i< num_gold-1;i++){
              int falseGold = 0;
              do{
                falseGold=rand() % ((rows*cols)-1) + 1;
              }
              while(thegoldmine->map[falseGold]==G_WALL || thegoldmine->map[falseGold]==G_FOOL );
              thegoldmine->map[falseGold] = G_FOOL;
            }
            
            int trueGold = 0;
            do{
                trueGold=rand() % ((rows*cols)-1) + 1;
              }
              while(thegoldmine->map[trueGold]==G_WALL || thegoldmine->map[trueGold]==G_FOOL );
              thegoldmine->map[trueGold] = G_GOLD;
            }
            playerLocation = 0;

            do{
                playerLocation=rand() % ((rows*cols)-1) + 1;
            }
            while(thegoldmine->map[playerLocation]==G_WALL || thegoldmine->map[playerLocation]==G_FOOL ||  thegoldmine->map[playerLocation]==G_GOLD);
            

            thegoldmine->map[playerLocation] = active_plr_bit;
            
           
            syscall_result=sem_post(mysemaphore); //release the semaphore
            if(syscall_result==-1) { perror("Failed sem_post"); return 2; }        		// sem_close(mysemaphore);
           
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
    sem_unlink("/goldchase_semaphore");

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

    	if (thegoldmine -> players_pids[i] == 0){
            flag = true;
    		thegoldmine -> players_pids[i] = getpid();
    		active_plr_bit = thegoldmine->player_name[i];
            if(i == 0){
                mq_name = "/ply1";
            }
            else if(i == 1){
                mq_name = "/ply2";
            }
            else if(i == 2){
                mq_name = "/ply3";
                
            }
            else if(i == 3){
                mq_name = "/ply4";               
            }
            else if(i == 4){
                mq_name = "/ply5" ;            
            }
    		break;
    	}
    } 
    
    if(flag == false){
      syscall_result=sem_post(mysemaphore); //release the semaphore
      if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
      cout<<"Can't have more than 5 players in the Game!";
      exit(1);
    }

    // if((readqueue_fd=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
	// 				S_IRUSR|S_IWUSR, &mq_attributes))==-1)
	// {
	// 	perror("mq_open");
	// 	mq_close(readqueue_fd);
	// 	mq_unlink(mq_name.c_str());
	// 	exit(1);
	// }
    // struct sigevent mq_notification_event;
	// mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	// mq_notification_event.sigev_signo=SIGUSR2;
	// mq_notify(readqueue_fd, &mq_notification_event);
    // playerLocation = 0;
    do{
        playerLocation=rand() % ((rows*cols)-1) + 1;

    }
    while(thegoldmine->map[playerLocation]==G_WALL 
    || thegoldmine->map[playerLocation]==G_FOOL 
    || thegoldmine->map[playerLocation]==G_PLR0 
    || thegoldmine->map[playerLocation]==G_PLR1
    || thegoldmine->map[playerLocation]==G_PLR2
    || thegoldmine->map[playerLocation]==G_PLR3
    || thegoldmine->map[playerLocation]==G_PLR4 
     );

    thegoldmine->map[playerLocation] = active_plr_bit;
    syscall_result=sem_post(mysemaphore); //release the semaphore 
    if(syscall_result==-1) { perror("Failed sem_post"); return 2; }

   

}


try {
  pointer_to_rendering_map=std::make_unique<Map>(thegoldmine->map, thegoldmine->rows, thegoldmine->cols);

} catch(const std::exception& e) {
  cerr << e.what() << '\n';
  // if no players remain, close and delete
  // shared memory and semaphore
    // mq_unlink(mq_name.c_str());
    thegoldmine->map[playerLocation] = 0;
    int to_remove =	stoi(getPlayer(thegoldmine));
    thegoldmine -> players_pids[(to_remove - 1)] = 0;

  syscall_result=sem_wait(mysemaphore);
  if(syscall_result==-1) { perror("Failed sem_wait"); return 2; }
  syscall_result=sem_post(mysemaphore); //release the semaphore
   if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
    
    bool exitf = false;
    for (int i = 0; i<5; i++){
  
        if(thegoldmine->players_pids[i]!=0){
        exitf = true;
        }
    }
  if(!exitf){
    close(shared_mem_fd);
    shm_unlink("/goldmemory");
    sem_close(mysemaphore);
    mq_close(readqueue_fd);
	mq_unlink(mq_name.c_str());
    sem_unlink("/goldchase_semaphore");
    
  }
  exit(1);
} //end catch
pointer_to_rendering_map->postNotice("This is a notice");
char prev_bit = 0;
 if((readqueue_fd=mq_open(mq_name.c_str(), O_RDONLY|O_CREAT|O_EXCL|O_NONBLOCK,
					S_IRUSR|S_IWUSR, &mq_attributes))==-1)
	{
		perror("mq_open");
		mq_close(readqueue_fd);
		mq_unlink(mq_name.c_str());
		exit(1);
	}
    struct sigevent mq_notification_event;
	mq_notification_event.sigev_notify=SIGEV_SIGNAL;
	mq_notification_event.sigev_signo=SIGUSR2;
	mq_notify(readqueue_fd, &mq_notification_event);

	notifyAll();	


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
                
                    thegoldmine->map[playerLocation-1] = active_plr_bit;
                    string bMessage = "";
				    broadcast(bMessage,true); 
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
            else if(won && playerLocation%cols == 0){
            	string bMessage = "";
				    broadcast(bMessage,true); 
            	 pointer_to_rendering_map->postNotice("You Won!");

                syscall_result=sem_wait(mysemaphore); //"grab" or "take" a semaphore
                if(syscall_result==-1) { perror("Failed sem_wait"); return 2; }
                thegoldmine->map[playerLocation] = 0;
                int to_remove =	stoi(getPlayer(thegoldmine));
                thegoldmine -> players_pids[(to_remove - 1)] = 0;

                // for (int i = 0; i < 5; i++){
              	// 	if(thegoldmine->players[i].getplayerName() == active_plr_bit){
                // 		thegoldmine->players[i].setFlag(0);
              	// 	}
            	// }
                pointer_to_rendering_map->drawMap();
                mq_unlink(mq_name.c_str());

                syscall_result=sem_post(mysemaphore); //release the semaphore
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
                break;
            
            }
                
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
                   
                    thegoldmine->map[playerLocation+cols] = active_plr_bit;
                     string bMessage = "";
				    broadcast(bMessage,true); 
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
            else if(won && playerLocation+cols > rows*cols){
             	 
                    string bMessage = "";
				    broadcast(bMessage,true); 
             	 pointer_to_rendering_map->postNotice("You Won!");
                syscall_result=sem_wait(mysemaphore); //"grab" or "take" a semaphore
                if(syscall_result==-1) { perror("Failed sem_wait"); return 2; }
                thegoldmine->map[playerLocation] = 0;
                int to_remove =	stoi(getPlayer(thegoldmine));
                thegoldmine -> players_pids[(to_remove - 1)] = 0;
                // for (int i = 0; i < 5; i++){
              	// 	if(thegoldmine->players[i].getplayerName() == active_plr_bit){
                // 		thegoldmine->players[i].setFlag(0);
              	// 	}
            	// }
                pointer_to_rendering_map->drawMap();
                mq_unlink(mq_name.c_str());

                syscall_result=sem_post(mysemaphore); //release the semaphore
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
                break;
            }

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
                    string bMessage = "";
				    broadcast(bMessage,true); 
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
            else if(won && playerLocation-cols < 0){
                    string bMessage = "";
				    broadcast(bMessage,true); 
            	 pointer_to_rendering_map->postNotice("You Won!");
                syscall_result=sem_wait(mysemaphore); //"grab" or "take" a semaphore
                if(syscall_result==-1) { perror("Failed sem_wait"); return 2; }
                thegoldmine->map[playerLocation] = 0;
                int to_remove =	stoi(getPlayer(thegoldmine));
                thegoldmine -> players_pids[(to_remove - 1)] = 0;

                // for (int i = 0; i < 5; i++){
              	// 	if(thegoldmine->players[i].getplayerName() == active_plr_bit){
                // 		thegoldmine->players[i].setFlag(0);
              	// 	}
            	// }
                pointer_to_rendering_map->drawMap();
                mq_unlink(mq_name.c_str());

                syscall_result=sem_post(mysemaphore); //release the semaphore
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
                break;
            }
        }
        else if(key_pressed == 'l')
        {
        	sysout("testing quit");
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
                     string bMessage = "";
				    broadcast(bMessage,true); 
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
            else if(won && (playerLocation+1)%cols == 0 ){
                 string bMessage = "";
				    broadcast(bMessage,true); 
            	 pointer_to_rendering_map->postNotice("You Won!");
                syscall_result=sem_wait(mysemaphore); //"grab" or "take" a semaphore
                if(syscall_result==-1) { perror("Failed sem_wait"); return 2; }
                thegoldmine->map[playerLocation] = 0;
                int to_remove =	stoi(getPlayer(thegoldmine));
                thegoldmine -> players_pids[(to_remove - 1)] = 0;
                // for (int i = 0; i < 5; i++){
              	// 	if(thegoldmine->players[i].getplayerName() == active_plr_bit){
                // 		thegoldmine->players[i].setFlag(0);
              	// 	}
            	// }

                pointer_to_rendering_map->drawMap();
                mq_unlink(mq_name.c_str());

                syscall_result=sem_post(mysemaphore); //release the semaphore
                if(syscall_result==-1) { perror("Failed sem_post"); return 2; }
                break;
            }

        }
        else if(key_pressed == 'm'){

            string Pmessage = "";
			int maskPlayerId = 0;
			pid_t pid = getpid();
			for(int i = 0;i < 5; i++)
			{
				if( thegoldmine -> players_pids[i] != 0 && thegoldmine -> players_pids[i] != pid)
				{
					if(i == 0)
						maskPlayerId |= G_PLR0;
					else if(i == 1)
						maskPlayerId |= G_PLR1;
					else if(i == 2)
						maskPlayerId |= G_PLR2;
					else if(i == 3)
						maskPlayerId |= G_PLR3;
					else if(i == 4)
						maskPlayerId |= G_PLR4;
					}
			}
			unsigned int temp = pointer_to_rendering_map -> getPlayer(maskPlayerId);
			int i;
			if(temp & G_PLR0)
				i = 0;
			else if(temp & G_PLR1)
				i = 1;
			else if(temp & G_PLR2)
				i = 2;
			else if(temp & G_PLR3)
				i = 3;
			else if(temp & G_PLR4)
				i = 4;

			if(temp != 0) 
			{
				string p = getPlayer(thegoldmine);
				Pmessage = pointer_to_rendering_map -> getMessage();
				if(Pmessage != "")
				{
					string finalMessage = "Player " + p + " says: " + Pmessage;
					writeMessage(finalMessage, i);
				}
				else
					pointer_to_rendering_map -> postNotice("Please enter some text");
			}

        }
        else if(key_pressed == 'b')
		{
			string bMessage = "";
			broadcast(bMessage,false);
		}
        pointer_to_rendering_map->drawMap();
        if(key_pressed == 'l' || key_pressed == 'h' || key_pressed == 'j' || key_pressed == 'k')
		{
			notifyAll();	
		}


        if(key_pressed == 'q'){
            sysout("testing quit");
            thegoldmine->map[playerLocation] = 0;
            int to_remove =	stoi(getPlayer(thegoldmine));
            thegoldmine -> players_pids[(to_remove - 1)] = 0;	
            mq_unlink(mq_name.c_str());
            break;
            exit(0);
        }  

}

 sysout("testing quit");
  thegoldmine->map[playerLocation] = 0;
  bool exitflag = false;
  for (int i = 0; i<5; i++){
  
  if(thegoldmine->players_pids[i] !=0){
  	exitflag = true;
  }
  }
  if(!exitflag){
        mq_close(readqueue_fd);
	    mq_unlink(mq_name.c_str());
        close(shared_mem_fd);
        shm_unlink("/goldmemory");
        sem_close(mysemaphore);
        sem_unlink("/goldchase_semaphore");
}

}

