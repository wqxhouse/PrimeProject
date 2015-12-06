// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#if APIABSTRACTION_D3D9 | APIABSTRACTION_D3D11 | (APIABSTRACTION_OGL && !defined(SN_TARGET_PS3))

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/Events/StandardKeyboardEvents.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/Render/D3D9Renderer.h"
#include "../../../Lua/LuaEnvironment.h"
// Sibling/Children includes
#include "DX9_KeyboardMouse.h"
#include "PrimeEngine/Application/WinApplication.h"

namespace PE {
using namespace Events;
namespace Components {

PE_IMPLEMENT_CLASS1(DX9_KeyboardMouse, Component);

void DX9_KeyboardMouse::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, DX9_KeyboardMouse::do_UPDATE);
}

void DX9_KeyboardMouse::do_UPDATE(Events::Event *pEvt)
{
	
	m_pQueueManager = Events::EventQueueManager::Instance();

	generateButtonEvents();
}

void DX9_KeyboardMouse::generateButtonEvents()
{
#ifndef _XBOX
#if PE_PLAT_IS_WIN32
	WinApplication *pWinApp = static_cast<WinApplication*>(m_pContext->getApplication());
	if(GetFocus() == pWinApp->getWindowHandle())
#endif
	{
		static bool udown = false;
		static bool idown = false;
		static bool kdown = false;
		static bool ldown = false;

		static bool xdown = false;
		static bool cdown = false;
		static bool vdown = false;
		static bool bdown = false;
		static bool ndown = false;
		static bool mdown = false;

		//Check for Button Down events

		//Check for Button Up events
		
		//Check for Button Held events
		if(GetAsyncKeyState('A') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_A_HELD));
			new (h) Event_KEY_A_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('S') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_S_HELD));
			new (h) Event_KEY_S_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('D') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_D_HELD));
			new (h) Event_KEY_D_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('W') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_W_HELD));
			new (h) Event_KEY_W_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_LEFT_HELD));
			new (h) Event_KEY_LEFT_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_DOWN) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_DOWN_HELD));
			new (h) Event_KEY_DOWN_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_RIGHT_HELD));
			new (h) Event_KEY_RIGHT_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState(VK_UP) & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_UP_HELD));
			new (h) Event_KEY_UP_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		if(GetAsyncKeyState('U') & 0x8000)
		{
			udown = true;
		}
		else if (udown)
		{
			Handle h("EVENT", sizeof(Event_KEY_COMMA_HELD));
			new (h)Event_KEY_COMMA_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
			udown = false;
		}

		if(GetAsyncKeyState('I') & 0x8000)
		{
			idown = true;
		}
		else if (idown == true)
		{
			Handle h("EVENT", sizeof(Event_KEY_PERIOD_HELD));
			new (h)Event_KEY_PERIOD_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			idown = false;
		}

		if(GetAsyncKeyState('K') & 0x8000)
		{
			kdown = true;
		}
		else if (kdown)
		{
			Handle h("EVENT", sizeof(Event_KEY_K_HELD));
			new (h)Event_KEY_K_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			kdown = false;
		}

		if(GetAsyncKeyState('L') & 0x8000)
		{
			ldown = true;
		}
		else if (ldown)
		{
			Handle h("EVENT", sizeof(Event_KEY_L_HELD));
			new (h) Event_KEY_L_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			ldown = false;
		}

		if (GetAsyncKeyState('X') & 0x8000)
		{
			// xdown = true;
			Handle h("EVENT", sizeof(Event_KEY_X_HELD));
			new (h)Event_KEY_X_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
	/*	else if (xdown)
		{
			Handle h("EVENT", sizeof(Event_KEY_X_HELD));
			new (h)Event_KEY_X_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			xdown = false;
		}*/

		if (GetAsyncKeyState('C') & 0x8000)
		{
			Handle h("EVENT", sizeof(Event_KEY_C_HELD));
			new (h)Event_KEY_C_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
			// cdown = true;
		}
		/*else if (cdown)
		{
			Handle h("EVENT", sizeof(Event_KEY_C_HELD));
			new (h)Event_KEY_C_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			cdown = false;
		}*/

		if (GetAsyncKeyState('V') & 0x8000)
		{
			// vdown = true;

			Handle h("EVENT", sizeof(Event_KEY_V_HELD));
			new (h)Event_KEY_V_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

		}
		//else if (vdown)
		//{
		//	Handle h("EVENT", sizeof(Event_KEY_V_HELD));
		//	new (h)Event_KEY_V_HELD;
		//	m_pQueueManager->add(h, Events::QT_INPUT);

		//	vdown = false;
		//}

		if (GetAsyncKeyState('B') & 0x8000)
		{
			// bdown = true;

			Handle h("EVENT", sizeof(Event_KEY_B_HELD));
			new (h)Event_KEY_B_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);
		}
		//else if (bdown)
		//{
		//	Handle h("EVENT", sizeof(Event_KEY_B_HELD));
		//	new (h)Event_KEY_B_HELD;
		//	m_pQueueManager->add(h, Events::QT_INPUT);

		//	bdown = false;
		//}

		if (GetAsyncKeyState('N') & 0x8000)
		{
			ndown = true;
		}
		else if (ndown)
		{
			Handle h("EVENT", sizeof(Event_KEY_N_HELD));
			new (h)Event_KEY_N_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			ndown = false;
		}

		if (GetAsyncKeyState('M') & 0x8000)
		{
			mdown = true;
		}
		else if (mdown)
		{
			Handle h("EVENT", sizeof(Event_KEY_M_HELD));
			new (h)Event_KEY_M_HELD;
			m_pQueueManager->add(h, Events::QT_INPUT);

			mdown = false;
		}
	}
#endif
}

}; // namespace Components
}; // namespace PE

#endif // API Abstraction
