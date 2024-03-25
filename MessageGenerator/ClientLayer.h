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

	int32_t m_GeneratorCount = 1;
	std::string m_RequestAddress = "http://httpbin.org/";
	std::string m_ConsoleText = "";
	std::string m_MessageBody = "Hello world!";
	std::string m_TimeTaken = "";
};