#include "FarmLogic.h"
#include "displayobject.hpp"
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <chrono>

void FarmLogic::run() {
    BakeryStats stats;

    std::srand(std::time(0));
    DisplayObject chicken("chicken", 60, 60, 2, 0);
    DisplayObject chicken2("chicken", 60, 60, 2, 1);
    DisplayObject nest("nest", 80, 60, 0, 2);
    DisplayObject nest2("nest", 80, 60, 0, 3);
    DisplayObject nest1eggs[3] = {
        DisplayObject("egg", 10, 20, 1, 4),
        DisplayObject("egg", 10, 20, 1, 5),
        DisplayObject("egg", 10, 20, 1, 6)
    };

    DisplayObject cow("cow", 60, 60, 2, 7);
    DisplayObject truck("truck", 80, 60, 2, 8);
    DisplayObject farmer("farmer", 30, 60, 2, 9);
    DisplayObject child("child", 30, 60, 2, 10);
    DisplayObject barn1("barn", 100, 100, 0, 11);
    DisplayObject barn2("barn", 100, 100, 0, 12);
    DisplayObject bakery("bakery", 250, 250, 0, 13);
    DisplayObject bakeryeggs[3] = {
        DisplayObject("egg", 10, 20, 1, 14),
        DisplayObject("egg", 10, 20, 1, 15),
        DisplayObject("egg", 10, 20, 1, 16)
    };
    DisplayObject bakeryflour[3] = {
        DisplayObject("flour", 20, 20, 1, 17),
        DisplayObject("flour", 20, 20, 1, 18),
        DisplayObject("flour", 20, 20, 1, 19)
    };
    DisplayObject bakerybutter[3] = {
        DisplayObject("butter", 20, 20, 1, 20),
        DisplayObject("butter", 20, 20, 1, 21),
        DisplayObject("butter", 20, 20, 1, 22)
    };
    DisplayObject bakerysugar[3] = {
        DisplayObject("sugar", 20, 20, 1, 23),
        DisplayObject("sugar", 20, 20, 1, 24),
        DisplayObject("sugar", 20, 20, 1, 25)
    };


    DisplayObject bakerycake[3] = {
        DisplayObject("cake", 20, 20, 1, 26),
        DisplayObject("cake", 20, 20, 1, 27),
        DisplayObject("cake", 20, 20, 1, 28)
    };


    chicken.setPos(400, 300);
    chicken2.setPos(300, 300);
    nest.setPos(100, 500);
    nest2.setPos(700, 500);
    nest1eggs[0].setPos(90, 507);
    nest1eggs[1].setPos(100, 507);
    nest1eggs[2].setPos(110, 507);
    cow.setPos(200, 200);
    truck.setPos(300, 200);
    farmer.setPos(600, 400);
    child.setPos(620, 100);
    barn1.setPos(50, 50);
    barn2.setPos(50, 150);
    bakery.setPos(550, 150);
    bakeryeggs[0].setPos(510, 130);
    bakeryeggs[1].setPos(520, 130);
    bakeryeggs[2].setPos(530, 130);

    bakeryflour[0].setPos(500, 110);
    bakeryflour[1].setPos(520, 110);
    bakeryflour[2].setPos(540, 110);

    bakerysugar[0].setPos(500, 90);
    bakerysugar[1].setPos(520, 90);
    bakerysugar[2].setPos(540, 90);

    bakerybutter[0].setPos(500, 70);
    bakerybutter[1].setPos(520, 70);
    bakerybutter[2].setPos(540, 70);

    bakerycake[0].setPos(600, 200);
    bakerycake[1].setPos(620, 200);
    bakerycake[2].setPos(640, 200);

    chicken.updateFarm();
    chicken2.updateFarm();
    nest.updateFarm();
    nest2.updateFarm();
    nest1eggs[0].updateFarm();
    nest1eggs[1].updateFarm();
    nest1eggs[2].updateFarm();
    cow.updateFarm();
    truck.updateFarm();
    farmer.updateFarm();
    child.updateFarm();
    barn1.updateFarm();
    barn2.updateFarm();
    bakery.updateFarm();
    bakeryeggs[0].updateFarm();
    bakeryeggs[1].updateFarm();
    bakeryeggs[2].updateFarm();

    bakeryflour[0].updateFarm();
    bakeryflour[1].updateFarm();
    bakeryflour[2].updateFarm();

    bakerybutter[0].updateFarm();
    bakerybutter[1].updateFarm();
    bakerybutter[2].updateFarm();

    bakerysugar[0].updateFarm();
    bakerysugar[1].updateFarm();
    bakerysugar[2].updateFarm();

    bakerycake[0].updateFarm();
    bakerycake[1].updateFarm();
    bakerycake[2].updateFarm();

    DisplayObject::redisplay(stats);
    
    int frame = 0;
    int randomNumberX = (std::rand() % 11) - 5;
    int randomNumberY = (std::rand() % 11) - 5;
    
    while (true) {
        
        frame++;
        if(frame % 5 == 0) {
            randomNumberX = (std::rand() % 11) - 5; // Generate a random number between -5 and 5
            randomNumberY = (std::rand() % 11) - 5; // Generate a random number between -5 and 5
        }
        if(frame % 10 == 0) {
            int randEggs = (std::rand() % 3);
            for(int i = 0; i < 3; i++) {
                if (i <= randEggs) {
                    nest1eggs[i].updateFarm();
                } else {
                    nest1eggs[i].erase();
                }
            }
        }

        chicken.setPos(chicken.x + randomNumberX*3, chicken.y  + randomNumberY);
        chicken2.setPos(chicken2.x + randomNumberX, chicken2.y + randomNumberY*3);
        chicken.updateFarm();
        chicken2.updateFarm();
        DisplayObject::redisplay(stats);
        // sleep for 100 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


void FarmLogic::start() {
    std::thread([]() {
       FarmLogic::run();
    })
    .detach();
}