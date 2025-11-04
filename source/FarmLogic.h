#pragma once
#include "displayobject.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <array>

class FarmLogic {
public:
    static void start();
    static void run();
    static std::mutex _farmDisplayMutex; // protects updateFarm, erase, redisplay
    static std::vector<std::thread> _workers; 
    static std::atomic<bool> _running;
    
    // Nest synchronization (3 nests)
    static std::mutex _nestMutexes[3];
    static std::condition_variable _nestCVs[3];
    static int _nestEggCounts[3];
    static bool _chickenOnNest[3];
    
    // Bakery storage synchronization
    static std::mutex _bakeryStorageMutex;
    static std::condition_variable _bakeryStorageCV;
    static int _bakeryEggs;
    static int _bakeryButter;
    static int _bakeryFlour;
    static int _bakerySugar;
    
    // Bakery stock (finished cakes)
    static std::mutex _bakeryStockMutex;
    static std::condition_variable _bakeryStockCV;
    static int _bakeryCakes;
    
    // Oven synchronization
    static std::mutex _ovenMutex;
    static std::condition_variable _ovenCV;
    static bool _ovenBusy;
    
    // Barn egg production
    static std::mutex _barnEggMutex;
    static std::condition_variable _barnEggCV;
    static int _barnEggs;
    
    // Truck intersection synchronization
    static std::mutex _intersectionMutex;
    static std::condition_variable _intersectionCV;
    static bool _intersectionOccupied;
    
    // Position synchronization for layer 2 objects (moving entities)
    static std::mutex _positionMutex;
    
    // Shop synchronization (only one child at a time)
    static std::mutex _shopMutex;
    
private:
    // Helper functions
    static void chickenThread(int chickenId);
    static void cowThread(int cowId);
    static void truckThread(int truckId, bool isEggTruck);
    static void farmerThread();
    static void childThread(int childId);
    static void ovenThread();
    static void redisplayThread();
    
    static bool checkCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    static bool canMoveToPosition(int x, int y, int width, int height, int myId);
};
