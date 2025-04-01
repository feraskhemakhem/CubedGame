#include "ClientLayer.h"

// for user inputs
#include "Walnut/Input/Input.h"
#include "Walnut/ImGui/ImGuiTheme.h"

// temp IMGUI functionality for MVP
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

namespace Cubed
{
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
		// set callback function to local private function
		m_Client.SetDataReceivedCallback([this](const Walnut::Buffer buffer) { OnDataReceived(buffer); });
	}


	void ClientLayer::OnDetach() 
	{

	}

	void ClientLayer::OnUpdate(float ts)
	{
		// if not connected, ignore movement
		if (m_Client.GetConnectionStatus() != Walnut::Client::ConnectionStatus::Connected)
			return;

		// read wasd inputs on update  to move square
		glm::vec2 dir{ 0.0f, 0.0f };
		if (Walnut::Input::IsKeyDown(Walnut::KeyCode::W))
		{
			dir.y = -1;
		}
		else if (Walnut::Input::IsKeyDown(Walnut::KeyCode::S))
		{
			dir.y = 1;
		}


		if (Walnut::Input::IsKeyDown(Walnut::KeyCode::A))
		{
			dir.x = -1;
		}
		else if (Walnut::Input::IsKeyDown(Walnut::KeyCode::D))
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
	}

	void ClientLayer::OnUIRender()
	{
		Walnut::Client::ConnectionStatus connectionStatus = m_Client.GetConnectionStatus();
		if (connectionStatus == Walnut::Client::ConnectionStatus::Connected)
		{
			// play game
			DrawRect(m_PlayerPosition, { 50.0f, 50.0f }, 0xffff00ff);
		}
		else
		{
			// try to connect
			ImGui::Begin("Connect to Server");

			ImGui::InputText("Server address", &m_serverAddress);
			if (connectionStatus == Walnut::Client::ConnectionStatus::FailedToConnect)
				ImGui::TextColored(ImColor(Walnut::UI::Colors::Theme::error), "Failed to connect.");
			else if (connectionStatus == Walnut::Client::ConnectionStatus::Connecting)
				ImGui::TextColored(ImColor(Walnut::UI::Colors::Theme::textDarker), "Connecting...");

			if (ImGui::Button("Connect"))
			{ 
				m_Client.ConnectToServer(m_serverAddress);
			}

			ImGui::End();
		}

		// display window
		ImGui::ShowDemoWindow();
	}

	void ClientLayer::OnDataReceived(const Walnut::Buffer buffer)
	{

	}

}