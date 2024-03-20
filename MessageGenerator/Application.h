#pragma once
#include "Layer.h"

#include <vector>
#include <memory>

struct GLFWwindow;
class Application
{
public:
	Application();
	~Application();

	void Run();

	void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

private:
	GLFWwindow* m_Window = nullptr;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;

	std::vector<std::shared_ptr<Layer>> m_LayerStack;
};