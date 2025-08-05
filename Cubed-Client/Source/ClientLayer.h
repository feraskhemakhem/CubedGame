#pragma once

#include "Walnut/Application.h"
#include "Walnut/Networking/Client.h"
#include "Walnut/Layer.h"

#include "Renderer/Renderer.h"

#include <glm/glm.hpp>

namespace Cubed 
{

	class ClientLayer : public Walnut::Layer
	{
	public:
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(float ts) override;
		virtual void OnRender() override;
		virtual void OnUIRender() override;
	private:
		void OnDataReceived(const Walnut::Buffer buffer);
	private:
		Renderer m_Renderer;

		glm::vec2 m_PlayerPosition{ 50, 50 };
		glm::vec2 m_PlayerVelocity{ 0, 0 };
		const float speed = 150.0f;
		const float friction = 10.0f;

		std::string m_serverAddress;

		Walnut::Client m_Client;
		uint32_t m_PlayerID;

		struct PlayerData
		{
			glm::vec2 Position;
			glm::vec2 Velocity;
		};

		// lock-safe map
		std::mutex m_PlayerDataMutex;
		std::map<uint32_t, PlayerData> m_PlayerData;
	};
}