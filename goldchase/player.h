#ifndef PLY_H
#define PLY_H

/////
// The Map class uses the Screen class to paint a map
/////
class player {
  public:
        bool getFlag();
        void setFlag(bool fg);
        char getplayerName();
        void setplayerName(unsigned char play);

  private:
    bool flag;
    unsigned char playerName;
};

#endif //PLY_H

