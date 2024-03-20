#include "ClientLayer.h"
#include "dependencies/imgui-docking/imgui.h"
#include "HTTPRequest.hpp"

#include <iostream>

void ClientLayer::OnAttach()
{

}

void ClientLayer::OnDetach()
{

}

void ClientLayer::OnUpdate(float dt)
{

    {
        ImGui::Begin("Settings");
        ImGui::AlignTextToFramePadding();

        ImGui::InputInt("Generator count", &m_GeneratorCount);

        ImGui::InputTextWithHint("Request address", "http://httpbin.org/", m_RequestAddress.data(), 1024);


        if (ImGui::Button("Send Request"))
        {
            try
            {
                // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
                http::Request request{ m_RequestAddress };
            
                // send a get request
                const auto response = request.send("GET");
                m_ConsoleText = std::string{ response.body.begin(), response.body.end() };
            }
            catch (const std::exception& e)
            {
                m_ConsoleText = std::string("Request failed, error: ") + e.what();
            }
        }

        ImGui::End();
    }

    {
        ImGui::Begin("Console output");
        ImGui::AlignTextToFramePadding();

        ImGui::TextWrapped(m_ConsoleText.c_str());

        ImGui::End();
    }

}