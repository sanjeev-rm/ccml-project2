## Goal: 
Gain hands-on experience with C++ threads in a simple graphics environment.

## Task: 
Our homework is inspired by the former Facebook game called Farmville. If you were a developer working on that project, you might create a thread to animate a chicken: it would peck around in the yard, maybe find a nest and lay an egg in it, etc. Each action updates the position of the chicken image in the game state (a graph of objects), and then the game redraws the whole game state at some frequency, perhaps 30 frames per second or 60 frames per second.

## Given code: 
We are providing a framework that utilizes the CUGL graphics package from CS 5152. In `FarmLogic.cpp`, we include a program intended to illustrate how you’ll use it – but the scene it shows makes no real sense! It really just illustrates the kinds of predrawn objects available to you, and sort of scatters them around, and animates a few of them in minor ways. For example, this is a screenshot we made of it:

![example](example.png)

## Code overview: 
- This project is using [CUGL](https://www.cs.cornell.edu/courses/cs5152/2025sp/resources/engine/) from CS 5152. Most of the graphics have been abstracted away, so students who wish only to complete the threading exercise do not need to worry about it. However, there will be an extra credit incentive for those who wish to make their farm simulation look nice (detailed below). 
- Your main logic should go in `FarmLogic::run()`. It should be possible to complete this assignment by only touching `FarmLogic`.
  - Those who wish to add more advanced functionality or graphics may edit the rest of the code. Sections that should not be touched will be clearly marked.
- `DisplayObject::redisplay()` sends a snapshot of the simulation state to the graphics framework to be drawn.
  - Try to call this method at least 10 times per second (10 frames per second).
- All on-screen objects are represented as rectangles with a width and a height.
- The positions of objects are center anchored.
  - For example, an chicken at (x,y) = (100,100) and (width,height) = (40,40) would have verticies at (80,80),(120,80),(120,120),(80,120).
- The Farm is 800 pixels wide and 600 pixels tall. The bottom left is (0,0), and the top right is (800,600).
- Each object has an ID field. This value should be unique for all objects.
- Each object has a layer field. Objects with a higher layer value will be rendered above objects with lower layer values.
  - We recommend layer=0 for stationary objects (e.g. nests, barns, bakery), layer=1 for items (e.g. eggs, flour, butter), and layer=2 for moving objects (e.g. chicken, cow, farmer).
  - Objects in the same layer should not collide. We will be testing for this in Part 2.
- Each object has the `setTexture` and `setPos` methods to change its texture or position.
- Each object has an `updateFarm` and `erase` method. `updateFarm` updates (and if needed, inserts) its value in the shared farm state object, and `erase` removes the object from the shared farm state object. When these are called, it will be reflected on screen upon the following `redisplay()` call.

## How to run:
### ugclinux (easiest option for Windows/Linux)
- If on Windows, use WSL Ubuntu 24.04
- ssh into ugclinux using the -Y flag. For example, `ssh -Y netid@ugclinux.cs.cornell.edu`
- Clone this repo in your ugclinux user
- Python dependencies: PyYAML, Pillow, shortuuid.
  - If you don't want to edit your system packages, you may create a virtual Python environment to install these packages
  - `pip install PyYAML Pillow shortuuid`
- `./compile.sh` to compile
- `./run.sh` to run
- After your first build, you can use `./compile.sh skip-cugl` to speed up compilation
### Natively (easiest option for Mac)
###### MacOS:
- If you don't have cmake installed, install cmake. The easiest way to do so is `brew install cmake`
- Python dependencies: PyYAML, Pillow, shortuuid.
  - If you don't want to edit your system packages, you may create a virtual Python environment to install these packages
  - `pip install PyYAML Pillow shortuuid`
- `./compile.sh` to compile
- `./run.sh` to run
- After your first build, you can use `./compile.sh skip-cugl` to speed up compilation
###### Windows/Linux
- If Windows, use WSL Ubuntu 24.04
- Install the following packages: `sudo apt update && sudo apt install -y libglew-dev libxext-dev freeglut3-dev uuid-dev python3-pip cmake`
- Python dependencies: PyYAML, Pillow, shortuuid.
  - If you don't want to edit your system packages, you may create a virtual Python environment to install these packages
  - `pip install PyYAML Pillow shortuuid`
- `./compile.sh` to compile
- `./run.sh` to run
- After your first build, you can use `./compile.sh skip-cugl` to speed up compilation
### Docker:
- Make sure you have Docker installed and make sure the engine is running
- From the Farmville project repository, and inside the folder run `docker build -t farmville .`. You only need to do this build step once
- Once that builds, run this command from within the farmville project repository to start the docker container: `docker run --rm -e PUID=1000 -e PGID=1000 -e TZ=America/New_York -p 3000:3000 --shm-size="1gb" -v "$(pwd)":/config/Desktop/farmville farmville`. You can stop the container and re-run it using this command without rebuilding the container
- Once that starts the container, go to your browser and visit `http://localhost:3000`
- You should see a virtual desktop with a folder on it called "farmville". This folder is connected to your local farmville repo folder. Changes in your local folder will show up in the docker container
- Inside the virtual desktop, right click and select "Open Terminal Here". Then, cd into the farmville folder and use `./compile.sh` to compile and `./run.sh` to run
- After your first build, you can edit the farmville source code on your local computer, and use `./compile.sh skip-cugl` in the virtual desktop to speed up compilation
### VM:
- If neither native nor Docker works for you, come to office hours or post on Ed and a TA will try to help you. If nothing works, the TA will help you set up a VM which should be guaranteed to work

## The scenario:
- We have a set of barns that produce eggs, flour, butter and sugar.
  - The screen definitely has room for two barns, so we will have one that produces butter and eggs, and a second barn that produces flour and sugar.
  - The farm will have a few nests, initially empty. You definitely will want at least two nests but more may be useful.
- The butter/eggs barn can produce unlimited amounts of butter, and the flour/sugar barn can make unlimited flour and sugar. But, the butter/eggs barn's egg output will need to match what the chickens lay.
- You should have a non-trivial amount of animals visible: at least three chickens and two cows.
- Chickens basically walk from nest to nest, wait for space, and then lay one or more eggs in the nest.
  - A nest with 3 eggs is full.
  - You will want a pretty rapid supply of eggs. Even so, the chickens should sometimes move around – they can’t just sit on a nest laying eggs continuously.
  - At a minimum, your chickens must change to a different nest at least once every 3 eggs laid, and they do this by walking, not teleportation.
- From time to time the farmer comes and collects the eggs. This empties the nests.
- You can animate the cows, or just have them standing around.
- The farmer can milk the cow now and then, but you would need to add animations in CUGL for that.
  - We are not requiring that you show the farmer milking the cow, but the best looking simulated farms will receive extra credit (detailed below).
- There should be two trucks. One of them drives back and forth to the butter and egg barn, and the other drives to the flour and sugar barn.
  - On arrival, each loads up with a full load of produce, then takes it to the bakery for storage.
  - A truck never needs to “wait” for butter, flour or sugar, but may need to wait for eggs, if it arrives when the barn doesn’t any available.
  - The truck unloads into the bakery, and then can go fetch more produce.
- A full load of produce “fills a truck”.
  - This will be three of each: three eggs, three boxes of butter, three bags of flour, three bags of sugar.
  - Notice that the eggs are still a limiting factor, because in our setup we can see them being laid. If the barn has extra eggs, the truck can’t take them all in one load: it would carry three, then come back for three more, etc.
- Again, keep in mind that moving is a step by step process.
  - A truck doesn’t teleport from the barn to the bakery: it has to follow some form of path (road) from barn to bakery and back. If you like, you can add a background image for the roads you will use.
- Now, let’s focus on the bakery. The bakery's storage has a capacity of 6 of each item, and when it is full for any given item, the truck must wait for space to unload.
- There is an oven, which requires two of each item to bake a batch of cakes.
  - You can bake a batch of cakes only if the oven is free (no prior batch is baking), and if there is room for the batch in the "stock" of the bakery. This stock area has room for 6 cakes, and each baked batch is 3 cakes.
- Children come to buy the cakes. They can buy as few as one cake or as many as six, randomly. Only one child can enter the shop at a time.
- There should be five children, and all of them stand around waiting to buy a cake, then walk away (to take to be eaten), and then return for another cake. We should be able to see all five children at all times.
- A child will wait (in the shop) if he or she is trying to buy k cakes, but there are currently less than k in stock. When more are baked, that child continues to buy them until it reaches the target number.
  - So for example, a child who wants 5 cakes and enters when there is just 1 left would need to wait for two more batches (3 cakes each!) to be ready. Then there would be 2 remaining when that child leaves the store (1 + 6 – 5). As the number of cakes changes, make sure the stock shows an accurate number of cakes.
- In addition to displaying the farm, we also track how many eggs have been laid and how many have been used up, how much butter, etc. The statistics are shown in the console. The system just runs endlessly, but shouldn’t ever “lose” products!


**Notice the various synchronization conditions!**
- We aren’t allowing children or chickens or trucks or other objects at the same layer to occupy the same space at the same time. You’ll need to enforce this. If your trucks follow roads that cross, they will need to be careful at the intersections or a crash could occur!
- A nest isn’t allowed to overflow: once it has 3 eggs in it, that nest is full until the farmer empties it. If a nest ever has 4 eggs, that would be a synchronization error.
- A truck must be emptied before it can do its next trip to the farm, but emptying it requires space in the bakery's storage for all the products it carried. It must also be full when it leaves the farm.
- The oven can't bake until it can take the required ingredients from storage and until there is space in the bakery's stock
- There may be additional requirements that we haven’t mentioned, for example to avoid having chickens or trucks crash into each other. (And you are welcome to extend the basic setup, but if you do, it would probably add more synchronization requirements).
- There are also C++ language synchronization requirements. Look for variables that need to be updated or read in critical sections and be sure to protect them properly!


## Your job:
- Our existing program has a displayable object class and creates some basic objects, which it displays in a pretty random way so that you can see them. Then it loops animating one or two things, again in a totally random way to demonstrate the capability.
- Your task is create one thread per moving object, which would loop and show the object as it moves around. Use the monitor style of synchronization, and use monitor condition variables to wait for specific things.
- Additionally, do the redisplay action in a separate thread that loops: it should redisplay, sleep for a while, then repeat. Aim for at least 10 frames per second (100ms sleep time).
- Keep your bakery stats updated
- **PLEASE NOTE**: Our example program (as given to you) uses sleep_for. This is NOT the recommended way for event-based threads to pause. For threads that need to take actions when certain events happen, they should use the condition variable wait_until operation.

## How to submit (Both parts 1 and 2):
- Submit a zip of your source folder to Gradescope
- Also submit a 15-30 second video of your simulation running to Gradescope
- (Part 2 only) A short document called report.pdf that explains your thread safety decisions. This does not need to be long, 0.5 pages is plenty
- (Part 2 only) If you would like to be considered for extra credit (extra credit is for part 2 only), submit a file called extracredit.txt that explains what extra steps you took to make your farm look nice

## Part 1: Due on 10/20 
For this part of the assignment, we want you to implement all the needed threads to do concurrent animation of all the moving parts, with proper layout on the screen (you have to decide where to put each thing), but without implementing any of the logic for threads interacting with each other. 
- For example, you won’t worry about chickens walking right over each other, or trucks colliding. You won’t worry that the oven needs to coordinate with the stock or even that it needs two units of each ingredient to make a batch of cakes – just have it work randomly, like in our given code. Basically, any rule in the application that involves two threads talking to one another is in part 2, and if you are unsure, just ask on Ed.

Have `redisplay` called from a separate thread that loops, redisplays, sleeps for a while, then repeats. Note that the starter code does not have redisplay in a separate thread, so you will have to fix it
Even so, there is one form of synchronization required! Our `obj.updateFarm`, `obj.erase`, and `DisplayObject.redisplay` methods are not thread safe because they all read or write from the same farm representation map. This means that two threads can never call any of `updateFarm`, `erase`, or `redisplay` simultaneously, even if they are calling it on different entities. Part 1 will be buggy if you do not implement this one form of mutual exclusion.

The simulation should run until `^C` or until the window is closed.

#### Requirements for Part 1:
- For part 1, we will look at your logic for ensuring that your `redisplay()`, `updateFarm()`, and `erase()` do not enter the critical section concurrently, do not deadlock, and do not livelock.
- Your locks on these functions should also be narrowly scoped. This means that you should not acquire the farm update/redisplay lock until you actually need to update or redisplay it, so logic for determining and setting positions for your entities should live outside the lock meant for display updates.
- You should have all of the required entities as per [The Scenario](https://github.coecis.cornell.edu/jah649/farmville?tab=readme-ov-file#the-scenario) on screen. The entities with movement should be in their own thread and moving. Random movement is fine for part 1
- These are the only things we will evaluate for part 1

This is an open-ended project, so the exact implementation details for how you do this are up to you!

## Part 2: Due on 11/03
For this part, add to your part 1 all the missing logic for all the synchronization required to fully implement the application.

What we will evaluate:
- You must have all of the required components in your farm as described in [The Scenario](https://github.coecis.cornell.edu/jah649/farmville?tab=readme-ov-file#the-scenario).
- We will run your program and make sure that the animation seems to be correct and implementing our various rules as per [The Scenario](https://github.coecis.cornell.edu/jah649/farmville?tab=readme-ov-file#the-scenario) (e.g. no objects on the same layer collide, there must be at least 2 of each item for the oven to bake a batch, chickens walk from nest to nest, etc). 
- We will check that you aren’t losing produce (like eggs that vanish).
- We will also check to see that your code has no deadlocks or livelocks caused by the extra synchronization required to implement part 2.
- Don't do something like using a single mutex lock for your entire simulation. Logic that can run in parallel should be able to run in parallel. For example, the thread logic that determines whether the bakery has enough ingredients to bake a cake should not be using the same lock as logic that ensures entities don't overlap on the screen
- We will ensure that you are making use of condition variables and reader/writer patterns where appropriate 
- The exact details of how you enforce thread safety are up to you
- A short writeup called report.pdf that explains the decisions you made regarding thread safety. 0.5-1 pages for this should suffice


Additional requirements for part 2:
1. At least one situation where chickens must check for and coordinate with other chickens to avoid "crashing into one-another".
2. At least one situation where two or more chickens contend for the same nest.
3. At least one nest where the eggs would not come all from one chicken (e.g. not "one chicken lays all 3 eggs" but "1 egg was from chicken A, the other 2 from chicken B").
4. You must protect against excess eggs per nest. A chicken can't lay a 4th egg in a nest.
5. The farmer cannot collect eggs from a nest while a chicken is sitting on it.
6. A truck cannot collect eggs that have not actually been produced yet.
7. The trucks must have some potential for reaching some form of intersection at the same time, and must coordinate so that first one goes through, then the other, and it can't always be that truck B always waits until after truck A passes. The rule has to depend on who gets there in what order and when, not some kind of rigid thing that might leave truck B waiting ten minutes until truck A happens to pass, for example.
8. A child who has his or her turn to buy cakes gets to wait until he or she has the proper number of cakes.
9. Children can't walk over one-another.

## Extra credit (for Part 2 only):
- This extra credit will be awarded for part 2 only. We will not evaluate extra credit for part 1
- If you wish to be considered for extra credit, do not forget to submit extracredit.txt on Gradescope
- This is an open-ended assignment, and we will be awarding extra credit to students who go beyond the basic requirements
- Students who use customize their farm look in some non-trivial way will be guaranteed to receive at least a little bit of extra credit. A way to satisfy this could be to add a nice texture for the bakery and changing the textures for some of the assets. Something extremely simple like changing the background color will not satisfy this
- Students who go further will get even more extra credit. Some examples would be:
  - Adding animations to actions like milking the cow or moving the truck
  - Ensuring that trucks and animals actually face the direction that they are moving in
  - Having more moving parts or functionality than what was required 
- The best looking simulations we come across will be shown in class
