#include "TestModels.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Math/CameraOps.h"

namespace PE {
	namespace Components {
		
		PE_IMPLEMENT_CLASS1(TestModel, Component);

		void TestModel::addDefaultComponents()
		{
			Component::addDefaultComponents();

			PE_REGISTER_EVENT_HANDLER(Events::Event_CALCULATE_TRANSFORMATIONS, TestModel::do_CALCULATE_TRANSFORMATIONS);
		}

		void TestModel::do_CALCULATE_TRANSFORMATIONS(Events::Event *pEvt)
		{
			

		}
	};
};