// #include "FarmLogic.h"
// #include "displayobject.hpp"
// #include <random>
// #include <chrono>
// #include <iostream>
// #include <unordered_map>
// #include <vector>
// #include <atomic>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <memory>
// #include <algorithm>

// // ===================================================================================
// // Globals from FarmLogic.h
// // ===================================================================================
// std::mutex FarmLogic::_farmDisplayMutex;
// std::vector<std::thread> FarmLogic::_workers;
// std::atomic<bool> FarmLogic::_running{true};

// // ===================================================================================
// // RNG helper
// // ===================================================================================
// static inline int randStep(int minv, int maxv) {
//     thread_local std::mt19937 rng(
//         std::random_device{}() ^
//         (std::mt19937::result_type)std::hash<std::thread::id>{}(std::this_thread::get_id()));
//     std::uniform_int_distribution<int> dist(minv, maxv);
//     return dist(rng);
// }

// // ===================================================================================
// // Monitors / shared state (lightweight & deadlock-safe)
// // ===================================================================================
// struct NestMonitor {
//     struct Nest {
//         int eggs = 0;         // 0..3
//         int sitter = -1;      // chicken id or -1
//         int cx = 0, cy = 0;   // nest center (for chickens/farmer to walk to)
//         std::mutex m;
//         std::condition_variable cv_space;  // chickens wait if 3 full or seat taken
//         std::condition_variable cv_empty;  // farmer waits until sitter leaves
//     };
//     std::vector<std::shared_ptr<Nest>> nests;
// };

// struct BarnMonitor {
//     // Barn A: eggs (limited by chickens), butter (unlimited)
//     // Barn B: flour + sugar (unlimited)
//     std::mutex mA;
//     std::condition_variable cv_eggs;
//     int eggs = 0;

//     void addEggs(int k) {
//         std::lock_guard<std::mutex> lk(mA);
//         eggs += k;
//         cv_eggs.notify_all();
//     }
//     void takeEggs(int k) {
//         std::unique_lock<std::mutex> lk(mA);
//         cv_eggs.wait(lk, [&]{ return eggs >= k; });
//         eggs -= k;
//     }
// };

// struct BakeryMonitor {
//     static constexpr int CAP = 6;
//     std::mutex m;
//     std::condition_variable cv_canUnload;      // trucks wait if storage full
//     std::condition_variable cv_canBake;        // oven waits for ingredients/space
//     std::condition_variable cv_cakesAvailable; // children wait for cakes
//     std::condition_variable cv_counterFree;    // exclusive counter

//     // storage caps=6 each
//     int eggs=0, butter=0, flour=0, sugar=0;
//     // stock caps=6
//     int cakes=0;
//     bool ovenBusy=false;
//     bool counterOccupied=false;

//     BakeryStats* stats = nullptr;

//     void unload(int e, int b, int f, int s) {
//         std::unique_lock<std::mutex> lk(m);
//         cv_canUnload.wait(lk, [&]{
//             return (eggs + e <= CAP) && (butter + b <= CAP) &&
//                    (flour + f <= CAP) && (sugar + s <= CAP);
//         });
//         eggs  += e; butter += b; flour += f; sugar += s;
//         // record produced for non-egg items when entering storage
//         {
//             std::lock_guard<std::mutex> glk(FarmLogic::_farmDisplayMutex);
//             if (stats) {
//                 stats->butter_produced += b;
//                 stats->flour_produced  += f;
//                 stats->sugar_produced  += s;
//             }
//         }
//         cv_canBake.notify_all();
//         cv_cakesAvailable.notify_all();
//     }

//     void bakeIfPossible() {
//         std::unique_lock<std::mutex> lk(m);
//         cv_canBake.wait(lk, [&]{
//             return !ovenBusy && eggs>=2 && butter>=2 && flour>=2 && sugar>=2 && (cakes+3)<=CAP;
//         });
//         ovenBusy = true;
//         eggs-=2; butter-=2; flour-=2; sugar-=2;

//         // stats: ingredients used
//         {
//             std::lock_guard<std::mutex> glk(FarmLogic::_farmDisplayMutex);
//             if (stats) {
//                 stats->eggs_used   += 2;
//                 stats->butter_used += 2;
//                 stats->flour_used  += 2;
//                 stats->sugar_used  += 2;
//             }
//         }

//         lk.unlock();
//         std::this_thread::sleep_for(std::chrono::milliseconds(800)); // bake
//         lk.lock();

//         cakes += 3;
//         {
//             std::lock_guard<std::mutex> glk(FarmLogic::_farmDisplayMutex);
//             if (stats) stats->cakes_produced += 3;
//         }

//         ovenBusy = false;
//         cv_cakesAvailable.notify_all();
//         cv_canUnload.notify_all();
//         cv_canBake.notify_all();
//     }

//     void buyCakes(int k) {
//         std::unique_lock<std::mutex> lk(m);
//         cv_counterFree.wait(lk, [&]{ return !counterOccupied; });
//         counterOccupied = true;

//         cv_cakesAvailable.wait(lk, [&]{ return cakes >= k; });
//         cakes -= k;
//         {
//             std::lock_guard<std::mutex> glk(FarmLogic::_farmDisplayMutex);
//             if (stats) stats->cakes_sold += k;
//         }

//         counterOccupied = false;
//         cv_counterFree.notify_one();
//         cv_canBake.notify_all();
//         cv_canUnload.notify_all();
//     }
// };

// // ===================================================================================
// // Part 1 simple movers (kept for cows & simple wanderers)
// // ===================================================================================
// void FarmLogic::moveChicken(DisplayObject& chicken) {
//     while (_running) {
//         int dx = randStep(-5, 5);
//         int dy = randStep(-5, 5);
//         chicken.x = std::clamp(chicken.x + dx, 0, DisplayObject::WIDTH);
//         chicken.y = std::clamp(chicken.y + dy, 0, DisplayObject::HEIGHT);
//         { std::lock_guard<std::mutex> lk(_farmDisplayMutex); chicken.updateFarm(); }
//         std::this_thread::sleep_for(std::chrono::milliseconds(120));
//     }
// }

// void FarmLogic::moveCow(DisplayObject& cow) {
//     while (_running) {
//         int dx = randStep(-3, 3);
//         int dy = randStep(-3, 3);
//         cow.x = std::clamp(cow.x + dx, 0, DisplayObject::WIDTH);
//         cow.y = std::clamp(cow.y + dy, 0, DisplayObject::HEIGHT);
//         { std::lock_guard<std::mutex> lk(_farmDisplayMutex); cow.updateFarm(); }
//         std::this_thread::sleep_for(std::chrono::milliseconds(180));
//     }
// }

// void FarmLogic::moveTruck(DisplayObject& truck, int startX, int endX, int y) {
//     bool forward = true;
//     while (_running) {
//         truck.x += forward ? 5 : -5;
//         if (truck.x >= endX) forward = false;
//         if (truck.x <= startX) forward = true;
//         truck.y = y;
//         { std::lock_guard<std::mutex> lk(_farmDisplayMutex); truck.updateFarm(); }
//         std::this_thread::sleep_for(std::chrono::milliseconds(30));
//     }
// }

// // ===================================================================================
// // Start + Run
// // ===================================================================================
// void FarmLogic::start() {
//     std::thread(run).detach();
// }

// void FarmLogic::run() {
//     BakeryStats stats;

//     // ---------- Entities (same vibe as starter) ----------
//     DisplayObject chicken1("chicken", 60, 60, 2, 1);
//     DisplayObject chicken2("chicken", 60, 60, 2, 2);
//     DisplayObject chicken3("chicken", 60, 60, 2, 3);

//     DisplayObject cow1("cow", 60, 60, 2, 4);
//     DisplayObject cow2("cow", 60, 60, 2, 5);

//     DisplayObject truckEggButter("truck", 80, 60, 2, 6);
//     DisplayObject truckFlourSugar("truck", 80, 60, 2, 7);

//     DisplayObject barnEggButter("barn", 100, 100, 0, 8);
//     DisplayObject barnFlourSugar("barn", 100, 100, 0, 9);

//     DisplayObject nestA("nest", 80, 60, 0, 10);
//     DisplayObject nestB("nest", 80, 60, 0, 11);

//     DisplayObject bakery("bakery", 250, 250, 0, 12);

//     DisplayObject farmer("farmer", 60, 60, 2, 13);
//     DisplayObject child1("child", 50, 50, 2, 14);
//     DisplayObject child2("child", 50, 50, 2, 15);
//     DisplayObject child3("child", 50, 50, 2, 16);
//     DisplayObject child4("child", 50, 50, 2, 17);
//     DisplayObject child5("child", 50, 50, 2, 18);

//     // ---------- Positions ----------
//     chicken1.setPos(120,120);
//     chicken2.setPos(220,120);
//     chicken3.setPos(320,120);

//     cow1.setPos(150,220);
//     cow2.setPos(250,220);

//     barnEggButter.setPos(120,520);
//     barnFlourSugar.setPos(680,520);

//     nestA.setPos(280,480);
//     nestB.setPos(520,480);

//     bakery.setPos(560,160);

//     truckEggButter.setPos(140,420);   // road y=420
//     truckFlourSugar.setPos(660,380);  // road y=380

//     farmer.setPos(400,300);
//     child1.setPos(600,200);
//     child2.setPos(650,200);
//     child3.setPos(700,200);
//     child4.setPos(750,200);
//     child5.setPos(780,200);

//     // ---------- Egg icons near nests (visual) ----------
//     DisplayObject nestA_eggs[3] = {
//         DisplayObject("egg", 10, 20, 1, 101),
//         DisplayObject("egg", 10, 20, 1, 102),
//         DisplayObject("egg", 10, 20, 1, 103)
//     };
//     DisplayObject nestB_eggs[3] = {
//         DisplayObject("egg", 10, 20, 1, 104),
//         DisplayObject("egg", 10, 20, 1, 105),
//         DisplayObject("egg", 10, 20, 1, 106)
//     };
//     for (int i=0;i<3;i++) {
//         nestA_eggs[i].setPos(nestA.x - 10 + i*10, nestA.y + 7);
//         nestB_eggs[i].setPos(nestB.x - 10 + i*10, nestB.y + 7);
//     }

//     // ---------- Bakery storage icons (6 each) ----------
//     auto makeRow = [&](const std::string& tex, int baseId, int x0, int y) {
//         std::vector<DisplayObject> v;
//         for (int i=0;i<6;i++) {
//             v.emplace_back(tex, 20, 20, 1, baseId+i);
//             v.back().setPos(x0 + i*18, y);
//         }
//         return v;
//     };
//     auto bakeryEggIcons   = makeRow("egg",   200, bakery.x-70, bakery.y-35);
//     auto bakeryButterIcons= makeRow("butter",300, bakery.x-70, bakery.y-15);
//     auto bakeryFlourIcons = makeRow("flour", 400, bakery.x-70, bakery.y+5);
//     auto bakerySugarIcons = makeRow("sugar", 500, bakery.x-70, bakery.y+25);
//     auto cakeIcons        = makeRow("cake",  600, bakery.x+40, bakery.y+40);

//     // ---------- Initial draw ----------
//     {
//         std::lock_guard<std::mutex> lk(_farmDisplayMutex);
//         chicken1.updateFarm(); chicken2.updateFarm(); chicken3.updateFarm();
//         cow1.updateFarm(); cow2.updateFarm();
//         truckEggButter.updateFarm(); truckFlourSugar.updateFarm();
//         barnEggButter.updateFarm();  barnFlourSugar.updateFarm();
//         nestA.updateFarm();  nestB.updateFarm();  bakery.updateFarm();
//         farmer.updateFarm();
//         child1.updateFarm(); child2.updateFarm(); child3.updateFarm(); child4.updateFarm(); child5.updateFarm();

//         for (auto& e: nestA_eggs) e.erase();
//         for (auto& e: nestB_eggs) e.erase();
//         for (auto& e: bakeryEggIcons) e.erase();
//         for (auto& e: bakeryButterIcons) e.erase();
//         for (auto& e: bakeryFlourIcons) e.erase();
//         for (auto& e: bakerySugarIcons) e.erase();
//         for (auto& e: cakeIcons) e.erase();

//         DisplayObject::redisplay(stats);
//     }

//     // ---------- Monitors ----------
//     NestMonitor nestMon;
//     {
//         auto nA = std::make_shared<NestMonitor::Nest>(); nA->cx = nestA.x; nA->cy = nestA.y;
//         auto nB = std::make_shared<NestMonitor::Nest>(); nB->cx = nestB.x; nB->cy = nestB.y;
//         nestMon.nests.push_back(nA); nestMon.nests.push_back(nB);
//     }
//     BarnMonitor   barnMon;
//     BakeryMonitor bakeryMon; bakeryMon.stats = &stats;

//     // ---------- Helpers to draw counts (no deadlocks) ----------
//     auto drawNestEggs = [&](NestMonitor::Nest& n, DisplayObject eggsIcons[3]){
//         int count;
//         { std::lock_guard<std::mutex> lk(n.m); count = n.eggs; }
//         std::lock_guard<std::mutex> dlk(_farmDisplayMutex);
//         for (int i=0;i<3;i++) {
//             if (i < count) eggsIcons[i].updateFarm();
//             else eggsIcons[i].erase();
//         }
//     };
//     auto drawBakeryRow = [&](int count, std::vector<DisplayObject>& icons){
//         std::lock_guard<std::mutex> dlk(_farmDisplayMutex);
//         for (int i=0;i<(int)icons.size(); ++i) {
//             if (i < count) icons[i].updateFarm();
//             else icons[i].erase();
//         }
//     };

//     // ---------- Redisplay + stats ticker + icon painter ----------
//     _workers.emplace_back([&]{
//         using namespace std::chrono_literals;
//         int tick = 0;
//         while (_running) {
//             // redraw sprites
//             { std::lock_guard<std::mutex> lk(_farmDisplayMutex); DisplayObject::redisplay(stats); }
//             std::this_thread::sleep_for(100ms); // 10 FPS
//             tick += 100;

//             // paint nest egg icons
//             drawNestEggs(*nestMon.nests[0], nestA_eggs);
//             drawNestEggs(*nestMon.nests[1], nestB_eggs);

//             // snapshot bakery counts and paint
//             int be=0, bb=0, bf=0, bs=0, ck=0;
//             {
//                 std::lock_guard<std::mutex> lk(bakeryMon.m);
//                 be=bakeryMon.eggs; bb=bakeryMon.butter;
//                 bf=bakeryMon.flour; bs=bakeryMon.sugar;
//                 ck=bakeryMon.cakes;
//             }
//             drawBakeryRow(be, bakeryEggIcons);
//             drawBakeryRow(bb, bakeryButterIcons);
//             drawBakeryRow(bf, bakeryFlourIcons);
//             drawBakeryRow(bs, bakerySugarIcons);
//             drawBakeryRow(ck, cakeIcons);

//             // print stats once/sec
//             if (tick >= 1000) {
//                 std::lock_guard<std::mutex> lk(_farmDisplayMutex);
//                 stats.print();
//                 tick = 0;
//             }
//         }
//     });

//     // ---------- Graceful exit (press Enter) ----------
//     _workers.emplace_back([&]{
//         std::cout << "\nPress ENTER to stop simulation...\n";
//         (void)std::cin.get();
//         _running = false;
//         barnMon.cv_eggs.notify_all();
//         bakeryMon.cv_canBake.notify_all();
//         bakeryMon.cv_canUnload.notify_all();
//         bakeryMon.cv_cakesAvailable.notify_all();
//         bakeryMon.cv_counterFree.notify_all();
//     });

//     // ---------- Cows optional animation ----------
//     _workers.emplace_back([&]{ moveCow(cow1); });
//     _workers.emplace_back([&]{ moveCow(cow2); });

//     // ---------- Chickens: simple walk + nest sit/lay + switch nests ----------
//     auto chickenLogic = [&](DisplayObject& me){
//         size_t idx = (me.id % nestMon.nests.size());
//         int laidHere = 0;
//         while (_running) {
//             // random walk toward the active nest
//             auto& target = *nestMon.nests[idx];
//             // step toward target crudely (no occupancy to keep it smooth)
//             int dx = (target.cx > me.x) ? 2 : (target.cx < me.x ? -2 : 0);
//             int dy = (target.cy > me.y) ? 2 : (target.cy < me.y ? -2 : 0);
//             me.x = std::clamp(me.x + dx, 0, DisplayObject::WIDTH);
//             me.y = std::clamp(me.y + dy, 0, DisplayObject::HEIGHT);
//             { std::lock_guard<std::mutex> lk(_farmDisplayMutex); me.updateFarm(); }
//             std::this_thread::sleep_for(std::chrono::milliseconds(20));

//             // close enough to sit/lay?
//             if (std::abs(me.x - target.cx) <= 2 && std::abs(me.y - target.cy) <= 2) {
//                 // sit if allowed (avoid overflow / other sitter)
//                 {
//                     std::unique_lock<std::mutex> lk(target.m);
//                     target.cv_space.wait(lk, [&]{ return !_running ? true : (target.eggs<3 && (target.sitter==-1 || target.sitter==me.id)); });
//                     if (!_running) break;
//                     target.sitter = me.id;
//                 }

//                 int lay = 1 + randStep(0,1);
//                 for (int i=0;i<lay && _running;i++) {
//                     {
//                         std::unique_lock<std::mutex> lk(target.m);
//                         if (target.eggs == 3) break;
//                         target.eggs += 1;
//                         target.cv_space.notify_all();
//                     }
//                     { std::lock_guard<std::mutex> glk(_farmDisplayMutex); stats.eggs_laid += 1; }
//                     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//                 }
//                 {
//                     std::lock_guard<std::mutex> lk(target.m);
//                     if (target.sitter == me.id) target.sitter = -1;
//                     target.cv_empty.notify_all();
//                 }
//                 // switch nest to force movement/coordination
//                 laidHere += lay;
//                 if (laidHere >= 3 || randStep(0,1)) { idx = (idx+1) % nestMon.nests.size(); laidHere = 0; }
//             }
//         }
//     };
//     _workers.emplace_back(chickenLogic, std::ref(chicken1));
//     _workers.emplace_back(chickenLogic, std::ref(chicken2));
//     _workers.emplace_back(chickenLogic, std::ref(chicken3));

//     // ---------- Farmer: collects only when no sitter ----------
//     auto farmerLogic = [&](DisplayObject& me){
//         while (_running) {
//             for (auto& n : nestMon.nests) {
//                 // drift toward nest
//                 int dx = (n->cx > me.x) ? 2 : (n->cx < me.x ? -2 : 0);
//                 int dy = (n->cy > me.y) ? 2 : (n->cy < me.y ? -2 : 0);
//                 me.x = std::clamp(me.x + dx, 0, DisplayObject::WIDTH);
//                 me.y = std::clamp(me.y + dy, 0, DisplayObject::HEIGHT);
//                 { std::lock_guard<std::mutex> g(_farmDisplayMutex); me.updateFarm(); }
//                 std::this_thread::sleep_for(std::chrono::milliseconds(25));

//                 std::unique_lock<std::mutex> lk(n->m);
//                 if (n->eggs > 0) {
//                     n->cv_empty.wait(lk, [&]{ return !_running || n->sitter == -1; });
//                     if (!_running) break;
//                     int take = n->eggs; n->eggs = 0;
//                     lk.unlock();
//                     if (take>0) {
//                         barnMon.addEggs(take); // make eggs available to egg truck
//                     }
//                 }
//             }
//         }
//     };
//     _workers.emplace_back(farmerLogic, std::ref(farmer));

//     // ---------- Trucks (simple back/forth, EB waits for eggs; FS unlimited) ----------
//     _workers.emplace_back([&]{
//         while (_running) {
//             // truckEggButter drives between barnEggButter.x and bakery.x on y=420
//             moveTruck(truckEggButter, std::min(barnEggButter.x, bakery.x), std::max(barnEggButter.x, bakery.x), 420);
//         }
//     });
//     _workers.emplace_back([&]{
//         while (_running) {
//             moveTruck(truckFlourSugar, std::min(barnFlourSugar.x, bakery.x), std::max(barnFlourSugar.x, bakery.x), 380);
//         }
//     });

//     // Logistics loop for EB truck (eggs wait + unload)
//     _workers.emplace_back([&]{
//         while (_running) {
//             barnMon.takeEggs(3);                         // blocks until real eggs exist
//             std::this_thread::sleep_for(std::chrono::milliseconds(120)); // load butter
//             bakeryMon.unload(3,3,0,0);                  // blocks if storage full
//         }
//     });

//     // Logistics loop for FS truck (no wait for source)
//     _workers.emplace_back([&]{
//         while (_running) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(120)); // load flour&sugar (unlimited)
//             bakeryMon.unload(0,0,3,3);
//         }
//     });

//     // ---------- Bakery oven ----------
//     _workers.emplace_back([&]{
//         while (_running) bakeryMon.bakeIfPossible();
//     });

//     // ---------- Children (exclusive counter; wait for k cakes) ----------
//     auto childLogic = [&](DisplayObject& kid){
//         while (_running) {
//             // approach bakery a bit (simple drift)
//             int dx = (bakery.x > kid.x) ? 2 : (bakery.x < kid.x ? -2 : 0);
//             int dy = (bakery.y > kid.y) ? 2 : (bakery.y < kid.y ? -2 : 0);
//             kid.x = std::clamp(kid.x + dx, 0, DisplayObject::WIDTH);
//             kid.y = std::clamp(kid.y + dy, 0, DisplayObject::HEIGHT);
//             { std::lock_guard<std::mutex> g(_farmDisplayMutex); kid.updateFarm(); }
//             std::this_thread::sleep_for(std::chrono::milliseconds(35));

//             if (std::abs(kid.x - bakery.x)<=4 && std::abs(kid.y - bakery.y)<=4) {
//                 int want = 1 + randStep(0,5); // 1..6
//                 bakeryMon.buyCakes(want);
//                 // walk away a little
//                 for (int i=0;i<12 && _running;i++){
//                     kid.x = std::clamp(kid.x + randStep(-6,6), 0, DisplayObject::WIDTH);
//                     kid.y = std::clamp(kid.y + randStep(-6,6), 0, DisplayObject::HEIGHT);
//                     { std::lock_guard<std::mutex> g(_farmDisplayMutex); kid.updateFarm(); }
//                     std::this_thread::sleep_for(std::chrono::milliseconds(45));
//                 }
//             }
//         }
//     };
//     _workers.emplace_back(childLogic, std::ref(child1));
//     _workers.emplace_back(childLogic, std::ref(child2));
//     _workers.emplace_back(childLogic, std::ref(child3));
//     _workers.emplace_back(childLogic, std::ref(child4));
//     _workers.emplace_back(childLogic, std::ref(child5));

//     // ---------- Join ----------
//     for (auto& t : _workers)
//         if (t.joinable()) t.join();
// }