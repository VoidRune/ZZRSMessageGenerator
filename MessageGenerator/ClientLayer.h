#pragma once
#include "Layer.h"
#include <cstdint>
#include <string>

class ClientLayer : public Layer
{
public:
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(float dt) override;
private:

	void SendData(int amountPerGenerator);
	void UpdateIds(int idsPerGenerator);

	int32_t m_ConnectGeneratorCount = 1;
	std::string m_Address = "127.0.0.1";
	int32_t m_Port = 8080;
	int32_t m_GeneratorStartId = 0;
	bool m_TryReconnect = false;
	int32_t m_LastGeneratorCount = 0;
	float m_ReconnectTime = 10.0f;
	float m_CurrentReconnectTimer = 0.0f;

	int32_t m_MinIdsPerGenerator = 1;
	int32_t m_MaxIdsPerGenerator = 1;
	float m_TimeTakenToMaxId = 1.0f;
	float m_CurrentTimeTaken = 0.0f;
	bool m_IsTimerRunning = false;
	int32_t m_CurrentIdsPerGenerator = m_MinIdsPerGenerator;

	std::string m_ConsoleText = "";
	bool m_FlushPeriodically = true;
	int32_t m_RequestCount = 1;
	bool m_SendContinuously = false;
	float m_AccumulatedRequestNumber = 0.0f;
	std::string m_TimeTaken = "Time taken: 0";
};