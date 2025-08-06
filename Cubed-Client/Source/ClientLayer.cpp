#include "ClientLayer.h"

// for user inputs
#include "Walnut/Input/Input.h"
#include "Walnut/ImGui/ImGuiTheme.h"

// temp IMGUI functionality for MVP
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "glm/gtc/type_ptr.hpp"

// for processing data to and from the server
#include "Walnut/Serialization/BufferStream.h"

// cubed-common include
#include "ServerPacket.h"

using namespace Walnut;

namespace Cubed
{
	static Walnut::Buffer s_ScratchBuffer;

	// draw a simple rectangle
	static void DrawRect(glm::vec2 position, glm::vec2 size, uint32_t color)
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		ImVec2 min = ImGui::GetWindowPos() + ImVec2(position.x, position.y);
		ImVec2 max = min + ImVec2(size.x, size.y);

		drawList->AddRectFilled(min, max, color);
	}

	void ClientLayer::OnAttach()
	{
		s_ScratchBuffer.Allocate((uint64_t)(10 * 1024) * 1024); // 10MB of scratch buffer

		// set callback function to local private function
		m_Client.SetDataReceivedCallback([this](const Walnut::Buffer buffer) { OnDataReceived(buffer); });

		m_Renderer.Init();
	}


	void ClientLayer::OnDetach() 
	{
		s_ScratchBuffer.Release();
	}

	void ClientLayer::OnUpdate(float ts)
	{
		// if not connected, ignore movement
	/*	if (m_Client.GetConnectionStatus() != Client::ConnectionStatus::Connected)
			return;*/

		// read wasd inputs on update  to move square
		glm::vec2 dir{ 0.0f, 0.0f };
		if (Input::IsKeyDown(KeyCode::W))
		{
			dir.y = -1;
		}
		else if (Input::IsKeyDown(KeyCode::S))
		{
			dir.y = 1;
		}


		if (Input::IsKeyDown(KeyCode::A))
		{
			dir.x = -1;
		}
		else if (Input::IsKeyDown(KeyCode::D))
		{
			dir.x = 1;
		}

		if (glm::length(dir) > 0.0f)
		{
			// avoid longer diagonals
			dir = glm::normalize(dir);
			
			m_PlayerVelocity = dir * speed;
		}

		m_PlayerPosition += m_PlayerVelocity * ts;

		m_PlayerVelocity = glm::mix(m_PlayerVelocity, glm::vec2(0.0f), friction * ts); // lerp decay at a rate of 10f

		m_PlayerRotation.y += ts * 20.0f;

		if (m_Client.GetConnectionStatus() == Client::ConnectionStatus::Connected)
		{

			// send player data to server
			BufferStreamWriter stream(s_ScratchBuffer);
			stream.WriteRaw(PacketType::ClientUpdate);
			stream.WriteRaw<glm::vec2>(m_PlayerPosition);
			stream.WriteRaw<glm::vec2>(m_PlayerVelocity);
			m_Client.SendBuffer(stream.GetBuffer());
		}
	}

	void ClientLayer::OnRender()
	{
		// 1. bind pipeline
		// 2. bind vertex/index buffers
		// 3. draw call

		m_Renderer.BeginScene(m_Camera);

		//Client::ConnectionStatus connectionStatus = m_Client.GetConnectionStatus();
		//if (connectionStatus == Client::ConnectionStatus::Connected)
		{
			// draw self
			m_Renderer.RenderCube(glm::vec3(m_PlayerPosition.x, 0.5f, m_PlayerPosition.y), m_PlayerRotation);

			// read other players' data
			m_PlayerDataMutex.lock();
			std::map<uint32_t, PlayerData> playerData = m_PlayerData;
			m_PlayerDataMutex.unlock();

			// process data freely outside of lock
			for (const auto& [id, data] : playerData)
			{
				// skip ourselves
				if (id == m_PlayerID)
					continue;

				// draw other players (do not have rotation data so set to 0)
				m_Renderer.RenderCube(glm::vec3(data.Position.x, 0.5f, data.Position.y), {0.0f, 0.0f, 0.0f});
			}
		}

		m_Renderer.EndScene(m_Camera);
	}

	void ClientLayer::OnUIRender()
	{
		Client::ConnectionStatus connectionStatus = m_Client.GetConnectionStatus();
		if (connectionStatus == Client::ConnectionStatus::Connected)
		{
			if (false)
			{

				// draw self
				DrawRect(m_PlayerPosition, { 50.0f, 50.0f }, 0xffff00ff);

				// read other players' data
				m_PlayerDataMutex.lock();
				std::map<uint32_t, PlayerData> playerData = m_PlayerData;
				m_PlayerDataMutex.unlock();

				// process data freely outside of lock
				for (const auto& [id, data] : playerData)
				{
					// skip ourselves
					if (id == m_PlayerID)
						continue;

					// draw other players
					DrawRect(data.Position, { 50.0f, 50.0f }, 0xff00ff00);
				}
			}
		}
		else
		{
			// try to connect
			ImGui::Begin("Connect to Server");

			ImGui::InputText("Server address", &m_serverAddress);
			if (connectionStatus == Client::ConnectionStatus::FailedToConnect)
				ImGui::TextColored(ImColor(UI::Colors::Theme::error), "Failed to connect.");
			else if (connectionStatus == Client::ConnectionStatus::Connecting)
				ImGui::TextColored(ImColor(UI::Colors::Theme::textDarker), "Connecting...");

			if (ImGui::Button("Connect"))
			{
				m_Client.ConnectToServer(m_serverAddress);
			}

			ImGui::End();
		}

		m_Renderer.RenderUI();

		ImGui::Begin("Controls");
		ImGui::DragFloat3("Player Position", glm::value_ptr(m_PlayerPosition), 0.05f);
		ImGui::DragFloat3("Player Rotation", glm::value_ptr(m_PlayerRotation), 0.05f);

		ImGui::DragFloat3("Camera Position", glm::value_ptr(m_Camera.Position), 0.05f);
		ImGui::DragFloat3("Camera Rotation", glm::value_ptr(m_Camera.Rotation), 0.05f);
		ImGui::End();
	}

	void ClientLayer::OnDataReceived(const Walnut::Buffer buffer)
	{
		BufferStreamReader stream(buffer);

		PacketType type;
		stream.ReadRaw(type);
		switch (type)
		{
		case PacketType::ClientConnect:

			stream.ReadRaw<uint32_t>(m_PlayerID);
			//WL_INFO("We have connected! Server says our ID is {}", idFromServer);
			//WL_INFO("We say our ID is {}", m_Client.GetID());
			break;
		case PacketType::ClientUpdate:
			// list of other clients
			m_PlayerDataMutex.lock();
			stream.ReadMap(m_PlayerData);
			m_PlayerDataMutex.unlock();
			break;
			
		}
	}

}