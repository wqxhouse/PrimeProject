// GmaeControls.cpp



// Sibling/Children Includes

#include "DefaultGameControls.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"


// definitions of keyboard and controller events. s.a. Button pressed, etc

#include "../../Events/StandardControllerEvents.h"

#include "../../Events/StandardKeyboardEvents.h"

#if APIABSTRACTION_IOS

	#include "../../Events/StandardIOSEvents.h"

#endif



// definitions of standard game events. the events that any game could potentially use

#include "../../Events/StandardGameEvents.h"

#include "../../Lua/LuaEnvironment.h"



// Arkane Control Values

#define Analog_To_Digital_Trigger_Distance 0.5f

static float Debug_Fly_Speed = 4.0f; //Units per second

#define Debug_Rotate_Speed 2.0f //Radians per second

#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/TestModels.h"

namespace PE {

namespace Components {



using namespace PE::Events;



PE_IMPLEMENT_CLASS1(DefaultGameControls, Component);



void DefaultGameControls::addDefaultComponents()

{

	Component::addDefaultComponents();



	PE_REGISTER_EVENT_HANDLER(Event_UPDATE, DefaultGameControls::do_UPDATE);

}



void DefaultGameControls::do_UPDATE(Events::Event *pEvt)
{

	// Process input events (XBOX360 buttons, triggers...)

	Handle iqh = Events::EventQueueManager::Instance()->getEventQueueHandle("input");

	// Process input event -> game event conversion

	while (!iqh.getObject<Events::EventQueue>()->empty())
	{
		Events::Event *pInputEvt = iqh.getObject<Events::EventQueue>()->getFront();

		m_frameTime = ((Event_UPDATE*)(pEvt))->m_frameTime;

		// Have DefaultGameControls translate the input event to GameEvents

		handleKeyboardDebugInputEvents(pInputEvt);

		handleControllerDebugInputEvents(pInputEvt);

        handleIOSDebugInputEvents(pInputEvt);

		

		iqh.getObject<Events::EventQueue>()->destroyFront();

	}



	// Events are destoryed by destroyFront() but this is called every frame just in case

	iqh.getObject<Events::EventQueue>()->destroy();

}

    

void DefaultGameControls::handleIOSDebugInputEvents(Event *pEvt)

{

	#if APIABSTRACTION_IOS

		m_pQueueManager = Events::EventQueueManager::Instance();

		if (Event_IOS_TOUCH_MOVED::GetClassId() == pEvt->getClassId())

		{

			Event_IOS_TOUCH_MOVED *pRealEvent = (Event_IOS_TOUCH_MOVED *)(pEvt);

            

			if(pRealEvent->touchesCount > 1)

			{

				Handle h("EVENT", sizeof(Event_FLY_CAMERA));

                Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

                

                Vector3 relativeMovement(0.0f,0.0f,1.0f * 30.0 * pRealEvent->m_normalized_dy);

                flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;

                m_pQueueManager->add(h, QT_GENERAL);

                

			}

			else

			{

				Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));

				Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

                

				Vector3 relativeRotate(-pRealEvent->m_normalized_dx * 10, pRealEvent->m_normalized_dy * 10, 0.0f);

				rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;

				m_pQueueManager->add(h, QT_GENERAL);

            

			}

		}

	#endif

}



void DefaultGameControls::handleKeyboardDebugInputEvents(Event *pEvt)
{
	m_pQueueManager = Events::EventQueueManager::Instance();

	if (Event_KEY_A_HELD::GetClassId() == pEvt->getClassId())
	{

		Handle h("EVENT", sizeof(Event_FLY_CAMERA));

		Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

		Vector3 relativeMovement(-1.0f,0.0f,0.0f);

		flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);
	}

	else if (Event_KEY_S_HELD::GetClassId() == pEvt->getClassId())
	{
		Handle h("EVENT", sizeof(Event_FLY_CAMERA));
		Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

		Vector3 relativeMovement(0.0f,0.0f,-1.0f);

		flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);

	}

	else if (Event_KEY_D_HELD::GetClassId() == pEvt->getClassId())

	{

		Handle h("EVENT", sizeof(Event_FLY_CAMERA));

		Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

		Vector3 relativeMovement(1.0f,0.0f,0.0f);
		flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;
		m_pQueueManager->add(h, QT_GENERAL);
	}

	else if (Event_KEY_W_HELD::GetClassId() == pEvt->getClassId())
	{
		Handle h("EVENT", sizeof(Event_FLY_CAMERA));
		Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

		Vector3 relativeMovement(0.0f,0.0f,1.0f);

		flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);
	}
	else if (Event_KEY_LEFT_HELD::GetClassId() == pEvt->getClassId())
	{

		Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));

		Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;
		Vector3 relativeRotate(-1.0f,0.0f,0.0f);
		rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;
		m_pQueueManager->add(h, QT_GENERAL);
	}
	else if (Event_KEY_DOWN_HELD::GetClassId() == pEvt->getClassId())
	{
		Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));
		Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;
		Vector3 relativeRotate(0.0f,-1.0f,0.0f);

		rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);
	}
	else if (Event_KEY_RIGHT_HELD::GetClassId() == pEvt->getClassId())
	{
		Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));

		Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;
		Vector3 relativeRotate(1.0f,0.0f,0.0f);

		rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);

	}

	else if (Event_KEY_UP_HELD::GetClassId() == pEvt->getClassId())
	{

		Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));

		Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

		

		Vector3 relativeRotate(0.0f,1.0f,0.0f);

		rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);

	}
	else if (Event_KEY_K_HELD::GetClassId() == pEvt->getClassId())
	{
		//m_pContext->_renderMode = 0;
		//EffectManager::Instance()->resizeLightNums(10);
		m_pContext->_roughness -= 0.005;
		m_pContext->_roughness = max(m_pContext->_roughness, 0.01f);
		
	}
	else if (Event_KEY_L_HELD::GetClassId() == pEvt->getClassId())
	{
		//m_pContext->_renderMode = 1;
		//EffectManager::Instance()->resizeLightNums(-10);
		m_pContext->_roughness += 0.005;
		m_pContext->_roughness = min(m_pContext->_roughness, 1.0f);
	}

	else if (Event_KEY_COMMA_HELD::GetClassId() == pEvt->getClassId())
	{
		m_pContext->_debugMode--;
		m_pContext->_debugMode = max(m_pContext->_debugMode, 0);
	}
	else if (Event_KEY_PERIOD_HELD::GetClassId() == pEvt->getClassId())
	{
		m_pContext->_debugMode++;
		m_pContext->_debugMode = m_pContext->_debugMode % 5;
	}
	else if (Event_KEY_X_HELD::GetClassId() == pEvt->getClassId())
	{
		float solarTime;
		solarTime = EffectManager::Instance()->getSkybox()->GetSolarTime();
		printf("SolarTime = %.2f\n", solarTime);
		EffectManager::Instance()->getSkybox()->SetSolarTime(solarTime + 0.02);
	}
	else if (Event_KEY_C_HELD::GetClassId() == pEvt->getClassId())
	{
		float solarTime;
		solarTime = EffectManager::Instance()->getSkybox()->GetSolarTime();
		printf("SolarTime = %.2f\n", solarTime);
		EffectManager::Instance()->getSkybox()->SetSolarTime(solarTime - 0.02);
	}
	else if (Event_KEY_V_HELD::GetClassId() == pEvt->getClassId())
	{
		float phi, theta;
		/*	EffectManager::Instance()->getSkybox()->GetSunDirection(phi, theta);
			EffectManager::Instance()->getSkybox()->SetSunDirection(phi, ++theta);*/

		
		m_pContext->_preModel = m_pContext->_curModel;
		m_pContext->_curModel++;
		m_pContext->_curModel = m_pContext->_curModel % RootSceneNode::Instance()->m_testmodels.m_size;

		EffectManager::Instance()->changeModel(m_pContext->_curModel, m_pContext->_preModel);
	}
	else if (Event_KEY_O_HELD::GetClassId() == pEvt->getClassId())
	{
		float phi, theta;
		/*EffectManager::Instance()->getSkybox()->GetSunDirection(phi, theta);
		EffectManager::Instance()->getSkybox()->SetSunDirection(phi, --theta);*/

		m_pContext->_metallic -= 0.005;
		m_pContext->_metallic = max(m_pContext->_metallic, 0.01f);
	}
	else if (Event_KEY_P_HELD::GetClassId() == pEvt->getClassId())
	{
		float phi, theta;
		/*EffectManager::Instance()->getSkybox()->GetSunDirection(phi, theta);
		EffectManager::Instance()->getSkybox()->SetSunDirection(phi, --theta);*/

		m_pContext->_metallic += 0.005;
		m_pContext->_metallic = min(m_pContext->_metallic, 1.0f);
	}

	else if (Event_KEY_T_HELD::GetClassId() == pEvt->getClassId())
	{
		/*m_pContext->_farFocusEnd -= 0.05f;
		m_pContext->_farFocusEnd = max(m_pContext->_farFocusEnd, 0.0f);
		*/

		float farFocusEnd = EffectManager::Instance()->_postProcess.getFarFoucsEnd() - 0.05f;
		EffectManager::Instance()->_postProcess.setFarFocusEnd(farFocusEnd);
	}

	else if (Event_KEY_Y_HELD::GetClassId() == pEvt->getClassId())
	{
		float farFocusEnd = EffectManager::Instance()->_postProcess.getFarFoucsEnd() + 0.05f;
		EffectManager::Instance()->_postProcess.setFarFocusEnd(farFocusEnd);
		//m_pContext->_farFocusEnd += 0.05f;
		//m_pContext->_farFocusEnd = min(m_pContext->_farFocusEnd, 1000.0f);

	}

	else if (Event_KEY_J_HELD::GetClassId() == pEvt->getClassId())
	{
		//m_pContext->_farFocusStart -= 0.05f;
		//m_pContext->_farFocusStart = max(m_pContext->_farFocusStart, 0.0f);

		float farFocusStart = EffectManager::Instance()->_postProcess.getFarFocusStart() - 0.05f;
		EffectManager::Instance()->_postProcess.setFarFocusStart(farFocusStart);
	}

	else if (Event_KEY_H_HELD::GetClassId() == pEvt->getClassId())
	{
		//float phi, theta;
		//m_pContext->_farFocusStart += 0.05f;
		//m_pContext->_farFocusStart = min(m_pContext->_farFocusStart, 1000.0f);ASSERT()

		float farFocusStart = EffectManager::Instance()->_postProcess.getFarFocusStart() + 0.05f;
		EffectManager::Instance()->_postProcess.setFarFocusStart(farFocusStart);
	}
	else if (Event_KEY_B_HELD::GetClassId() == pEvt->getClassId())
	{
		EffectManager *ef = EffectManager::Instance();
		ef->getEnableIndirectLighting() ? ef->setEnableIndirectLighting(false) : ef->setEnableIndirectLighting(true);
	}
	else if (Event_KEY_N_HELD::GetClassId() == pEvt->getClassId())
	{
		EffectManager *ef = EffectManager::Instance();
		ef->getEnableLocalCubemap() ? ef->setEnableLocalCubemap(false) : ef->setEnableLocalCubemap(true);
	}
	else if (Event_KEY_M_HELD::GetClassId() == pEvt->getClassId())
	{
		PostProcess *p = &EffectManager::Instance()->_postProcess;
		p->getEnableColorCorrection() ? p->SetEnableColorCorrection(false) : p->SetEnableColorCorrection(true);

	}
	else if (Event_KEY_Z_HELD::GetClassId() == pEvt->getClassId())
	{
		m_pContext->_curCubeMap++;
		m_pContext->_curCubeMap = m_pContext->_curCubeMap % 7;
		if (m_pContext->_curCubeMap == 6)
		{
			EffectManager::Instance()->getSkybox()->SetSky();
		}
		else
		{
			EffectManager::Instance()->getSkybox()->SetCubemap(m_pContext->_cubmapID[m_pContext->_curCubeMap]);
		}
		
	}
	else if (Event_KEY_LEFT_BRACKET_HELD::GetClassId() == pEvt->getClassId())
	{
		float manualExp = EffectManager::Instance()->_postProcess.getManualExposure() - 0.1;
		printf("%.2f\n", manualExp);
		EffectManager::Instance()->_postProcess.setManualExposure(manualExp);
	}
	else if (Event_KEY_RIGHT_BRACKET_HELD::GetClassId() == pEvt->getClassId())
	{
		float manualExp = EffectManager::Instance()->_postProcess.getManualExposure() + 0.1;
		printf("%.2f\n", manualExp);
		EffectManager::Instance()->_postProcess.setManualExposure(manualExp);
	}
	else if (Event_KEY_ZERO_HELD::GetClassId() == pEvt->getClassId())
	{
		EffectManager::Instance()->_postProcess.getEnableManualExposure() ?
			EffectManager::Instance()->_postProcess.setEnableManualExposure(false) :
			EffectManager::Instance()->_postProcess.setEnableManualExposure(true);
	}
	else if (Event_KEY_MINUS_HELD::GetClassId() == pEvt->getClassId())
	{
		float keyVal = EffectManager::Instance()->_postProcess.getKeyValue() + 0.005;
		printf("Keyval: %.2f\n", keyVal);
		EffectManager::Instance()->_postProcess.setKeyValue(keyVal);
	}
	else if (Event_KEY_EQUAL_HELD::GetClassId() == pEvt->getClassId())
	{
		float keyVal = EffectManager::Instance()->_postProcess.getKeyValue() - 0.005;
		printf("Keyval: %.2f\n", keyVal);
		EffectManager::Instance()->_postProcess.setKeyValue(keyVal);
	}
	else if (Event_KEY_R_HELD::GetClassId() == pEvt->getClassId())
	{
		EffectManager::Instance()->_postProcess.getEnableDOF() ?
			EffectManager::Instance()->_postProcess.setEnableDOF(false) :
			EffectManager::Instance()->_postProcess.setEnableDOF(true);
	}
	else if (Event_KEY_NUM_0::GetClassId() == pEvt->getClassId())
	{


		m_pContext->isSSR ? m_pContext->isSSR = false : m_pContext->isSSR = true;
	}
	else if (Event_KEY_NUM_1::GetClassId() == pEvt->getClassId())
	{
		float intensity = EffectManager::Instance()->_normalIntensity;
		intensity += 0.03;
		intensity = min(intensity, 1.0f);
		EffectManager::Instance()->_normalIntensity = intensity;
	}
	else if (Event_KEY_NUM_2::GetClassId() == pEvt->getClassId())
	{
		float intensity = EffectManager::Instance()->_normalIntensity;
		intensity -= 0.03;
		intensity = max(intensity, 0.0f);
		EffectManager::Instance()->_normalIntensity = intensity;
	}
	else if (Event_KEY_NUM_3::GetClassId() == pEvt->getClassId())
	{
		m_pContext->isSmallBalls ? m_pContext->isSmallBalls = false : m_pContext->isSmallBalls = true;
	}
	else
	{
		Component::handleEvent(pEvt);
	}
}


void DefaultGameControls::handleControllerDebugInputEvents(Event *pEvt)

{

	if (Event_ANALOG_L_THUMB_MOVE::GetClassId() == pEvt->getClassId())

	{

		Event_ANALOG_L_THUMB_MOVE *pRealEvent = (Event_ANALOG_L_THUMB_MOVE*)(pEvt);

		

		Handle h("EVENT", sizeof(Event_FLY_CAMERA));

		Event_FLY_CAMERA *flyCameraEvt = new(h) Event_FLY_CAMERA ;

		

		Vector3 relativeMovement(pRealEvent->m_absPosition.getX(), 0.0f, pRealEvent->m_absPosition.getY());//Flip Y and Z axis

		flyCameraEvt->m_relativeMove = relativeMovement * Debug_Fly_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);

	}

	else if (Event_ANALOG_R_THUMB_MOVE::GetClassId() == pEvt->getClassId())

	{

		Event_ANALOG_R_THUMB_MOVE *pRealEvent = (Event_ANALOG_R_THUMB_MOVE *)(pEvt);

		

		Handle h("EVENT", sizeof(Event_ROTATE_CAMERA));

		Event_ROTATE_CAMERA *rotateCameraEvt = new(h) Event_ROTATE_CAMERA ;

		

		Vector3 relativeRotate(pRealEvent->m_absPosition.getX(), pRealEvent->m_absPosition.getY(), 0.0f);

		rotateCameraEvt->m_relativeRotate = relativeRotate * Debug_Rotate_Speed * m_frameTime;

		m_pQueueManager->add(h, QT_GENERAL);

	}

	else if (Event_PAD_N_DOWN::GetClassId() == pEvt->getClassId())

	{

	}

	else if (Event_PAD_N_HELD::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_N_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_S_DOWN::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_S_HELD::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_S_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_W_DOWN::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_W_HELD::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_W_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_E_DOWN::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_E_HELD::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_PAD_E_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_A_HELD::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_Y_DOWN::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_A_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_B_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_X_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_Y_UP::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_ANALOG_L_TRIGGER_MOVE::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_ANALOG_R_TRIGGER_MOVE::GetClassId() == pEvt->getClassId())

	{

		

	}

	else if (Event_BUTTON_L_SHOULDER_DOWN::GetClassId() == pEvt->getClassId())

	{

		

	}

	else

	{

		Component::handleEvent(pEvt);

	}

}



}; // namespace Components

}; // namespace PE

