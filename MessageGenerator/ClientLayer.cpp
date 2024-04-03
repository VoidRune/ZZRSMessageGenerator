#include "ClientLayer.h"
#include "Random.h"
#include "dependencies/imgui-docking/imgui.h"

#define WIN32
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include "asio-1.20.0/include/asio.hpp"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <execution>

asio::error_code g_Ec;
asio::io_context g_Context;
std::thread g_ContextThread;

std::vector<asio::ip::tcp::socket> g_Sockets;
std::vector<char> g_FlushBuffer(20 * 1024);

class Generator
{
public:
    Generator(uint32_t id) :
        Id(id),
        Socket(g_Context)
    {

    }

    //~Generator()
    //{
    //    Socket.close();
    //}

    bool Connect(std::string addr, int32_t port)
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(addr, g_Ec), port);
        Socket.connect(endpoint, g_Ec);
        if (!g_Ec)
        {
            std::cout << "Connected!" << std::endl;
            return true;
        }
        std::cout << "Failed to connect!" << std::endl;
        return false;
    }

    void Flush()
    {
        size_t bytes = Socket.available();
        while (bytes > 0)
        {
            //std::cout << bytes << std::endl;

            Socket.read_some(asio::buffer(g_FlushBuffer.data(), g_FlushBuffer.size()), g_Ec);
            bytes = Socket.available();
            //if (!g_Ec)
            //{
            //    bytes = Socket.available();
            //}
            //else
            //{
            //    std::cout << g_Ec.message() << std::endl;
            //}
        }

        //Socket.async_read_some(asio::buffer(g_FlushBuffer.data(), g_FlushBuffer.size()), 
        //    [&](std::error_code ec, std::size_t len) {
        //        if (!ec)
        //        {
        //            Flush();
        //        }
        //    });
    }

    uint32_t Id;
    asio::ip::tcp::socket Socket;
};

std::vector<Generator> g_Generators;

void ClientLayer::OnAttach()
{
    m_CustomRequestBody =
        "POST /report HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "\r\n"
        "id=1&timestamp=2&value=3";

    m_CustomRequestBody =
        "POST /report HTTP/1.1\r\n"
        "User - Agent: PostmanRuntime / 7.37.0\r\n"
        "Accept : */*\r\n"
        "Cache-Control: no-cache\r\n"
        "Postman-Token: 2c470340-c2a2-4794-b11c-a93ec979bf88\r\n"
        "Host: localhost:8080\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "id=1&timestep=5&value=12";

    g_ContextThread = std::thread([&]() { g_Context.run(); });
}

void ClientLayer::OnDetach()
{
    g_Context.stop();
    g_ContextThread.join();
}

void ClientLayer::OnUpdate(float dt)
{

    if (m_FlushPeriodically)
    {
        for (auto& g : g_Generators)
        {
            g.Flush();
        }
    }

    {
        ImGui::Begin("Generator settings");
        ImGui::AlignTextToFramePadding();


        ImGui::InputTextWithHint("Request address", "http://httpbin.org/", m_Address.data(), 1024);
        ImGui::InputInt("Port", &m_Port);
        //ImGui::InputTextWithHint("Message body", "body", m_MessageBody.data(), 1024);
        //ImGui::Text(m_TimeTaken.c_str());

        auto temp = std::string("Generators open: ") + std::to_string(g_Generators.size());
        ImGui::Text(temp.c_str());

        ImGui::InputInt("Generator count", &m_ConnectGeneratorCount);
        if (ImGui::Button("Connect generator"))
        {
            for (size_t i = 0; i < m_ConnectGeneratorCount; i++)
            {
                Generator gen(g_Generators.size());
                if(gen.Connect(m_Address, m_Port))
                    g_Generators.push_back(std::move(gen));
                else
                {
                    m_ConsoleText = "Failed to connect!";
                    break;
                }
            }

            m_ConsoleText = std::string("Successfully connected ") + std::to_string(m_ConnectGeneratorCount) + " generators!";
        }

        if (ImGui::BeginListBox("Generators"))
        {
            //ImGui::PushItemWidth(84);
            for (size_t i = 0; i < g_Generators.size(); i++)
            {
                std::string s = "Generator" + std::to_string(i);
                std::string c = "Print response" + std::to_string(i);
                ImGui::Text(s.c_str());
                ImGui::SameLine();

                if (ImGui::Button(c.c_str()))
                {
                    auto& g = g_Generators[i];
                    //g.Socket.wait(g.Socket.wait_read);
                    size_t bytes = g.Socket.available();
                    if (bytes > 0)
                    {
                        m_ConsoleText.clear();
                        m_ConsoleText.resize(bytes);
                        g.Socket.read_some(asio::buffer(m_ConsoleText.data(), m_ConsoleText.size()), g_Ec);
                    }
                    else
                    {
                        m_ConsoleText = "There is 0 data to read :(";
                    }
                }
            }
            //ImGui::PopItemWidth();
            ImGui::EndListBox();
        }

        if (ImGui::Button("Flush generators"))
        {
            for (auto& g : g_Generators)
            {
                g.Flush();
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Flush every frame", &m_FlushPeriodically);

        if (ImGui::Button("Clear generators"))
        {
            g_Generators.clear();
        }

        ImGui::End();
    }

    {
        ImGui::Begin("Request settings");
        ImGui::AlignTextToFramePadding();

        ImGui::InputInt("Request count", &m_RequestCount);

        if (ImGui::Button("Send Request"))
        {
            //const auto coreCount = std::thread::hardware_concurrency();
            //std::cout << coreCount << std::endl;

            auto start = std::chrono::high_resolution_clock::now();

            //std::string request =
            //    "POST /index.html HTTP/1.1\r\n"
            //    "Host: example.com\r\n"
            //    "Content-Type: application/x-www-form-urlencoded\r\n"
            //    "Content-Length: 23\r\n"
            //    "\r\n"
            //    "id=1&timestep=2&value=3\r\n\r\n";


            std::string requestTemplate =
                "POST /report HTTP/1.1\r\n"
                "Host: localhost:12345\r\n"
                "Connection: keep-alive\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: ";

            //std::string requestTemplate =
            //    "POST /report HTTP/1.1\r\n"
            //    "Host: example.com\r\n"
            //    "Content-Type: application/x-www-form-urlencoded\r\n"
            //    "\r\n";

            for (auto& g : g_Generators)
            {
                for (size_t i = 0; i < m_RequestCount; i++)
                {
                    std::string dataString = "id=" + std::to_string(g.Id) + "&timestamp=" + std::to_string(i) + "&value=" + std::to_string(Random::Lehmer32());
                    std::string send = requestTemplate + std::to_string(dataString.size()) + "\r\n\r\n" + dataString;
                    //std::string send = requestTemplate + dataString;

                    if (m_UseCustomRequestBody)
                        asio::write(g.Socket, asio::buffer(m_CustomRequestBody.data(), m_CustomRequestBody.size()), g_Ec);
                    else
                        asio::write(g.Socket, asio::buffer(send.data(), send.size()), g_Ec);
                    //g.Socket.write_some(asio::buffer(send.data(), send.size()), g_Ec);

                    if (g_Ec)
                    {
                        std::cout << g_Ec.message() << std::endl;
                    }
                }
                //g.Flush();

                if (g_Ec)
                {
                    m_ConsoleText = g_Ec.message();
                }
            }

            //for (auto& g : g_Generators)
            //{
            //    g.Socket.wait(g.Socket.wait_read);
            //    size_t bytes = g.Socket.available();
            //    if (bytes > 0)
            //    {
            //        //std::vector<char> vBuffer(bytes);
            //        m_ConsoleText.clear();
            //        m_ConsoleText.resize(bytes);
            //        g.Socket.read_some(asio::buffer(m_ConsoleText.data(), m_ConsoleText.size()), g_Ec);
            //
            //        //for (auto c : vBuffer)
            //        //    std::cout << c;
            //    }
            //
            //    break;
            //}

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            m_TimeTaken = "Time taken: " + std::to_string(duration.count() * 1e-6);
        }

        ImGui::Text(m_TimeTaken.c_str());
        ImGui::Checkbox("Send continuously over one second", &m_SendContinuously);

        ImGui::End();
    }

    if (m_SendContinuously)
    {
        std::string requestTemplate = 
            "POST /report HTTP/1.1\r\n"
            "User - Agent: PostmanRuntime / 7.37.0\r\n"
            "Accept : */*\r\n"
            "Cache-Control: no-cache\r\n"
            "Postman-Token: 2c470340-c2a2-4794-b11c-a93ec979bf88\r\n"
            "Host: localhost:8080\r\n"
            "Accept-Encoding: gzip, deflate, br\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: 24\r\n"
            "\r\n"
            "id=1&timestep=5&value=12";
        

        m_AccumulatedRequestNumber += m_RequestCount * dt;
        uint32_t overTimeNumber = std::floor(m_AccumulatedRequestNumber); 
        m_AccumulatedRequestNumber -= overTimeNumber;
        //std::cout << overTimeNumber << std::endl;
        for (auto& g : g_Generators)
        {
            for (size_t i = 0; i < overTimeNumber; i++)
            {
                std::string dataString = "id=" + std::to_string(g.Id) + "&timestamp=" + std::to_string(i) + "&value=" + std::to_string(Random::Float());
                //std::string send = requestTemplate + std::to_string(dataString.size() - 4) + "\r\n\r\n" + dataString;
                std::string send = requestTemplate + dataString;

                if (m_UseCustomRequestBody)
                    asio::write(g.Socket, asio::buffer(m_CustomRequestBody.data(), m_CustomRequestBody.size()), g_Ec);
                else
                    asio::write(g.Socket, asio::buffer(send.data(), send.size()), g_Ec);
                //g.Socket.write_some(asio::buffer(send.data(), send.size()), g_Ec);

                if (g_Ec)
                {
                    std::cout << g_Ec.message() << std::endl;
                }
            }
            //g.Flush();

            if (g_Ec)
            {
                m_ConsoleText = g_Ec.message();
            }
        }
    }

    {
        ImGui::Begin("Custom request");
        ImGui::AlignTextToFramePadding();

        ImGui::TextWrapped(m_ConsoleText.c_str());

        ImGui::End();
    }

    {
        ImGui::Begin("Custom data");
        ImGui::AlignTextToFramePadding();
        ImGui::Checkbox("Use custom request data", &m_UseCustomRequestBody);
        ImGui::InputTextMultiline("Input data", m_CustomRequestBody.data(), 4096);

        ImGui::End();
    }

}