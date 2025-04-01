#pragma once

#include "Walnut/Application.h"
#include "Walnut/Layer.h"

#include "Walnut/Networking/Client.h"

#include <glm/glm.hpp>

namespace Cubed 
{

	class ClientLayer : public Walnut::Layer
	{
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(float ts) override;
		virtual void OnUIRender() override;
	private:
		void OnDataReceived(const Walnut::Buffer buffer);
	private:
		glm::vec2 m_PlayerPosition{ 50, 50 };
		glm::vec2 m_PlayerVelocity{ 0, 0 };
		const float speed = 150.0f;
		const float friction = 10.0f;

		std::string m_serverAddress;

		Walnut::Client m_Client;
	};

}