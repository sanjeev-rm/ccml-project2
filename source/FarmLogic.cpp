#include "FarmLogic.h"
#include "displayobject.hpp"
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <array>
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
const int NEST_POSITIONS[3][2] = {{150, 500}, {400, 500}, {650, 500}};
const int BARN1_X = 100;
const int BARN1_Y = 100;
const int BARN2_X = 100;
const int BARN2_Y = 250;
const int BAKERY_X = 650;
const int BAKERY_Y = 150;
const int INTERSECTION_X = 400;
const int INTERSECTION_Y = 200;

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

// Chicken thread - walks between nests and lays eggs
void FarmLogic::chickenThread(int chickenId) {
    std::random_device rd;
    std::mt19937 gen(rd() + chickenId);
    std::uniform_int_distribution<> nestDist(0, 2);
    std::uniform_int_distribution<> eggDist(1, 2);
    
    DisplayObject chicken("chicken", 60, 60, 2, 100 + chickenId);
    chicken.setPos(200 + chickenId * 100, 0);
    
    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        chicken.updateFarm();
    }
    
    auto pickDifferentNest = [&](int exclude) {
        std::array<int, 3> nests = {0, 1, 2};
        std::shuffle(nests.begin(), nests.end(), gen);
        for (int nest : nests) {
            if (nest != exclude) {
                return nest;
            }
        }
        return exclude;
    };

    int nextNest = nestDist(gen);
    bool isCollided = false;
    int collisionStepX = 0;
    int collisionStepY = 0;
    int collisionMovesRemaining = 0;

    const int normalStep = 3;
    const int collisionStepSize = 6;

    auto startCollisionAvoidance = [&]() {
        std::array<std::pair<int, int>, 8> directions = {
            std::make_pair(1, 0), std::make_pair(-1, 0), std::make_pair(0, 1), std::make_pair(0, -1),
            std::make_pair(1, 1), std::make_pair(-1, 1), std::make_pair(1, -1), std::make_pair(-1, -1)
        };
        std::shuffle(directions.begin(), directions.end(), gen);

        for (const auto& dir : directions) {
            int testX = chicken.x;
            int testY = chicken.y;
            bool pathClear = true;

            for (int step = 0; step < 10; ++step) {
                testX += dir.first * collisionStepSize;
                testY += dir.second * collisionStepSize;

                if (testX < 30 || testX > 770 || testY < 30 || testY > 570 ||
                    !canMoveToPosition(testX, testY, chicken.width, chicken.height, chicken.id)) {
                    pathClear = false;
                    break;
                }
            }

            if (pathClear) {
                isCollided = true;
                collisionStepX = dir.first * collisionStepSize;
                collisionStepY = dir.second * collisionStepSize;
                collisionMovesRemaining = 10;
                return true;
            }
        }

        return false;
    };

    auto performCollisionStep = [&](std::chrono::milliseconds sleepDuration) {
        if (!isCollided || collisionMovesRemaining <= 0) {
            isCollided = false;
            collisionMovesRemaining = 0;
            return false;
        }

        int newX = chicken.x + collisionStepX;
        int newY = chicken.y + collisionStepY;

        if (newX < 30 || newX > 770 || newY < 30 || newY > 570 ||
            !canMoveToPosition(newX, newY, chicken.width, chicken.height, chicken.id)) {
            isCollided = false;
            collisionMovesRemaining = 0;
            return false;
        }

        chicken.setPos(newX, newY);
        {
            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
            chicken.updateFarm();
        }

        collisionMovesRemaining--;
        if (collisionMovesRemaining <= 0) {
            isCollided = false;
        }

        std::this_thread::sleep_for(sleepDuration);
        return true;
    };

    while (_running) {
        int targetNest = nextNest;

        // Walk to nest
        int targetX = NEST_POSITIONS[targetNest][0];
        int targetY = NEST_POSITIONS[targetNest][1];

        int initialDistance = std::max(1, std::abs(chicken.x - targetX) + std::abs(chicken.y - targetY));
        int stuckSteps = 0;
        int maxStuckSteps = std::clamp(initialDistance * 4, 200, 1500);
        bool retargetCurrentNest = false;

        while (std::abs(chicken.x - targetX) > 5 || std::abs(chicken.y - targetY) > 5) {
            if (performCollisionStep(std::chrono::milliseconds(50))) {
                continue;
            }

            int dx = (targetX > chicken.x) ? normalStep : (targetX < chicken.x) ? -normalStep : 0;
            int dy = (targetY > chicken.y) ? normalStep : (targetY < chicken.y) ? -normalStep : 0;

            int newX = chicken.x + dx;
            int newY = chicken.y + dy;

            bool moved = false;

            auto tryMove = [&](int moveX, int moveY) {
                if (moveX == 0 && moveY == 0) {
                    return false;
                }

                int candidateX = chicken.x + moveX;
                int candidateY = chicken.y + moveY;

                if (candidateX < 30 || candidateX > 770 || candidateY < 30 || candidateY > 570) {
                    return false;
                }

                if (!canMoveToPosition(candidateX, candidateY, chicken.width, chicken.height, chicken.id)) {
                    return false;
                }

                chicken.setPos(candidateX, candidateY);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    chicken.updateFarm();
                }
                return true;
            };

            std::array<std::pair<int, int>, 8> candidateMoves = {std::make_pair(dx, dy),
                                                                  std::make_pair(dx, 0),
                                                                  std::make_pair(0, dy),
                                                                  std::make_pair(dx, (dy == 0 ? normalStep : dy)),
                                                                  std::make_pair(dx, (dy == 0 ? -normalStep : -dy)),
                                                                  std::make_pair(0, (dy >= 0 ? normalStep : -normalStep)),
                                                                  std::make_pair(0, (dy >= 0 ? 2 * normalStep : -2 * normalStep)),
                                                                  std::make_pair((dx >= 0 ? normalStep : -normalStep), dy)};

            for (const auto& move : candidateMoves) {
                if (tryMove(move.first, move.second)) {
                    moved = true;
                    break;
                }
            }

            // Try to move; if blocked, try just x or just y movement
            if (!moved) {
                if (startCollisionAvoidance()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }

                stuckSteps++;

                if (stuckSteps >= maxStuckSteps) {
                    retargetCurrentNest = true;
                    break;
                }

                int randomOffsetX = (gen() % 40) - 20;
                int randomOffsetY = (gen() % 40) - 20;
                int escapeX = std::max(50, std::min(750, chicken.x + randomOffsetX));
                int escapeY = std::max(50, std::min(550, chicken.y + randomOffsetY));

                if (canMoveToPosition(escapeX, escapeY, chicken.width, chicken.height, chicken.id)) {
                    chicken.setPos(escapeX, escapeY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        chicken.updateFarm();
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            } else {
                isCollided = false;
                collisionMovesRemaining = 0;
                stuckSteps = 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (retargetCurrentNest) {
            int escapeX = chicken.x + ((gen() % 2) ? 100 : -100);
            int escapeY = chicken.y + ((gen() % 2) ? 80 : -80);
            escapeX = std::max(50, std::min(750, escapeX));
            escapeY = std::max(50, std::min(550, escapeY));

            for (int attempt = 0; attempt < 6; attempt++) {
                if (canMoveToPosition(escapeX, escapeY, chicken.width, chicken.height, chicken.id)) {
                    chicken.setPos(escapeX, escapeY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        chicken.updateFarm();
                    }
                    break;
                }
                escapeX += (gen() % 40) - 20;
                escapeY += (gen() % 40) - 20;
                escapeX = std::max(50, std::min(750, escapeX));
                escapeY = std::max(50, std::min(550, escapeY));
            }

            nextNest = pickDifferentNest(targetNest);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            continue;
        }

        // Check if nest is available (NO WAITING - immediate check)
        std::unique_lock<std::mutex> nestLock(_nestMutexes[targetNest]);
        bool canLayEggs = (_nestEggCounts[targetNest] < 3 && !_chickenOnNest[targetNest]);

        // If nest is full or occupied, IMMEDIATELY MOVE TO DIFFERENT NEST
        if (!canLayEggs) {
            nestLock.unlock();

            // FORCE chicken to move FAR AWAY from this nest
            int escapeX = chicken.x + ((gen() % 2) ? 80 : -80);
            int escapeY = chicken.y + ((gen() % 2) ? 80 : -80);
            
            // Keep within bounds
            escapeX = std::max(50, std::min(750, escapeX));
            escapeY = std::max(50, std::min(550, escapeY));
            
            // Force move (try multiple times if needed)
            for (int attempt = 0; attempt < 5; attempt++) {
                if (canMoveToPosition(escapeX, escapeY, chicken.width, chicken.height, chicken.id)) {
                    chicken.setPos(escapeX, escapeY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        chicken.updateFarm();
                    }
                    break;
                }
                escapeX += (gen() % 40) - 20;
                escapeY += (gen() % 40) - 20;
                escapeX = std::max(50, std::min(750, escapeX));
                escapeY = std::max(50, std::min(550, escapeY));
            }

            // Force select a DIFFERENT nest next time
            nextNest = pickDifferentNest(targetNest);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue; // Skip to next iteration with different nest
        }

        _chickenOnNest[targetNest] = true;

        int eggsToLay = std::min(eggDist(gen), 3 - _nestEggCounts[targetNest]);

        for (int i = 0; i < eggsToLay; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            _nestEggCounts[targetNest]++;

            // Update barn eggs
            {
                std::lock_guard<std::mutex> barnLock(_barnEggMutex);
                _barnEggs++;
                DisplayObject::stats.eggs_laid++;
                _barnEggCV.notify_all();
            }

            // Display egg
            DisplayObject egg("egg", 10, 20, 1, 200 + targetNest * 10 + _nestEggCounts[targetNest]);
            egg.setPos(NEST_POSITIONS[targetNest][0] + (_nestEggCounts[targetNest] - 1) * 15 - 15,
                      NEST_POSITIONS[targetNest][1] + 7);
            {
                std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                egg.updateFarm();
            }
        }

        _chickenOnNest[targetNest] = false;
        nestLock.unlock();
        _nestCVs[targetNest].notify_all();

        // Random wandering after laying eggs - move away from nest
        std::uniform_int_distribution<> wanderDist(5, 15);
        int wanderSteps = wanderDist(gen);

        for (int step = 0; step < wanderSteps; step++) {
            if (performCollisionStep(std::chrono::milliseconds(100))) {
                continue;
            }

            // Random direction
            int randDx = (gen() % 7) - 3; // -3 to +3
            int randDy = (gen() % 7) - 3;

            int newX = chicken.x + randDx * 5;
            int newY = chicken.y + randDy * 5;

            // Keep within bounds
            newX = std::max(30, std::min(770, newX));
            newY = std::max(30, std::min(570, newY));

            if (canMoveToPosition(newX, newY, chicken.width, chicken.height, chicken.id)) {
                chicken.setPos(newX, newY);
                {
                    std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                    chicken.updateFarm();
                }
                isCollided = false;
                collisionMovesRemaining = 0;
            } else {
                if (startCollisionAvoidance()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        nextNest = pickDifferentNest(targetNest);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// Cow thread - just stands around
void FarmLogic::cowThread(int cowId) {
    DisplayObject cow("cow", 60, 60, 2, 300 + cowId);
    cow.setPos(250 + cowId * 80, 350);
    
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
    std::random_device rd;
    std::mt19937 gen(rd());
    
    DisplayObject farmer("farmer", 30, 60, 2, 400);
    farmer.setPos(200, 400);
    
    {
        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
        farmer.updateFarm();
    }
    
    int collectionsCount = 0;
    bool isCollided = false;
    int collisionTurnsRemaining = 0;

    while (_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        // Visit each nest
        bool quotaMet = false;
        for (int nestIdx = 0; nestIdx < 3 && !quotaMet; nestIdx++) {
            // Walk to nest
            int targetX = NEST_POSITIONS[nestIdx][0];
            int targetY = NEST_POSITIONS[nestIdx][1] - 50;
            
            int lastX = farmer.x;
            int lastY = farmer.y;
            
            while (std::abs(farmer.x - targetX) > 5 || std::abs(farmer.y - targetY) > 5) {
                // If in collision mode, MOVE OUT OF THE WAY aggressively
                if (isCollided && collisionTurnsRemaining > 0) {
                    // Move to random position away from current location
                    int escapeX = farmer.x + ((gen() % 2) ? 60 : -60);
                    int escapeY = farmer.y + ((gen() % 2) ? 60 : -60);
                    
                    escapeX = std::max(50, std::min(750, escapeX));
                    escapeY = std::max(50, std::min(550, escapeY));
                    
                    if (canMoveToPosition(escapeX, escapeY, farmer.width, farmer.height, farmer.id)) {
                        farmer.setPos(escapeX, escapeY);
                        {
                            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                            farmer.updateFarm();
                        }
                    }
                    
                    collisionTurnsRemaining--;
                    if (collisionTurnsRemaining <= 0) {
                        isCollided = false;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                
                int dx = (targetX > farmer.x) ? 3 : (targetX < farmer.x) ? -3 : 0;
                int dy = (targetY > farmer.y) ? 3 : (targetY < farmer.y) ? -3 : 0;
                
                int newX = farmer.x + dx;
                int newY = farmer.y + dy;
                
                bool moved = false;
                
                if (canMoveToPosition(newX, newY, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(newX, newY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else if (dx != 0 && canMoveToPosition(farmer.x + dx, farmer.y, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(farmer.x + dx, farmer.y);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else if (dy != 0 && canMoveToPosition(farmer.x, farmer.y + dy, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(farmer.x, farmer.y + dy);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else {
                    // COLLISION DETECTED - activate collision mode
                    isCollided = true;
                    collisionTurnsRemaining = 20;
                }
                
                // Reset collision state if moving normally
                if (moved && farmer.x != lastX && farmer.y != lastY) {
                    isCollided = false;
                    collisionTurnsRemaining = 0;
                }
                
                lastX = farmer.x;
                lastY = farmer.y;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            // Collect eggs (wait for chicken to leave nest)
            std::unique_lock<std::mutex> nestLock(_nestMutexes[nestIdx]);
            _nestCVs[nestIdx].wait(nestLock, [nestIdx]() {
                return !_chickenOnNest[nestIdx];
            });
            
            if (_nestEggCounts[nestIdx] > 0) {
                // Remove eggs from display
                for (int i = 0; i < _nestEggCounts[nestIdx]; i++) {
                    DisplayObject egg("egg", 10, 20, 1, 200 + nestIdx * 10 + i + 1);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        egg.erase();
                    }
                }
                _nestEggCounts[nestIdx] = 0;
                collectionsCount++;
                if (collectionsCount >= 3) {
                    quotaMet = true;
                }
            }

            nestLock.unlock();
            _nestCVs[nestIdx].notify_all();
        }

        // After 3 collections, drop off eggs at barn
        if (collectionsCount >= 3) {
            // Walk to barn1 (egg barn)
            int targetX = BARN1_X;
            int targetY = BARN1_Y - 50;
            
            int lastX = farmer.x;
            int lastY = farmer.y;
            
            while (std::abs(farmer.x - targetX) > 5 || std::abs(farmer.y - targetY) > 5) {
                // If in collision mode, MOVE OUT OF THE WAY
                if (isCollided && collisionTurnsRemaining > 0) {
                    int escapeX = farmer.x + ((gen() % 2) ? 60 : -60);
                    int escapeY = farmer.y + ((gen() % 2) ? 60 : -60);
                    
                    escapeX = std::max(50, std::min(750, escapeX));
                    escapeY = std::max(50, std::min(550, escapeY));
                    
                    if (canMoveToPosition(escapeX, escapeY, farmer.width, farmer.height, farmer.id)) {
                        farmer.setPos(escapeX, escapeY);
                        {
                            std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                            farmer.updateFarm();
                        }
                    }
                    
                    collisionTurnsRemaining--;
                    if (collisionTurnsRemaining <= 0) {
                        isCollided = false;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                
                int dx = (targetX > farmer.x) ? 3 : (targetX < farmer.x) ? -3 : 0;
                int dy = (targetY > farmer.y) ? 3 : (targetY < farmer.y) ? -3 : 0;
                
                int newX = farmer.x + dx;
                int newY = farmer.y + dy;
                
                bool moved = false;
                
                if (canMoveToPosition(newX, newY, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(newX, newY);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else if (dx != 0 && canMoveToPosition(farmer.x + dx, farmer.y, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(farmer.x + dx, farmer.y);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else if (dy != 0 && canMoveToPosition(farmer.x, farmer.y + dy, farmer.width, farmer.height, farmer.id)) {
                    farmer.setPos(farmer.x, farmer.y + dy);
                    {
                        std::lock_guard<std::mutex> lock(_farmDisplayMutex);
                        farmer.updateFarm();
                    }
                    moved = true;
                } else {
                    // COLLISION DETECTED
                    isCollided = true;
                    collisionTurnsRemaining = 20;
                }
                
                if (moved && farmer.x != lastX && farmer.y != lastY) {
                    isCollided = false;
                    collisionTurnsRemaining = 0;
                }
                
                lastX = farmer.x;
                lastY = farmer.y;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            
            // Drop off eggs at barn (brief pause to simulate drop-off)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            collectionsCount = 0;
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
    
    // Start 2 trucks (one for eggs/butter, one for flour/sugar)
    _workers.push_back(std::thread(truckThread, 0, true));  // Egg truck
    _workers.push_back(std::thread(truckThread, 1, false)); // Flour/sugar truck
    
    // Start oven
    _workers.push_back(std::thread(ovenThread));
    
    // Start 5 children
    _workers.push_back(std::thread(childThread, 0));
    _workers.push_back(std::thread(childThread, 1));
    _workers.push_back(std::thread(childThread, 2));
    _workers.push_back(std::thread(childThread, 3));
    _workers.push_back(std::thread(childThread, 4));
    
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