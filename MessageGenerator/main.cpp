#include "Application.h"
#include "ClientLayer.h"

int main()
{
    Application* app = new Application();
    std::shared_ptr<ClientLayer> clientLayer = std::make_shared<ClientLayer>();
    app->PushLayer(clientLayer);
    app->Run();
    delete app;

	return 0;
}