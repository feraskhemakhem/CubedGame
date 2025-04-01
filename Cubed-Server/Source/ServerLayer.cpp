#include "ServerLayer.h"

#include <chrono>

#include "Walnut/Core/Log.h"
#include "Walnut/Serialization/BufferStream.h"

#include "ServerPacket.h"

using namespace Walnut;

namespace Cubed
{
	static Buffer s_ScratchBuffer;

	// Layer overrides

	void ServerLayer::OnAttach()
	{
		s_ScratchBuffer.Allocate(10 * 1024 * 1024); // 10MB of scratch buffer

		m_Console.SetMessageSendCallback([this](std::string_view message) { OnConsoleMessage(message); });
	
		m_Server.SetClientConnectedCallback([this](const ClientInfo& clientInfo) { OnClientConnected(clientInfo); });
		m_Server.SetClientDisconnectedCallback([this](const ClientInfo& clientInfo) { OnClientDisconnected(clientInfo); });
		m_Server.SetDataReceivedCallback([this](const ClientInfo& clientInfo, const Buffer buffer) { OnDataReceived(clientInfo, buffer); });

		m_Server.Start();
	}

	void ServerLayer::OnDetach()
	{
		s_ScratchBuffer.Release();
		m_Server.Stop();
	}

	void ServerLayer::OnUpdate(float ts)
	{
		BufferStreamWriter stream(s_ScratchBuffer);
		stream.WriteRaw(PacketType::ClientUpdate);
		m_PlayerDataMutex.lock();
		{
			stream.WriteMap(m_PlayerData);
		}
		m_PlayerDataMutex.unlock();

		m_Server.SendBufferToAllClients(stream.GetBuffer());

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(5ms);
	}

	void ServerLayer::OnUIRender()
	{

	}

	// console callbacks

	void ServerLayer::OnConsoleMessage(std::string_view message)
	{
		if (message.starts_with('/'))
		{
			std::cout << "You called the " << message << " command!" << std::endl;
		}
	}

	// server callbacks

	void ServerLayer::OnClientConnected(const ClientInfo& clientInfo)
	{
		WL_INFO_TAG("Server", "Client connected! ID = {}", clientInfo.ID);

		//send back client ID to client so they can identify themselves
		BufferStreamWriter stream(s_ScratchBuffer);

		stream.WriteRaw(PacketType::ClientConnect);
		stream.WriteRaw(clientInfo.ID);

		m_Server.SendBufferToClient(clientInfo.ID, stream.GetBuffer());
	}

	void ServerLayer::OnClientDisconnected(const ClientInfo& clientInfo)
	{
		WL_INFO_TAG("Server", "Client disconnected! ID = {}", clientInfo.ID);
	}

	void ServerLayer::OnDataReceived(const ClientInfo& clientInfo, const Buffer buffer)
	{
		BufferStreamReader stream(buffer);

		PacketType type;
		stream.ReadRaw(type);
		switch (type)
		{
		case PacketType::ClientUpdate:

			m_PlayerDataMutex.lock();
			{
				PlayerData& playerData = m_PlayerData[clientInfo.ID];
				stream.ReadRaw<glm::vec2>(playerData.Position);
				stream.ReadRaw<glm::vec2>(playerData.Velocity);
			}
			m_PlayerDataMutex.unlock();

			break;
		}
	}
}