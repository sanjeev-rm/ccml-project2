#pragma once
#include "displayobject.hpp"
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>

class FarmLogic {
public:
    static void start();
    static void run();
    static std::mutex _farmDisplayMutex; // protects updateFarm, erase, redisplay
    static std::vector<std::thread> _workers; 
    static std::atomic<bool> _running;
    
private:
    
    // Helper functions for moving entities
    static void moveChicken(DisplayObject& chicken);
    static void moveCow(DisplayObject& cow);
    static void moveTruck(DisplayObject& truck, int startX, int endX, int y);
};
