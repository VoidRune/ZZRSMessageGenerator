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

	int32_t m_ConnectGeneratorCount = 1;
	std::string m_Address = "93.184.216.34";
	int32_t m_Port = 80;
	std::string m_ConsoleText = "";
	bool m_FlushPeriodically = false;
	int32_t m_RequestCount = 1;
	std::string m_MessageBody = "Hello world!";
	std::string m_TimeTaken = "Time taken: 0";
};