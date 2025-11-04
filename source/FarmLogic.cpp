#include "FarmLogic.h"
#include "displayobject.hpp"
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <random>
#include <utility>

// Initialize static members
std::mutex FarmLogic::_farmDisplayMutex;
std::vector<std::thread> FarmLogic::_workers;
std::atomic<bool> FarmLogic::_running{true};

std::mutex FarmLogic::_nestMutexes[3];
std::condition_variable FarmLogic::_nestCVs[3];
int FarmLogic::_nestEggCounts[3] = {0, 0, 0};
bool FarmLogic::_chickenOnNest[3] = {false, false, false};
std::array<std::array<int, 3>, 3> FarmLogic::_nestEggIds = {{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}};
std::atomic<int> FarmLogic::_nextEggId{1000};

std::mutex FarmLogic::_bakeryStorageMutex;
std::condition_variable FarmLogic::_bakeryStorageCV;
int FarmLogic::_bakeryEggs = 0;
int FarmLogic::_bakeryButter = 0;
int FarmLogic::_bakeryFlour = 0;
int FarmLogic::_bakerySugar = 0;

std::mutex FarmLogic::_bakeryStockMutex;
std::condition_variable FarmLogic::_bakeryStockCV;
int FarmLogic::_bakeryCakes = 0;

std::mutex FarmLogic::_ovenMutex;
std::condition_variable FarmLogic::_ovenCV;
bool FarmLogic::_ovenBusy = false;

std::mutex FarmLogic::_barnEggMutex;
std::condition_variable FarmLogic::_barnEggCV;
int FarmLogic::_barnEggs = 0;

std::mutex FarmLogic::_intersectionMutex;
std::condition_variable FarmLogic::_intersectionCV;
bool FarmLogic::_intersectionOccupied = false;

std::mutex FarmLogic::_positionMutex;
std::mutex FarmLogic::_shopMutex;

// Constants for positions
const int NEST_POSITIONS[3][2] = {{300, 140}, {400, 80}, {500, 140}};
const int CHICKEN_WAIT_POSITIONS[3][2] = {{400, 260}, {440, 300}, {360, 300}};
const int BARN1_X = 100;
const int BARN1_Y = 100;
const int BARN2_X = 100;
const int BARN2_Y = 250;
const int BAKERY_X = 650;
const int BAKERY_Y = 150;
const int INTERSECTION_X = 400;
const int INTERSECTION_Y = 200;
const int FARMER_REST_X = BARN1_X;
const int FARMER_REST_Y = BARN1_Y + 80;

// Helper function to check collision between two rectangles
bool FarmLogic::checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    // Center-anchored rectangles
    int left1 = x1 - w1/2, right1 = x1 + w1/2;
    int bottom1 = y1 - h1/2, top1 = y1 + h1/2;
    int left2 = x2 - w2/2, right2 = x2 + w2/2;
    int bottom2 = y2 - h2/2, top2 = y2 + h2/2;
    
    return !(right1 < left2 || left1 > right2 || top1 < bottom2 || bottom1 > top2);
}

// Check if can move to position without colliding with other layer 2 objects
bool FarmLogic::canMoveToPosition(int x, int y, int width, int height, int myId) {
    std::lock_guard<std::mutex> lock(_positionMutex);
    for (const auto& pair : DisplayObject::theFarm) {
        if (pair.second.id != myId && pair.second.layer == 2) {
            if (checkCollision(x, y, width, height,
                             pair.second.x, pair.second.y,
                             pair.second.width, pair.second.height)) {
                return false;
            }
        }
    }
    return true;
}

void FarmLogic::moveEntity(DisplayObject& entity, int targetX, int targetY, int step, int delayMs) {
    while (_running && entity.x != targetX) {
        int diff = targetX - entity.x;
        int delta = (std::abs(diff) < step) ? diff : (diff > 0 ? step : -step);
        int newX = entity.x + delta;
        if (canMoveToPosition(newX, entity.y, entity.width, entity.height, entity.id)) {
            entity.setPos(newX, entity.y);
            {
                std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                entity.updateFarm();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    while (_running && entity.y != targetY) {
        int diff = targetY - entity.y;
        int delta = (std::abs(diff) < step) ? diff : (diff > 0 ? step : -step);
        int newY = entity.y + delta;
        if (canMoveToPosition(entity.x, newY, entity.width, entity.height, entity.id)) {
            entity.setPos(entity.x, newY);
            {
                std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                entity.updateFarm();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

// Chicken thread - walks between nests and lays eggs
void FarmLogic::chickenThread(int chickenId) {
    DisplayObject chicken("chicken", 60, 60, 2, 100 + chickenId);
    int targetNest = chickenId % 3;
    chicken.setPos(NEST_POSITIONS[targetNest][0], NEST_POSITIONS[targetNest][1]);

    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        chicken.updateFarm();
    }

    const int stepSize = 4;

    while (_running) {
        {
            std::unique_lock<std::mutex> nestLock(_nestMutexes[targetNest]);
            _nestCVs[targetNest].wait(nestLock, [&]() {
                return !_running || (_nestEggCounts[targetNest] == 0 && !_chickenOnNest[targetNest]);
            });
            if (!_running) {
                break;
            }
            _chickenOnNest[targetNest] = true;
        }

        moveEntity(chicken, NEST_POSITIONS[targetNest][0], NEST_POSITIONS[targetNest][1], stepSize, 60);

        for (int eggIndex = 0; eggIndex < 3 && _running; ++eggIndex) {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            int eggId = _nextEggId.fetch_add(1);
            DisplayObject egg("egg", 10, 20, 1, eggId);

            int offsetX = (eggIndex - 1) * 15;
            int offsetY = 25 + (eggIndex == 1 ? 6 : 0);
            egg.setPos(NEST_POSITIONS[targetNest][0] + offsetX,
                      NEST_POSITIONS[targetNest][1] + offsetY);

            {
                std::lock_guard<std::mutex> displayLock(_farmDisplayMutex);
                egg.updateFarm();
            }

            {
                std::lock_guard<std::mutex> nestLock(_nestMutexes[targetNest]);
                _nestEggIds[targetNest][eggIndex] = eggId;
                _nestEggCounts[targetNest] = eggIndex + 1;
            }

            {
                std::lock_guard<std::mutex> statsLock(_barnEggMutex);
                DisplayObject::stats.eggs_laid++;
            }
        }

        {
            std::lock_guard<std::mutex> nestLock(_nestMutexes[targetNest]);
            _nestCVs[targetNest].notify_all();
        }

        moveEntity(chicken,
                   CHICKEN_WAIT_POSITIONS[chickenId][0],
                   CHICKEN_WAIT_POSITIONS[chickenId][1],
                   stepSize,
                   60);

        {
            std::lock_guard<std::mutex> nestLock(_nestMutexes[targetNest]);
            _chickenOnNest[targetNest] = false;
        }
        _nestCVs[targetNest].notify_all();

        targetNest = (targetNest + 1) % 3;
    }
}

// Cow thread - just stands around
void FarmLogic::cowThread(int cowId) {
    DisplayObject cow("cow", 60, 60, 2, 300 + cowId);
    cow.setPos(700 + cowId * 40, 450);

    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        cow.updateFarm();
    }

    while (_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// Farmer thread - collects eggs from nests
void FarmLogic::farmerThread() {
    DisplayObject farmer("farmer", 30, 60, 2, 400);
    farmer.setPos(FARMER_REST_X, FARMER_REST_Y);

    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        farmer.updateFarm();
    }

    const int stepSize = 4;

    while (_running) {
        for (int nestIdx = 0; nestIdx < 3 && _running; ++nestIdx) {
            std::unique_lock<std::mutex> waitLock(_nestMutexes[nestIdx]);
            _nestCVs[nestIdx].wait(waitLock, [&]() {
                return !_running || (_nestEggCounts[nestIdx] >= 3 && !_chickenOnNest[nestIdx]);
            });
            if (!_running) {
                return;
            }
            waitLock.unlock();

            moveEntity(farmer, NEST_POSITIONS[nestIdx][0], NEST_POSITIONS[nestIdx][1] - 70, stepSize, 60);

            std::array<int, 3> eggsToCollect{};
            {
                std::lock_guard<std::mutex> collectLock(_nestMutexes[nestIdx]);
                eggsToCollect = _nestEggIds[nestIdx];
                _nestEggCounts[nestIdx] = 0;
                _nestEggIds[nestIdx].fill(0);
            }

            int collected = 0;
            {
                std::lock_guard<std::mutex> displayLock(_farmDisplayMutex);
                for (int eggId : eggsToCollect) {
                    if (eggId != 0) {
                        DisplayObject::theFarm.erase(eggId);
                        collected++;
                    }
                }
                farmer.updateFarm();
            }

            if (collected > 0) {
                std::lock_guard<std::mutex> barnLock(_barnEggMutex);
                DisplayObject::stats.eggs_used += collected;
            }

            _nestCVs[nestIdx].notify_all();

            moveEntity(farmer, FARMER_REST_X, FARMER_REST_Y, stepSize, 60);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

// Truck thread - transports goods from barn to bakery
void FarmLogic::truckThread(int truckId, bool isEggTruck) {
    DisplayObject truck("truck", 80, 60, 2, 500 + truckId);
    int startX = isEggTruck ? BARN1_X : BARN2_X;
    int startY = isEggTruck ? BARN1_Y : BARN2_Y;
    truck.setPos(startX, startY);
    
    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        truck.updateFarm();
    }
    
    while (_running) {
        // Load at barn
        if (isEggTruck) {
            // Wait for 3 eggs from barn
            std::unique_lock<std::mutex> barnLock(_barnEggMutex);
            _barnEggCV.wait(barnLock, []() { return _barnEggs >= 3; });
            _barnEggs -= 3;
            barnLock.unlock();
            
            // Also produce butter at this barn
            DisplayObject::stats.butter_produced += 3;
        } else {
            // Produce flour and sugar at this barn
            DisplayObject::stats.flour_produced += 3;
            DisplayObject::stats.sugar_produced += 3;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Drive to bakery (through intersection)
        int targetX = BAKERY_X;
        int targetY = BAKERY_Y;
        
        while (std::abs(truck.x - targetX) > 5 || std::abs(truck.y - targetY) > 5) {
            int dx = (targetX > truck.x) ? 4 : (targetX < truck.x) ? -4 : 0;
            int dy = (targetY > truck.y) ? 4 : (targetY < truck.y) ? -4 : 0;
            
            int newX = truck.x + dx;
            int newY = truck.y + dy;
            
            // Check if approaching intersection
            if (std::abs(newX - INTERSECTION_X) < 50 && std::abs(newY - INTERSECTION_Y) < 50) {
                std::unique_lock<std::mutex> intLock(_intersectionMutex);
                _intersectionCV.wait(intLock, []() { return !_intersectionOccupied; });
                _intersectionOccupied = true;
                intLock.unlock();
                
                // Cross intersection
                while (std::abs(truck.x - INTERSECTION_X) < 50 && std::abs(truck.y - INTERSECTION_Y) < 50) {
                    if (canMoveToPosition(newX, newY, truck.width, truck.height, truck.id)) {
                        truck.setPos(newX, newY);
                        {
                            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                            truck.updateFarm();
                        }
                    }
                    
                    dx = (targetX > truck.x) ? 4 : (targetX < truck.x) ? -4 : 0;
                    dy = (targetY > truck.y) ? 4 : (targetY < truck.y) ? -4 : 0;
                    newX = truck.x + dx;
                    newY = truck.y + dy;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                
                intLock.lock();
                _intersectionOccupied = false;
                intLock.unlock();
                _intersectionCV.notify_all();
            } else {
                if (canMoveToPosition(newX, newY, truck.width, truck.height, truck.id)) {
                    truck.setPos(newX, newY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        truck.updateFarm();
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Unload at bakery storage (wait for space)
        std::unique_lock<std::mutex> storageLock(_bakeryStorageMutex);
        if (isEggTruck) {
            _bakeryStorageCV.wait(storageLock, []() {
                return _bakeryEggs <= 3 && _bakeryButter <= 3;
            });
            _bakeryEggs += 3;
            _bakeryButter += 3;
        } else {
            _bakeryStorageCV.wait(storageLock, []() {
                return _bakeryFlour <= 3 && _bakerySugar <= 3;
            });
            _bakeryFlour += 3;
            _bakerySugar += 3;
        }
        storageLock.unlock();
        _bakeryStorageCV.notify_all();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Return to barn (through intersection)
        targetX = startX;
        targetY = startY;
        
        while (std::abs(truck.x - targetX) > 5 || std::abs(truck.y - targetY) > 5) {
            int dx = (targetX > truck.x) ? 4 : (targetX < truck.x) ? -4 : 0;
            int dy = (targetY > truck.y) ? 4 : (targetY < truck.y) ? -4 : 0;
            
            int newX = truck.x + dx;
            int newY = truck.y + dy;
            
            // Check intersection
            if (std::abs(newX - INTERSECTION_X) < 50 && std::abs(newY - INTERSECTION_Y) < 50) {
                std::unique_lock<std::mutex> intLock(_intersectionMutex);
                _intersectionCV.wait(intLock, []() { return !_intersectionOccupied; });
                _intersectionOccupied = true;
                intLock.unlock();
                
                while (std::abs(truck.x - INTERSECTION_X) < 50 && std::abs(truck.y - INTERSECTION_Y) < 50) {
                    if (canMoveToPosition(newX, newY, truck.width, truck.height, truck.id)) {
                        truck.setPos(newX, newY);
                        {
                            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                            truck.updateFarm();
                        }
                    }
                    
                    dx = (targetX > truck.x) ? 4 : (targetX < truck.x) ? -4 : 0;
                    dy = (targetY > truck.y) ? 4 : (targetY < truck.y) ? -4 : 0;
                    newX = truck.x + dx;
                    newY = truck.y + dy;
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                
                intLock.lock();
                _intersectionOccupied = false;
                intLock.unlock();
                _intersectionCV.notify_all();
            } else {
                if (canMoveToPosition(newX, newY, truck.width, truck.height, truck.id)) {
                    truck.setPos(newX, newY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        truck.updateFarm();
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// Oven thread - bakes cakes when ingredients are available
void FarmLogic::ovenThread() {
    while (_running) {
        std::unique_lock<std::mutex> ovenLock(_ovenMutex);
        _ovenCV.wait(ovenLock, []() { return !_ovenBusy; });
        
        // Check if we have ingredients and space for cakes
        std::unique_lock<std::mutex> storageLock(_bakeryStorageMutex);
        _bakeryStorageCV.wait(storageLock, []() {
            return _bakeryEggs >= 2 && _bakeryButter >= 2 && 
                   _bakeryFlour >= 2 && _bakerySugar >= 2;
        });
        
        std::unique_lock<std::mutex> stockLock(_bakeryStockMutex);
        _bakeryStockCV.wait(stockLock, []() { return _bakeryCakes <= 3; });
        
        // Take ingredients
        _bakeryEggs -= 2;
        _bakeryButter -= 2;
        _bakeryFlour -= 2;
        _bakerySugar -= 2;
        DisplayObject::stats.eggs_used += 2;
        DisplayObject::stats.butter_used += 2;
        DisplayObject::stats.flour_used += 2;
        DisplayObject::stats.sugar_used += 2;
        
        storageLock.unlock();
        _bakeryStorageCV.notify_all();
        
        _ovenBusy = true;
        ovenLock.unlock();
        
        // Bake cakes
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // Add cakes to stock
        _bakeryCakes += 3;
        DisplayObject::stats.cakes_produced += 3;
        
        stockLock.unlock();
        _bakeryStockCV.notify_all();
        
        ovenLock.lock();
        _ovenBusy = false;
        ovenLock.unlock();
        _ovenCV.notify_all();
    }
}

// Child thread - walks to bakery and buys cakes
void FarmLogic::childThread(int childId) {
    std::random_device rd;
    std::mt19937 gen(rd() + childId);
    std::uniform_int_distribution<> cakeDist(1, 6);
    
    DisplayObject child("child", 30, 60, 2, 600 + childId);
    child.setPos(BAKERY_X + 100 + childId * 40, 50);
    
    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        child.updateFarm();
    }
    
    while (_running) {
        int cakesWanted = cakeDist(gen);
        
        // Walk to shop entrance
        int targetX = BAKERY_X;
        int targetY = BAKERY_Y - 80;
        
        while (std::abs(child.x - targetX) > 5 || std::abs(child.y - targetY) > 5) {
            int dx = (targetX > child.x) ? 2 : (targetX < child.x) ? -2 : 0;
            int dy = (targetY > child.y) ? 2 : (targetY < child.y) ? -2 : 0;
            
            int newX = child.x + dx;
            int newY = child.y + dy;
            
            if (canMoveToPosition(newX, newY, child.width, child.height, child.id)) {
                child.setPos(newX, newY);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            } else if (dx != 0 && canMoveToPosition(child.x + dx, child.y, child.width, child.height, child.id)) {
                child.setPos(child.x + dx, child.y);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            } else if (dy != 0 && canMoveToPosition(child.x, child.y + dy, child.width, child.height, child.id)) {
                child.setPos(child.x, child.y + dy);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Enter shop (only one child at a time)
        std::lock_guard<std::mutex> shopLock(_shopMutex);
        
        // Wait for enough cakes to be available
        std::unique_lock<std::mutex> stockLock(_bakeryStockMutex);
        _bakeryStockCV.wait(stockLock, [cakesWanted]() {
            return _bakeryCakes >= cakesWanted;
        });
        
        _bakeryCakes -= cakesWanted;
        DisplayObject::stats.cakes_sold += cakesWanted;
        
        stockLock.unlock();
        _bakeryStockCV.notify_all();
        
        // Walk away
        targetX = BAKERY_X + 100 + childId * 40;
        targetY = 50;
        
        while (std::abs(child.x - targetX) > 5 || std::abs(child.y - targetY) > 5) {
            int dx = (targetX > child.x) ? 2 : (targetX < child.x) ? -2 : 0;
            int dy = (targetY > child.y) ? 2 : (targetY < child.y) ? -2 : 0;
            
            int newX = child.x + dx;
            int newY = child.y + dy;
            
            if (canMoveToPosition(newX, newY, child.width, child.height, child.id)) {
                child.setPos(newX, newY);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            } else if (dx != 0 && canMoveToPosition(child.x + dx, child.y, child.width, child.height, child.id)) {
                child.setPos(child.x + dx, child.y);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            } else if (dy != 0 && canMoveToPosition(child.x, child.y + dy, child.width, child.height, child.id)) {
                child.setPos(child.x, child.y + dy);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    child.updateFarm();
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// Redisplay thread - updates display at 10 FPS
void FarmLogic::redisplayThread() {
    while (_running) {
        {
            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
            DisplayObject::redisplay(DisplayObject::stats);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void FarmLogic::run() {
    std::srand(std::time(0));
    _running = true;
    _workers.clear();

    for (int i = 0; i < 3; ++i) {
        _nestEggCounts[i] = 0;
        _chickenOnNest[i] = false;
        _nestEggIds[i].fill(0);
    }
    _nextEggId.store(1000);
    _barnEggs = 0;
    
    // Create static farm objects (layer 0 - stationary)
    DisplayObject barn1("barn", 100, 100, 0, 10);
    barn1.setPos(BARN1_X, BARN1_Y);
    
    DisplayObject barn2("barn", 100, 100, 0, 11);
    barn2.setPos(BARN2_X, BARN2_Y);
    
    DisplayObject bakery("bakery", 250, 250, 0, 12);
    bakery.setPos(BAKERY_X, BAKERY_Y);
    
    // Create nests
    DisplayObject nest1("nest", 80, 60, 0, 13);
    nest1.setPos(NEST_POSITIONS[0][0], NEST_POSITIONS[0][1]);
    
    DisplayObject nest2("nest", 80, 60, 0, 14);
    nest2.setPos(NEST_POSITIONS[1][0], NEST_POSITIONS[1][1]);
    
    DisplayObject nest3("nest", 80, 60, 0, 15);
    nest3.setPos(NEST_POSITIONS[2][0], NEST_POSITIONS[2][1]);
    
    // Update static objects to farm
    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        barn1.updateFarm();
        barn2.updateFarm();
        bakery.updateFarm();
        nest1.updateFarm();
        nest2.updateFarm();
        nest3.updateFarm();
        DisplayObject::redisplay(DisplayObject::stats);
    }
    
    // Start all worker threads
    _workers.push_back(std::thread(redisplayThread));
    
    // Start 3 chickens
    _workers.push_back(std::thread(chickenThread, 0));
    _workers.push_back(std::thread(chickenThread, 1));
    _workers.push_back(std::thread(chickenThread, 2));
    
    // Start 2 cows
    _workers.push_back(std::thread(cowThread, 0));
    _workers.push_back(std::thread(cowThread, 1));
    
    // Start farmer
    _workers.push_back(std::thread(farmerThread));
    
    // Wait for all threads
    for (auto& worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}


void FarmLogic::start() {
    std::thread([]() {
       FarmLogic::run();
    })
    .detach();
}