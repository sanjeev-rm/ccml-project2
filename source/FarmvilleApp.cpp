
#include "FarmvilleApp.h"
#include <cstdlib>
#include <ctime>
#include <atomic>
#include "displayobject.hpp"
#include "FarmLogic.h"

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::scene2;

// The number of frames before moving the logo to a new position
#define TIME_STEP 60
// This is adjusted by screen aspect ratio to get the height
#define GAME_WIDTH 800

/**
 * The method called after OpenGL is initialized, but before running the application.
 *
 * This is the method in which all user-defined program intialization should
 * take place.  You should not create a new init() method.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to FOREGROUND,
 * causing the application to run.
 */
void FarmvilleApp::onStartup()
{
    // Create a scene graph the same size as the window
    _scene = Scene2::allocWithHint(Size(GAME_WIDTH, 0));

    // Create a sprite batch (and background color) to render the scene
    _batch = SpriteBatch::alloc();
    setClearColor(Color4(0, 229, 0, 255));
    _scene->setSpriteBatch(_batch);

    _root = OrderedNode::allocWithOrder(OrderedNode::Order::ASCEND);
    _scene->addChild(_root);
    // Create an asset manager to load all assets
    _assets = AssetManager::alloc();

    // You have to attach the individual loaders for each asset type
    _assets->attach<Texture>(TextureLoader::alloc()->getHook());
    _assets->attach<Font>(FontLoader::alloc()->getHook());

    // This reads the given JSON file and uses it to load all other assets
    _assets->loadDirectory("json/assets.json");

    // Activate mouse or touch screen input as appropriate
    // We have to do this BEFORE the scene, because the scene has a button
#if defined(CU_TOUCH_SCREEN)
    Input::activate<Touchscreen>();
#else
    Input::activate<Mouse>();
#endif

    // Build the scene from these assets
    buildScene();
    Application::onStartup();

    // Report the safe area
    Rect bounds = Display::get()->getSafeBounds();
    CULog("Safe Area %sx%s", bounds.origin.toString().c_str(),
          bounds.size.toString().c_str());

    bounds = getSafeBounds();
    CULog("Safe Area %sx%s", bounds.origin.toString().c_str(),
          bounds.size.toString().c_str());

    bounds = getDisplayBounds();
    CULog("Full Area %sx%s", bounds.origin.toString().c_str(),
          bounds.size.toString().c_str());

    std::string root = cugl::Application::get()->getSaveDirectory();
    std::string path = root + "save.json";
    CULog("%s", path.c_str());

    // Start farm simulation
    FarmLogic::start();
}

/**
 * The method called when the application is ready to quit.
 *
 * This is the method to dispose of all resources allocated by this
 * application.  As a rule of thumb, everything created in onStartup()
 * should be deleted here.
 *
 * When overriding this method, you should call the parent method as the
 * very last line.  This ensures that the state will transition to NONE,
 * causing the application to be deleted.
 */
void FarmvilleApp::onShutdown()
{
    // Delete all smart pointers

    // TODO: delete all elements
    _scene = nullptr;
    _batch = nullptr;
    _assets = nullptr;

    // Deativate input
#if defined CU_TOUCH_SCREEN
    Input::deactivate<Touchscreen>();
#else
    Input::deactivate<Mouse>();
#endif
    Application::onShutdown();
}

std::string getTexture(std::string str)
{
    auto slash = str.find_last_of('/');
    // Find position of last '.'
    auto dot = str.find_last_of('.');
    // Extract the substring in between
    return str.substr(slash + 1, dot - (slash + 1));
}

/**
 * The method called to update the application data.
 *
 * This is your core loop and should be replaced with your custom implementation.
 * This method should contain any code that is not an OpenGL call.
 *
 * When overriding this method, you do not need to call the parent method
 * at all. The default implmentation does nothing.
 *
 * @param timestep  The amount of time (in seconds) since the last frame
 */
void FarmvilleApp::update(float timestep)
{
    Size size = getDisplaySize();
    auto current = std::atomic_load_explicit(
        &DisplayObject::buffedFarmPointer,
        std::memory_order_acquire);
    auto map = (*current);
    for (const auto &[key, value] : map)
    {
        if (_elements.count(key) > 0)
        {
            _elements[key]->setPosition(value.x, value.y);
            _elements[key]->setVisible(true);

            if (getTexture(_elements[key]->getTexture()->getName()) != value.texture)
            {
                _elements[key]->setTexture(_assets->get<Texture>(value.texture));
            }
        }
        else
        {
            // create a new element
            std::shared_ptr<scene2::PolygonNode> element = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(value.texture));
            element->setTag(value.id+1);
            element->setPosition(value.x, value.y);
            element->setPriority(value.layer);
            element->setScale(value.width / element->getWidth(), value.height / element->getHeight());
            element->setAnchor(Vec2::ANCHOR_CENTER);
            _root->addChild(element);
            _elements[key] = element;
        }
    }

    auto children = _root->getChildren();
    for (int i = 0; i < children.size(); ++i) {
        auto element = children[i];
        auto id = element->getTag() - 1;
        if (map.count(id) == 0){
            element->setVisible(false);
        }
    }

}

/**
 * The method called to draw the application to the screen.
 *
 * This is your core loop and should be replaced with your custom implementation.
 * This method should OpenGL and related drawing calls.
 *
 * When overriding this method, you do not need to call the parent method
 * at all. The default implmentation does nothing.
 */
void FarmvilleApp::draw()
{
    // This takes care of begin/end
    _scene->render();
}

/**
 * Internal helper to build the scene graph.
 *
 * Scene graphs are not required.  You could manage all scenes just like
 * you do in 3152.  However, they greatly simplify scene management, and
 * have become standard in most game engines.
 */
void FarmvilleApp::buildScene()
{
}
