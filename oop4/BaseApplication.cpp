/*
-----------------------------------------------------------------------------
Filename:    BaseApplication.cpp
-----------------------------------------------------------------------------

This source file is part of the
   ___                 __    __ _ _    _
  /___\__ _ _ __ ___  / / /\ \ (_) | _(_)
 //  // _` | '__/ _ \ \ \/  \/ / | |/ / |
/ \_// (_| | | |  __/  \  /\  /| |   <| |
\___/ \__, |_|  \___|   \/  \/ |_|_|\_\_|
      |___/
Tutorial Framework (for Ogre 1.9)
http://www.ogre3d.org/wiki/
-----------------------------------------------------------------------------
*/

#include "BaseApplication.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <macUtils.h>
#endif

//---------------------------------------------------------------------------
BaseApplication::BaseApplication(void)
    : mRoot(0),
    mCamera(0),
    mSceneMgr(0),
    mWindow(0),
    mResourcesCfg(Ogre::StringUtil::BLANK),
    mPluginsCfg(Ogre::StringUtil::BLANK),
    mTrayMgr(0),
    mCameraMan(0),
    mDetailsPanel(0),
    mCursorWasVisible(false),
    mShutDown(false),
    mInputManager(0),
    mMouse(0),
    mKeyboard(0),
    mOverlaySystem(0),
	// oop5
	StackBox(0),
	CodeBox(0),
	RegBox(0),
	eax(0),
	ebp(0),
	esp(0),
	eip(0)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    m_ResourcePath = Ogre::macBundlePath() + "/Contents/Resources/";
#else
    m_ResourcePath = "";
#endif
}

//---------------------------------------------------------------------------
BaseApplication::~BaseApplication(void)
{
    if (mTrayMgr) delete mTrayMgr;
    if (mCameraMan) delete mCameraMan;
    if (mOverlaySystem) delete mOverlaySystem;

    // Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);
    delete mRoot;
}

//---------------------------------------------------------------------------
bool BaseApplication::configure(void)
{
    // Show the configuration dialog and initialise the system.
    // You can skip this and use root.restoreConfig() to load configuration
    // settings if you were sure there are valid ones saved in ogre.cfg.
    if(mRoot->showConfigDialog())
    {
        // If returned true, user clicked OK so initialise.
        // Here we choose to let the system create a default rendering window by passing 'true'.
        mWindow = mRoot->initialise(true, "TutorialApplication Render Window");

        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
void BaseApplication::chooseSceneManager(void)
{
    // Get the SceneManager, in this case a generic one
    mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);

    // Initialize the OverlaySystem (changed for Ogre 1.9)
    mOverlaySystem = new Ogre::OverlaySystem();
    mSceneMgr->addRenderQueueListener(mOverlaySystem);
}
//---------------------------------------------------------------------------
void BaseApplication::createCamera(void)
{
    // Create the camera
    mCamera = mSceneMgr->createCamera("PlayerCam");

    // Position it at 500 in Z direction
    mCamera->setPosition(Ogre::Vector3(0,0,80));
    // Look back along -Z
    mCamera->lookAt(Ogre::Vector3(0,0,-300));
    mCamera->setNearClipDistance(5);

    mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // Create a default camera controller
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void BaseApplication::destroyScene(void)
{
}
//---------------------------------------------------------------------------
void BaseApplication::createViewports(void)
{
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0,0,0));

    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
}
//---------------------------------------------------------------------------
void BaseApplication::setupResources(void)
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(mResourcesCfg);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // OS X does not set the working directory relative to the app.
            // In order to make things portable on OS X we need to provide
            // the loading with it's own bundle path location.
            if (!Ogre::StringUtil::startsWith(archName, "/", false)) // only adjust relative directories
                archName = Ogre::String(Ogre::macBundlePath() + "/" + archName);
#endif

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                archName, typeName, secName);
        }
    }
}
//---------------------------------------------------------------------------
void BaseApplication::createResourceListener(void)
{
}
//---------------------------------------------------------------------------
void BaseApplication::loadResources(void)
{
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}
//---------------------------------------------------------------------------
void BaseApplication::go(void)
{
#ifdef _DEBUG
#ifndef OGRE_STATIC_LIB
    mResourcesCfg = m_ResourcePath + "resources_d.cfg";
    mPluginsCfg = m_ResourcePath + "plugins_d.cfg";
#else
    mResourcesCfg = "resources_d.cfg";
    mPluginsCfg = "plugins_d.cfg";
#endif
#else
#ifndef OGRE_STATIC_LIB
    mResourcesCfg = m_ResourcePath + "resources.cfg";
    mPluginsCfg = m_ResourcePath + "plugins.cfg";
#else
    mResourcesCfg = "resources.cfg";
    mPluginsCfg = "plugins.cfg";
#endif
#endif

    if (!setup())
        return;

    mRoot->startRendering();

    // Clean up
    destroyScene();
}
//---------------------------------------------------------------------------
bool BaseApplication::setup(void)
{
    mRoot = new Ogre::Root(mPluginsCfg);

    setupResources();

    bool carryOn = configure();
    if (!carryOn) return false;

    chooseSceneManager();
    createCamera();
    createViewports();

    // Set default mipmap level (NB some APIs ignore this)
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Create any resource listeners (for loading screens)
    createResourceListener();
    // Load resources
    loadResources();

    // Create the scene
    createScene();

    createFrameListener();

    return true;
};
//---------------------------------------------------------------------------
bool BaseApplication::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if(mWindow->isClosed())
        return false;

    if(mShutDown)
        return false;

    // Need to capture/update each device
    mKeyboard->capture();
    mMouse->capture();

    mTrayMgr->frameRenderingQueued(evt);

    if (!mTrayMgr->isDialogVisible())
    {
        mCameraMan->frameRenderingQueued(evt);   // If dialog isn't up, then update the camera
        if (mDetailsPanel->isVisible())          // If details panel is visible, then update its contents
        {
            mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
            mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
            mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
            mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
            mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
            mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
            mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
        }
    }

    return true;
}
//---------------------------------------------------------------------------
bool BaseApplication::keyReleased(const OIS::KeyEvent &arg)
{
    mCameraMan->injectKeyUp(arg);
    return true;
}
//---------------------------------------------------------------------------
bool BaseApplication::mouseMoved(const OIS::MouseEvent &arg)
{
    if (mTrayMgr->injectMouseMove(arg)) return true;
    mCameraMan->injectMouseMove(arg);
    return true;
}
//---------------------------------------------------------------------------
bool BaseApplication::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    if (mTrayMgr->injectMouseDown(arg, id)) return true;
    mCameraMan->injectMouseDown(arg, id);
    return true;
}
//---------------------------------------------------------------------------
bool BaseApplication::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    if (mTrayMgr->injectMouseUp(arg, id)) return true;
    mCameraMan->injectMouseUp(arg, id);
    return true;
}
//---------------------------------------------------------------------------
// Adjust mouse clipping area
void BaseApplication::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);

    const OIS::MouseState &ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;
}
//---------------------------------------------------------------------------
// Unattach OIS before window shutdown (very important under Linux)
void BaseApplication::windowClosed(Ogre::RenderWindow* rw)
{
    // Only close for window that created OIS (the main window in these demos)
    if(rw == mWindow)
    {
        if(mInputManager)
        {
            mInputManager->destroyInputObject(mMouse);
            mInputManager->destroyInputObject(mKeyboard);

            OIS::InputManager::destroyInputSystem(mInputManager);
            mInputManager = 0;
        }
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool BaseApplication::keyPressed( const OIS::KeyEvent &arg )
{
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if (arg.key == OIS::KC_F)   // toggle visibility of advanced frame stats
    {
        mTrayMgr->toggleAdvancedFrameStats();
    }
    else if (arg.key == OIS::KC_G)   // toggle visibility of even rarer debugging details
    {
        if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE)
        {
            mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
            mDetailsPanel->show();
        }
        else
        {
            mTrayMgr->removeWidgetFromTray(mDetailsPanel);
            mDetailsPanel->hide();
        }
    }
    else if (arg.key == OIS::KC_T)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::TextureFilterOptions tfo;
        unsigned int aniso;

        switch (mDetailsPanel->getParamValue(9).asUTF8()[0])
        {
        case 'B':
            newVal = "Trilinear";
            tfo = Ogre::TFO_TRILINEAR;
            aniso = 1;
            break;
        case 'T':
            newVal = "Anisotropic";
            tfo = Ogre::TFO_ANISOTROPIC;
            aniso = 8;
            break;
        case 'A':
            newVal = "None";
            tfo = Ogre::TFO_NONE;
            aniso = 1;
            break;
        default:
            newVal = "Bilinear";
            tfo = Ogre::TFO_BILINEAR;
            aniso = 1;
        }

        Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
        Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(aniso);
        mDetailsPanel->setParamValue(9, newVal);
    }
    else if (arg.key == OIS::KC_R)   // cycle polygon rendering mode
    {
        Ogre::String newVal;
        Ogre::PolygonMode pm;

        switch (mCamera->getPolygonMode())
        {
        case Ogre::PM_SOLID:
            newVal = "Wireframe";
            pm = Ogre::PM_WIREFRAME;
            break;
        case Ogre::PM_WIREFRAME:
            newVal = "Points";
            pm = Ogre::PM_POINTS;
            break;
        default:
            newVal = "Solid";
            pm = Ogre::PM_SOLID;
        }

        mCamera->setPolygonMode(pm);
        mDetailsPanel->setParamValue(10, newVal);
    }
    else if(arg.key == OIS::KC_F5)   // refresh all textures
    {
        Ogre::TextureManager::getSingleton().reloadAll();
    }
    else if (arg.key == OIS::KC_SYSRQ)   // take a screenshot
    {
        mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
    }
    else if (arg.key == OIS::KC_ESCAPE)
    {
        mShutDown = true;
    }
	// oop5
	else if (arg.key == OIS::KC_SPACE)
	{
		instructionHandler();

	}

    mCameraMan->injectKeyDown(arg);
    return true;
}

void BaseApplication::createFrameListener(void)
{
    Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
    OIS::ParamList pl;
    size_t windowHnd = 0;
    std::ostringstream windowHndStr;

    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

    mInputManager = OIS::InputManager::createInputSystem(pl);

    mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, true));
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, true));

    mMouse->setEventCallback(this);
    mKeyboard->setEventCallback(this);

    // Set initial mouse clipping size
    windowResized(mWindow);

    // Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    mInputContext.mKeyboard = mKeyboard;
    mInputContext.mMouse = mMouse;
    mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mInputContext, this);
    mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
    mTrayMgr->showLogo(OgreBites::TL_BOTTOMRIGHT);
    //mTrayMgr->hideCursor();

    // Create a params panel for displaying sample details
    Ogre::StringVector items;
    items.push_back("cam.pX");
    items.push_back("cam.pY");
    items.push_back("cam.pZ");
    items.push_back("");
    items.push_back("cam.oW");
    items.push_back("cam.oX");
    items.push_back("cam.oY");
    items.push_back("cam.oZ");
    items.push_back("");
    items.push_back("Filtering");
    items.push_back("Poly Mode");
	
    mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
    mDetailsPanel->setParamValue(9, "Bilinear");
    mDetailsPanel->setParamValue(10, "Solid");
    mDetailsPanel->hide();

    mRoot->addFrameListener(this);

	//***********************************
	// oop5
	//***********************************
	// Initializing StackBox
	StackBox = mTrayMgr->createTextBox(OgreBites::TL_TOPRIGHT, "Stack", "", 450, 350);

	//***********************************
	// Initializing CodeBox
	CodeBox = mTrayMgr->createTextBox(OgreBites::TL_TOPLEFT, "Code", "", 450, 500);
	
	TiXmlDocument doc("C:\\Users\\japro_000\\Desktop\\Universe\\power of 5\\OOP\\lab5\\project\\oop4\\sample.xml");
	if ( !doc.LoadFile() )
		exit( 1 );

	TiXmlElement* itemElement = 0;
	for( itemElement = doc.FirstChildElement("instruction");
			 itemElement;
			 itemElement = itemElement->NextSiblingElement() )
	{
		std::string command = itemElement->Attribute("command");
		std::string arg1 = (command != "retn") ? itemElement->Attribute("arg1") : "";
		bool b = command == "mov";
		std::string arg2 = (command != "retn" && command == "mov") ? itemElement->Attribute("arg2") : "";
		struct instruction s;
		s.command = command;

		bool is_hex = false;
		s.arg1 = /* arg1; */ parse_value(arg1, is_hex);
		s.arg2 = /* arg2; */ parse_value(arg2, is_hex);
		s.is_hex = is_hex;

		int size;
		int addr;
		if (command == "mov")
		{
			if (arg2 == "esp" || arg2 == "ebp" || arg2 == "eax")
				size = 2;
			else
				size = 5;
		}
		else if (command == "jmp" || command == "call")
		{
			if (arg1 == "esp" || arg1 == "ebp" || arg1 == "eax")
				size = 2;
			else
				size = 5;
		}
		else if (command.compare("pop") || command.compare("push"))
		{
			if (arg1 == "esp" || arg1 == "ebp" || arg1 == "eax")
				size = 2;
			else
				size = 1;
		}
		else // retn
		{
			size = 1;
		}

		s.size = size;
			
		if (Instructions.size() == 0)
			s.addr = 0;
		else
			s.addr = Instructions[Instructions.size() - 1].addr + Instructions[Instructions.size() - 1].size;

		Instructions.push_back(s);

	}	

	PrintCode();


	//***********************************
	// Initializing RegBox
	RegBox = mTrayMgr->createTextBox(OgreBites::TL_BOTTOMRIGHT, "Reg", "", 300, 150);
	PrintReg();
}

std::string BaseApplication::parse_value(std::string value, bool & is_hex)
{
	is_hex = false;
	if (value.size() > 0 && (value[value.size() - 1] == 'h' || value[value.size() - 1] == 'H') && (value[0] >= '0' && value[0] <= '9'))
	{
		unsigned int x;   
		std::stringstream ss;
		value.pop_back();
		ss << std::hex << value.c_str();
		ss >> x;

		is_hex = true;
		return std::to_string(x);
	}
	else
		return value;
}

unsigned int BaseApplication::hexstr_to_dec(std::string str)
{
	unsigned int x;   
	std::stringstream ss;
	ss << std::hex << str;
	ss >> x;
	return x;
}

void BaseApplication::inst_mov(int idx)
{
	if (Instructions[idx].arg1 == "ebp")
	{
		if (Instructions[idx].arg2 == "esp")
			ebp = esp;
		else if (Instructions[idx].arg2 == "eax")
			ebp = eax;
		else
			ebp = atoi(Instructions[idx].arg2.c_str());
	}
	else if (Instructions[idx].arg1 == "esp")
	{
		if (Instructions[idx].arg2 == "eax")
			esp = eax;
		else if (Instructions[idx].arg2 == "ebp")
			esp = ebp;
		else
			esp = atoi(Instructions[idx].arg2.c_str());

		if (esp == 0) Stack.clear();
		else
		{
			for (int i = Stack.size() - 1; i >= 0; --i)
			{
				if ((unsigned int)esp > (unsigned int)((-1) * i * 4) - 1)
					Stack.pop_back();
				else
					break;
			}
		}
	}
	else if (Instructions[idx].arg1 == "eax")
	{
		if (Instructions[idx].arg2 == "esp")
			eax = esp;
		else if (Instructions[idx].arg2 == "ebp")
			eax = ebp;
		else
			eax = atoi(Instructions[idx].arg2.c_str());
	}
	else
		std::cout << "" << std::endl;

	//eip = (idx + 1 < Instructions.size()) ? Instructions[idx + 1].addr : eip;
	eip = Instructions[idx].addr + Instructions[idx].size;
}

void BaseApplication::inst_push(int idx)
{
	if (Instructions[idx].arg1 == "ebp")
	{
		Stack.push_back(ebp);
	}
	else if (Instructions[idx].arg1 == "esp")
	{
		Stack.push_back(esp);
	}
	else if (Instructions[idx].arg1 == "eax")
	{
		Stack.push_back(eax);
	}
	else
		Stack.push_back(atoi(Instructions[idx].arg1.c_str()));

	esp -= 4;

	//eip = (idx + 1 < Instructions.size()) ? Instructions[idx + 1].addr : eip;
	eip = Instructions[idx].addr + Instructions[idx].size;
}

void BaseApplication::inst_pop(int idx)
{
	if (esp == 0)
		std::cout << "" << std::endl;

	if (Instructions[idx].arg1 == "ebp")
	{
		ebp = Stack[Stack.size() - 1];
	}
	else if (Instructions[idx].arg1 == "esp")
	{
		esp = Stack[Stack.size() - 1];
	}
	else if (Instructions[idx].arg1 == "eax")
	{
		eax = Stack[Stack.size() - 1];
	}
	else
		std::cout << "" << std::endl;
	
	Stack.pop_back();
	esp += 4;

	//eip = (idx + 1 < Instructions.size()) ? Instructions[idx + 1].addr : eip;
	eip = Instructions[idx].addr + Instructions[idx].size;
}

void BaseApplication::inst_jmp(int idx)
{
	if (Instructions[idx].arg1 == "ebp")
	{
		eip = ebp;
	}
	else if (Instructions[idx].arg1 == "esp")
	{
		eip = esp;
	}
	else if (Instructions[idx].arg1 == "eax")
	{
		eip = eax;
	}
	else
		eip = atoi(Instructions[idx].arg1.c_str());
}

void BaseApplication::inst_call(int idx)
{
	Stack.push_back(eip + Instructions[idx].size);
	esp -= 4;

	if (Instructions[idx].arg1 == "ebp")
	{
		eip = ebp;
	}
	else if (Instructions[idx].arg1 == "esp")
	{
		eip = esp;
	}
	else if (Instructions[idx].arg1 == "eax")
	{
		eip = eax;
	}
	else
		eip = atoi(Instructions[idx].arg1.c_str());

	

}

void BaseApplication::inst_retn(int idx)
{
	eip = ebp = Stack[Stack.size() - 1];
	Stack.pop_back();
	esp += 4;
}

void BaseApplication::instructionHandler()
{
	// Find current instruction
	int idx = -1;
	for (int i = 0; i < Instructions.size(); ++i)
	{
		if (Instructions[i].addr >= eip)
		{
			idx = i;
			break;
		}	
	}

	// When all instructions passed
	if (idx == -1)
	{
		PrintCode();
		PrintStack();
		PrintReg();

		return;
	}

	if (Instructions[idx].command == "mov")
	{
		inst_mov(idx);
	}
	else if (Instructions[idx].command == "push")
	{
		inst_push(idx);
	}
	else if (Instructions[idx].command == "pop")
	{
		inst_pop(idx);
	}
	else if (Instructions[idx].command == "jmp")
	{
		inst_jmp(idx);
	}
	else if (Instructions[idx].command == "call")
	{
		inst_call(idx);
	}
	else if (Instructions[idx].command == "retn")
	{
		inst_retn(idx);
	}
	else
		std::cout << "" << std::endl;

	PrintCode();
	PrintReg();
	PrintStack();
}

void BaseApplication::PrintReg()
{
	RegBox->clearText();
	
	std::string str = "";
	
	str += "EBP 0x";
	str += fill_zeros_8(decint_to_hexstr(ebp));
	str += "\n";

	str += "ESP 0x";
	str += fill_zeros_8(decint_to_hexstr(esp));
	str += "\n";

	str += "EAX 0x";
	str += fill_zeros_8(decint_to_hexstr(eax));

	RegBox->setText(str);
}

void BaseApplication::PrintCode()
{
	CodeBox->clearText();

	std::string new_code = "";
	bool is_eiped = false;
	
	for (int i = 0; i < Instructions.size(); ++i)
	{
		if (Instructions[i].addr >= eip && is_eiped == false)
		{
			new_code += "-> 0x";
			is_eiped = true;
		}
		else
		{
			new_code += "   0x";
		}


		std::stringstream stream;
		stream << std::hex << Instructions[i].addr;
		std::string tmp(stream.str());
		for (int j = tmp.size(); j < 4; j++)
			new_code += "0";
		new_code += tmp;
		
		new_code += "  ";
		new_code += Instructions[i].command;
		new_code += " ";
		
		if (Instructions[i].arg1.size() > 0 && Instructions[i].arg1[0] >= '0' && Instructions[i].arg1[0] <= '9' && Instructions[i].is_hex)
			new_code += decint_to_hexstr(atoi(Instructions[i].arg1.c_str())) + 'h';
		else
			new_code += Instructions[i].arg1;
		
		if (Instructions[i].arg2.size() > 0) new_code += ", ";

		if (Instructions[i].arg2.size() > 0 && Instructions[i].arg2[0] >= '0' && Instructions[i].arg2[0] <= '9' && Instructions[i].is_hex)
			new_code += decint_to_hexstr(atoi(Instructions[i].arg2.c_str())) + 'h';
		else
			new_code += Instructions[i].arg2;

		new_code += "\n";

	}

	if (!is_eiped)
	{
		new_code += "->\n";
	}

	CodeBox->setText(new_code);
}

void BaseApplication::PrintStack()
{
	StackBox->clearText();

	std::string new_stack = "";

	for (int i = 0; i < Stack.size(); ++i)
	{
		std::string tmp;
		unsigned int address = -1;
		tmp = decint_to_hexstr(address + 1 - (i + 1) * 4).c_str();
		tmp = "0x" + tmp + "  ";
		tmp += fill_zeros_8(decint_to_hexstr(Stack[i]).c_str());
		tmp += "\n";

		new_stack = tmp + new_stack;
	}
	
	StackBox->setText(new_stack);
}

std::string BaseApplication::decint_to_hexstr(unsigned int dec)
{
	std::stringstream stream;
	stream << std::hex << dec;
	return stream.str();
}

std::string BaseApplication::fill_zeros(std::string str)
{
	std::string tmp = "";
	for (int j = str.size(); j < 4; j++)
		tmp += "0";
	tmp += str;
	return tmp;
}

std::string BaseApplication::fill_zeros_8(std::string str)
{
	std::string tmp = "";
	for (int j = str.size(); j < 8; j++)
		tmp += "0";
	tmp += str;
	return tmp;
}