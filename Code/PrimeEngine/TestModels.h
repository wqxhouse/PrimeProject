#ifndef __PYENGINE_2_0_TESTMODEL_H__
#define __PYENGINE_2_0_TESTMODEL_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Utils/Array/Array.h"
#include "PrimeEngine/Math/Vector4.h"
#include "PrimeEngine/Math/Vector3.h"
#include "PrimeEngine/Scene/SceneNode.h"
namespace PE {
	namespace Components 
	{
		
		struct TestModel : public Component
		{
			PE_DECLARE_CLASS(TestModel);
			// Constructor -------------------------------------------------------------
			TestModel(
				PE::GameContext &context,
				PE::MemoryArena arena,
				Handle hMyself,
				SceneNode *scenenode,
				float roughness
				) : Component(context, arena, hMyself)
			{
				m_sceneNode = scenenode;
				m_roughness = roughness;
			}
			
			virtual ~TestModel(){}

			// Component ------------------------------------------------------------

			virtual void addDefaultComponents();

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_CALCULATE_TRANSFORMATIONS);
			virtual void do_CALCULATE_TRANSFORMATIONS(Events::Event *pEvt);

			SceneNode * m_sceneNode;
			float m_roughness;
		};
			
	};
};


#endif