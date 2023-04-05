#include "goldchase.h"
#include "Map.h"
#include<fstream>
#include<string>
#include <iostream>

int main(int argc, char** argv)
{

  std::ifstream mapfile(argv[1]);
  std::string line;
  getline(mapfile,line);
  int num_gold=stoi(line);
  int rows=0, cols=0;

  while(getline(mapfile, line))
  {
    ++rows;
    cols=line.length()>cols ? line.length() : cols;
  }
  mapfile.close();
  mapfile.open(argv[1]);
  getline(mapfile, line); //discard num gold
  unsigned char* thegoldmine=new unsigned char[rows*cols]{0};
  int current_byte=0;
  while(getline(mapfile, line))
  {
    for(int i=0; i<line.length(); ++i)
    {
      if(line[i]=='*')
        thegoldmine[current_byte]=G_WALL;
      ++current_byte;
    }
    current_byte+=cols-line.length();
  }
  mapfile.close();

 Map goldMine(thegoldmine,rows,cols);
  goldMine.postNotice("This is a notice");
  while(goldMine.getKey()!='Q')
    ;//empty loop

    }
