
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "ClientGame.h"

#include "PrimeEngine/PrimeEngineIncludes.h"
#include "PrimeEngine/RenderJob.h"
#include "PrimeEngine/GameThreadJob.h"
#include "PrimeEngine/Application/Application.h"
#include "PrimeEngine/APIAbstraction/Effect/PEDepthStencilState.h"
#include "PrimeEngine/Scene/Light.h"


#ifdef _XBOX
#include <xbdm.h>
#endif

#if APIABSTRACTION_PS3
#include <cell/sysmodule.h>
#include <sys/process.h>
#include <sys/spu_initialize.h>
#include <sys/paths.h>
#endif

// Static member variables
PE::Handle PE::Components::ClientGlobalGameCallbacks::s_gameHandle;


PE::Components::ClientGame::EngineInitParams PE::Components::ClientGame::EngineInitParams::s_params;
PE::GameContext PE::Components::ClientGame::s_context;
PE::Components::ClientGlobalGameCallbacks::StaticGameConstruct PE::Components::ClientGlobalGameCallbacks::s_constructFunction = &PE::Components::ClientGame::ConstructCallback;
PE::Components::ClientGlobalGameCallbacks::StaticInitEngine PE::Components::ClientGlobalGameCallbacks::s_initEngineFunction = &PE::Components::ClientGame::InitEngineCallback;

int TestExternIntVar;
 
 namespace PE {

using namespace Events;


	// gloabl vars
	volatile bool g_drawThreadInitialized;
	volatile bool g_drawThreadCanStart;
	volatile bool g_drawThreadShouldExit;
	volatile bool g_drawThreadExited;

	volatile bool g_gameThreadInitialized;
	volatile bool g_gameThreadCanStart;
	volatile bool g_gameThreadShouldExit;
	volatile bool g_gameThreadExited;

	Threading::Mutex g_drawThreadInitializationLock;
	Threading::ConditionVariable g_drawThreadInitializedCV(g_drawThreadInitializationLock);

	Threading::Mutex g_gameThreadInitializationLock;
	Threading::ConditionVariable g_gameThreadInitializedCV(g_gameThreadInitializationLock);
	
	Threading::Mutex g_drawThreadLock;
	Threading::ConditionVariable g_drawCanStartCV(g_drawThreadLock);

	Threading::PEThread g_drawThread, g_gameThread;

namespace Components {
	int ClientGame::initEngine(GameContext &context, PE::MemoryArena arena, EngineInitParams &engineParams)
{
    PEINFO("PYENGINE LAUNCHING\n");
    
#ifdef _XBOX
    {
        HRESULT hr = DmMapDevkitDrive();
        if (SUCCEEDED(hr))
        {
            PEINFO("PE: PROGRESS: Mounted DEVKIT:\\\n");
        }
        else
        {
            PEINFO("PE: ERROR: PROGRESS: Failed to Mount DEVKIT:\\\n");
        }
    }
#endif
#if defined(SN_TARGET_PS3)
    
    pthread_attr_t attr;
    size_t stackSize = 0;
    size_t newStackSize = 512*1024;
    
    int rc = pthread_attr_init(&attr);
    if(rc != 0) { 
        return -1;
    }
    
    pthread_attr_getstacksize(&attr, &stackSize);
    
    printf("PE: PROGRESS: Checking stack size = %d\nChanging to %d ..", (int)(stackSize), (int)(newStackSize));
    
    rc = pthread_attr_setstacksize(&attr, newStackSize);
    if(rc != 0) {
        printf("PE: ERROR: pthread_attr_setstacksize failed errcode = %d", rc);
        return -1;
    }
    
    pthread_attr_getstacksize(&attr, &stackSize);
    printf("PE: PROGRESS: Checking stack size = %d\n", (int)(stackSize));
    
    
    // Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
    sys_spu_initialize(6, 1);	
    
    int ret;
    
    ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
    if (ret < 0) {
        printf("cellSysmoduleLoadModule() failed (%d)\n", ret);
        exit(1);
    }

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
    switch( ret )
    {
        case CELL_OK:
            // The module is successfully loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_DUPLICATED:
            // The module was already loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_UNKNOWN:
        case CELL_SYSMODULE_ERROR_FATAL:
            printf("!! Failed to load CELL_SYSMODULE_FS\n" ); 
            printf("!! Exiting Program \n" ); 
            exit(1);
    }
        
    ret = cellSysmoduleLoadModule(CELL_SYSMODULE_USBD);
    switch( ret )
    {
        case CELL_OK:
            // The module is successfully loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_DUPLICATED:
            // The module was already loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_UNKNOWN:
        case CELL_SYSMODULE_ERROR_FATAL:
            printf("!! Failed to load CELL_SYSMODULE_USBD\n" ); 
            printf("!! Exiting Program \n" ); 
            exit(1);
    }
        
    ret = cellSysmoduleLoadModule(CELL_SYSMODULE_IO);
    switch( ret )
    {
        case CELL_OK:
            // The module is successfully loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_DUPLICATED:
            // The module was already loaded,
            break;
                
        case CELL_SYSMODULE_ERROR_UNKNOWN:
        case CELL_SYSMODULE_ERROR_FATAL:
            printf("!! Failed to load CELL_SYSMODULE_IO\n" ); 
            printf("!! Exiting Program \n" ); 
            exit(1);
    }
    
#endif

#if PE_PLAT_IS_PSVITA
	int ret = sceSysmoduleLoadModule( SCE_SYSMODULE_NET);
	PEASSERT( ret == SCE_OK, "ret=%#x \n", ret );

	// file open testing
	FILE *f = fopen("host0:/test.txt", "rt");
	fclose(f);
#endif
	
	/*
	DIR *dir;
	struct dirent *entry;
	char name[] = ".";

	if (!(dir = opendir(name)))
		return;
	if (!(entry = readdir(dir)))
		return;
	
	do {
		if (entry->d_type == DT_DIR) {
			char path[1024];
			int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			printf("%*s[%s]\n", level*2, "", entry->d_name);
			listdir(path, level + 1);
		}
		else
			printf("%*s- %s\n", level*2, "", entry->d_name);
	} while (entry = readdir(dir));
	closedir(dir);
	*/
    srand((unsigned int)(time(NULL)));
    
    MemoryManager::Construct();

    PEINFO("PE: PROGRESS: MemoryManager Constructed\n");
    
	{
		PE::Handle handle("MAIN_FUNCTION_ARGS", sizeof(MainFunctionArgs));
		#if PE_PLAT_IS_WIN32
			context.m_pMPArgs = new(handle) MainFunctionArgs(context, PE::MemoryArena_Client, engineParams.lpCmdLine, engineParams.hInstance);
		#else
			context.m_pMPArgs = new(handle) MainFunctionArgs(context, PE::MemoryArena_Client, engineParams.lpCmdLine);
		#endif
	}

	Handle handle("LUA_COMPONENT", sizeof(ClientLuaEnvironment));
	context.m_pLuaEnv = new(handle) ClientLuaEnvironment(context, PE::MemoryArena_Client, handle);
	context.getLuaEnvironment()->registerInitialLibrariesFunctions();
	// non component classes that need to register..
	MaterialCPU::SetLuaFunctions(context.getLuaEnvironment(), context.getLuaEnvironment()->L);
	context.getLuaEnvironment()->run();

	PEINFO("PE: PROGRESS: LuaEnvironment Constructed and LuaEnvironmenr script has been run\n");
	
	PE::Register(context.getLuaEnvironment(), PE::GlobalRegistry::Instance()); // calling global registry of engine components


	PEINFO("PE: PROGRESS: Registered all PE Components and Events\n");
	
	{
		PE::Handle handle("LOG_COMPONENT", sizeof(Log));
		context.m_pLog = new(handle) Log(context, arena, handle);
		context.getLog()->addDefaultComponents();
	}
	
	context.getLuaEnvironment()->addDefaultComponents(); // end lua component initialization

	PEINFO("PE: PROGRESS: LuaEnvironment Constructed (Added event handlers now that the events are registered)\n");

	{
		Handle h("ClientNetworkManager", sizeof(ClientNetworkManager));
		context.m_pNetworkManager = new (h) ClientNetworkManager(context, arena, h);
		context.getNetworkManager()->addDefaultComponents();
	}

	// register classes with network that could not be registered through constructor because network manager wasnt created
	context.getLuaEnvironment()->m_networkId = Networkable::s_NetworkId_LuaEnvironment;
	context.getLuaEnvironment()->registerWithNetwork(context.getNetworkManager());

	// create application window/ios app
	{
		Application::Construct(context, engineParams.m_windowRes.m_xi, engineParams.m_windowRes.m_yi, engineParams.m_windowCaption);
	}

	{
		IRenderer::Construct(context, engineParams.m_windowRes.m_xi, engineParams.m_windowRes.m_yi);
	}
	PEINFO("SizeOf(ptr) is %d\n", sizeof(char *));
    PEINFO("PE: PROGRESS: IRenderer Constructed\n");

	{
		Handle handle("GAMEOBJECTMANAGER", sizeof(GameObjectManager));
		context.m_pGameObjectManager = new(handle) GameObjectManager(context, arena, handle);
		context.getGameObjectManager()->addDefaultComponents();
	}
	PEINFO("PE: PROGRESS: GameObjectManager Constructed\n");

	Input::Construct(context, PE::MemoryArena_Client);
    
    PEINFO("PE: PROGRESS: Input Constructed\n");
    
    // Input do_UPDATE() will put events on input queue
    context.getGameObjectManager()->addComponent(Input::s_hMyself);
    
    
    //Construct Single Event Handlers
    
	#if PE_ENABLE_GPU_PROFILING
		Profiling::Profiler::Construct();
	#endif

    SingleHandler_DRAW::Construct(context, PE::MemoryArena_Client);
    PESSEH_CHANGE_TO_DEBUG_SHADER::Construct(context, PE::MemoryArena_Client);
    PESSEH_POP_SHADERS::Construct(context, PE::MemoryArena_Client);
    PESSEH_DRAW_Z_ONLY::Construct(context, PE::MemoryArena_Client);
                
    Events::EventQueueManager::Construct(context, PE::MemoryArena_Client);
    
#ifdef PYENGINE_USE_PHYSX
    Physics::Initialize();
    //Physics::LoadScene(); //Loading city/floor generated phys rep. At this point NVIDIA has not provided an exporter for MAYA
    Physics::LoadDefaults();
#endif
	SamplerStateManager::ConstructAndInitialize(context, arena);
	PERasterizerStateManager::ConstructAndInitialize(context, arena);
    PEAlphaBlendStateManager::ConstructAndInitialize(context, arena);
    PEDepthStencilStateManager::ConstructAndInitialize(context, arena);

    EffectManager::Construct(context, PE::MemoryArena_Client);
    
    PEINFO("PE: PROGRESS: EffectManager Constructed\n");

	 
	VertexBufferGPUManager::Construct(context, PE::MemoryArena_Client);

	{
		Handle h("MeshManager", sizeof(MeshManager));
		context.m_pMeshManager = new (h) MeshManager(context, arena, h);
		context.getMeshManager()->addDefaultComponents();
	}


    // This will load default effects (shaders)
    EffectManager::Instance()->loadDefaultEffects();
    
    GPUTextureManager::Construct(context, MemoryArena_Client);
    AnimationSetGPUManager::Construct(context, MemoryArena_Client);
    
    PositionBufferCPUManager::Construct(context, MemoryArena_Client);
    NormalBufferCPUManager::Construct(context, MemoryArena_Client);
    TexCoordBufferCPUManager::Construct(context, MemoryArena_Client);
    
    DrawList::Construct(context, MemoryArena_Client);
    
    RootSceneNode::Construct(context, MemoryArena_Client);
	DebugRenderer::Construct(context, MemoryArena_Client);
	CameraManager::Construct(context, MemoryArena_Client);
    
    // initialize timer functionality
    Timer::Initialize();

	context.getGameObjectManager()->addComponent(context.getNetworkManager()->getHandle());

	context.getNetworkManager()->initNetwork();
	
	//Liu

	EffectManager::Instance()->randomLightInfo(100);


	// Add a directional light
	Handle hLight("LIGHT", sizeof(Light));
	Vector3 sunDir = EffectManager::Instance()->getSkybox()->GetSunDirection();
	Vector3 sunColor = EffectManager::Instance()->getSkybox()->GetSunColor();
	Vector4 sunColorVec4 = Vector4(sunColor.m_x, sunColor.m_y, sunColor.m_z, 1.0f);
	
	Light *pLight = new(hLight)Light(
		context,
		arena,
		hLight,
		Vector3(5, 0, 0), //Position
		Vector3(0, 0, 0),
		Vector3(0, 0, 0),
		sunDir, //Direction (z-axis)
		Vector4(0, 0, 0, 1), //Ambient
		sunColorVec4, //Diffuse
		Vector4(0, 0, 0, 1), //Specular
		Vector3(0.05, 0.05, 0.05), //Attenuation (x, y, z)
		1, // Spot Power
		20, //Range
		false, //Whether or not it casts shadows
		1,//0 = point, 1 = directional, 2 = spot
		Vector3(1, 0, 0),
		0
		);
	RootSceneNode::Instance()->m_lights.add(hLight);


	return 1;
}
        
                
                
int ClientGame::initGame()
{
    // we intitalise game in one thread before laucnhing all the different threads
	m_pContext->getGPUScreen()->AcquireRenderContextOwnership(m_pContext->m_gameThreadThreadOwnershipMask);
	initNoise();
    return 1;
}
void ClientGame::runGameFrameStatic()
{
    ClientGlobalGameCallbacks::getGameInstance()->runGameFrame();
}

void ClientGame::dummyIdleFunction()
{
	#if APIABSTRACTION_OGL && APIABSTRACTION_GLPC
		//glutPostRedisplay();
	#endif
}

int ClientGame::runGame()
{
    // game thread no longer owns render context if we are spawning threads
	m_pContext->getGPUScreen()->ReleaseRenderContextOwnership(m_pContext->m_gameThreadThreadOwnershipMask);
	
    m_runGame = true;
    
	#if APIABSTRACTION_IOS
		return 0;
	#endif
    
    // create a timer for frame time tracking
    /*Timer *pT = */new(m_hTimer) Timer();
    
    // Game Loop ---------------------------------------------------------------
    	#if PYENGINE_2_0_MULTI_THREADED
	{
		g_drawThreadInitialized = false;
		g_drawThreadCanStart = false;
		g_drawThreadShouldExit = false;
	
		g_drawThreadInitializationLock.lock();// lock the rendering thread initialization lock
		
		g_drawThread.m_function = drawThreadFunctionJob;
		g_drawThread.m_pParams = m_pContext;
		g_drawThread.run(); // thread will wait on rendering thread lock

		
		while (!g_drawThreadInitialized)
		{
			// sleep until draw thread is initialized
			bool success = g_drawThreadInitializedCV.sleep();
			assert(success);
		}

		// draw thread now initialized and will sleep on renderThreadLock
	}
	#endif
    
	while(m_runGame)
    {
        runGameFrame();
    } // while (runGame) -- game loop

	return 0;
}

}; // namespace Components
}; // namespace PE

