#include <iostream>
#include <list>
#include <unordered_map>
#include <memory>
#pragma once


struct BakeryStats {
    int eggs_laid       = 0;
    int eggs_used       = 0;
    int butter_produced = 0;
    int butter_used     = 0;
    int sugar_produced  = 0;
    int sugar_used      = 0;
    int flour_produced  = 0;
    int flour_used      = 0;
    int cakes_produced  = 0;
    int cakes_sold      = 0;

	void print() const {
        std::cout
          << "\n\n\n\n\n\nBakeryStats:\n"
          << "  eggs_laid:        " << eggs_laid       << "\n"
          << "  eggs_used:        " << eggs_used       << "\n"
          << "  butter_produced:  " << butter_produced << "\n"
          << "  butter_used:      " << butter_used     << "\n"
          << "  sugar_produced:   " << sugar_produced  << "\n"
          << "  sugar_used:       " << sugar_used      << "\n"
          << "  flour_produced:   " << flour_produced  << "\n"
          << "  flour_used:       " << flour_used      << "\n"
          << "  cakes_produced:   " << cakes_produced  << "\n"
          << "  cakes_sold:       " << cakes_sold      << "\n";
    }
};

class DisplayObject {
public:

	//DO NOT CHANGE THE TYPES OR NAMES OF THESE VARIABLES
	int  width;
	int  height;
	int  layer;
	int  x;
	int  y;
	int  id;
	std::string texture;

	void setPos(int, int);
	void setTexture(const std::string&);

	DisplayObject(const std::string&, const int, const int, const int, const int);
	~DisplayObject();
	void updateFarm();
	void erase();

	static void redisplay(BakeryStats& stats);

	//DO NOT CHANGE WIDTH AND HEIGHT
	inline static const int WIDTH = 800;
	inline static const int HEIGHT = 600;

	static std::unordered_map<int, DisplayObject> theFarm;
	static BakeryStats stats;


	//DO NOT CHANGE THE TYPE OF THIS VARIABLE
	static std::shared_ptr<std::unordered_map<int, DisplayObject>> buffedFarmPointer;
	
private:
	
};
