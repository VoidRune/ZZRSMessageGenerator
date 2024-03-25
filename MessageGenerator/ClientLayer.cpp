#include "ClientLayer.h"
#include "dependencies/imgui-docking/imgui.h"
#include "HTTPRequest.hpp"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <execution>

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
        ImGui::InputTextWithHint("Message body", "body", m_MessageBody.data(), 1024);
        ImGui::Text(m_TimeTaken.c_str());

        if (ImGui::Button("Send Request"))
        {
            //const auto coreCount = std::thread::hardware_concurrency();
            //std::cout << coreCount << std::endl;

            auto start = std::chrono::high_resolution_clock::now();

            std::vector<int> dummy(m_GeneratorCount);
            std::for_each(std::execution::par_unseq, dummy.begin(), dummy.end(), [&](int a)
                {
                    try
                    {
                        // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
                        http::Request request{ m_RequestAddress };

                        const auto response = request.send("POST", m_MessageBody, {
                            {"Content-Type", "application/x-www-form-urlencoded"}
                            });
                        //std::cout << std::string{ response.body.begin(), response.body.end() } << '\n'; // print the result

                        // send a get request
                        //const auto response = request.send("GET");

                        m_ConsoleText = std::string{ response.body.begin(), response.body.end() };
                    }
                    catch (const std::exception& e)
                    {
                        //m_ConsoleText = std::string("Request failed, error: ") + e.what();
                    }
                });

            //for (size_t i = 0; i < m_GeneratorCount; i++)
            //{
            //    try
            //    {
            //        // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
            //        http::Request request{ m_RequestAddress };
            //
            //        //const std::string body = "foo=1&bar=baz";
            //        //const auto response = request.send("POST", body, {
            //        //    {"Content-Type", "application/x-www-form-urlencoded"}
            //        //    });
            //        //std::cout << std::string{ response.body.begin(), response.body.end() } << '\n'; // print the result
            //
            //        // send a get request
            //        const auto response = request.send("GET", "", {}, std::chrono::milliseconds{ 0 });
            //
            //        m_ConsoleText = std::string{ response.body.begin(), response.body.end() };
            //    }
            //    catch (const std::exception& e)
            //    {
            //        //m_ConsoleText = std::string("Request failed, error: ") + e.what();
            //    }
            //}
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            m_TimeTaken = std::to_string(duration.count() * 1e-6);
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