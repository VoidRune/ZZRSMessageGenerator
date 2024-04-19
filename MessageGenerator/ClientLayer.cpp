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
    Generator():
        Socket(g_Context)
    {

    }

    bool Connect(std::string addr, int32_t port)
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(addr, g_Ec), port);
        Socket.connect(endpoint, g_Ec);
        if (!g_Ec)
        {
            std::cout << "Connected generator!" << std::endl;
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

    std::vector<uint32_t> Ids;
    std::vector<uint32_t> MessageIds;
    std::vector<float> RandomValues;
    asio::ip::tcp::socket Socket;
};

std::vector<Generator> g_Generators;

void ClientLayer::SendData(int amountPerGenerator)
{
    if (amountPerGenerator == 0)
        return;

    static std::string requestTemplate =
        "POST /report HTTP/1.1\r\n"
        "Host: localhost:12345\r\n"
        "Accept: */*\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: ";

    for (auto& g : g_Generators)
    {
        for (size_t i = 0; i < amountPerGenerator; i++)
        {
            for (int j = 0; j < g.Ids.size(); j++)
            {

                auto now = std::chrono::system_clock::now();
                auto midnight = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch() % std::chrono::hours(24)
                    );
                g.RandomValues[j] += Random::Float();
                std::string dataString =
                    "id=" + std::to_string(g.Ids[j])
                    + "&count=" + std::to_string(g.MessageIds[j] + i)
                    + "&timestamp=" + std::to_string(midnight.count())
                    + "&value=" + std::to_string(g.RandomValues[j]);
                //std::cout << dataString << std::endl;
                std::string send = requestTemplate + std::to_string(dataString.size()) + "\r\n\r\n" + dataString;
                //std::string send = requestTemplate + dataString;

                asio::write(g.Socket, asio::buffer(send.data(), send.size()), g_Ec);
                //g.Socket.write_some(asio::buffer(send.data(), send.size()), g_Ec);

                if (g_Ec)
                {
                    std::cout << g_Ec.message() << std::endl;
                }
            }
        }
        for (auto& mId : g.MessageIds)
        {
            mId += amountPerGenerator;
        }

        if (g_Ec)
        {
            m_ConsoleText = g_Ec.message();
        }
    }
}

void ClientLayer::UpdateIds(int idsPerGenerator)
{
    for (size_t i = 0; i < g_Generators.size() * idsPerGenerator; i += idsPerGenerator)
    {
        std::vector<uint32_t> ids;
        for (size_t j = 0; j < idsPerGenerator; j++)
        {
            ids.push_back(i + j + m_GeneratorStartId);
        }

        Generator& gen = g_Generators[i / idsPerGenerator];
        gen.Ids = ids;
        gen.MessageIds.resize(ids.size());
        gen.RandomValues.resize(ids.size());
    }
}

void ClientLayer::OnAttach()
{
    Random::seed = time(nullptr);
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

        ImGui::Separator();

        ImGui::InputInt("Generator start id", &m_GeneratorStartId);

        ImGui::InputInt("Min Ids", &m_MinIdsPerGenerator);
        //ImGui::SameLine();
        ImGui::InputInt("Max Ids", &m_MaxIdsPerGenerator);
        ImGui::InputFloat("Time(minutes): Min -> Max", &m_TimeTakenToMaxId);

        if(m_IsTimerRunning)
            m_CurrentTimeTaken += dt / 60.0f;

        if (ImGui::Button("Start"))
        {
            m_IsTimerRunning = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            m_IsTimerRunning = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            m_CurrentTimeTaken = 0.0f;
        }
        if (m_CurrentTimeTaken > m_TimeTakenToMaxId)
            m_CurrentTimeTaken = m_TimeTakenToMaxId;

        if (m_TimeTakenToMaxId < 0.05)
            m_TimeTakenToMaxId = 0.05;

        float r = m_CurrentTimeTaken / m_TimeTakenToMaxId;
        int curr = (int)std::lerp(m_MinIdsPerGenerator, m_MaxIdsPerGenerator, r);
        if (curr != m_CurrentIdsPerGenerator)
        {
            m_CurrentIdsPerGenerator = curr;
            UpdateIds(m_CurrentIdsPerGenerator);
            //std::cout << "Updated " << m_CurrentIdsPerGenerator << std::endl;
            m_ConsoleText = "Updating ids per generator: " + std::to_string(m_CurrentIdsPerGenerator);
        }

        ImGui::SameLine();
        ImGui::Text(std::to_string(m_CurrentTimeTaken).c_str());

        ImGui::Separator();

        auto temp = std::string("Generators open: ") + std::to_string(g_Generators.size());
        ImGui::Text(temp.c_str());

        ImGui::InputInt("Generator count", &m_ConnectGeneratorCount);
        if (ImGui::Button("Connect generator"))
        {
            bool hasFailed = false;

            for (size_t i = 0; i < m_ConnectGeneratorCount * m_CurrentIdsPerGenerator; i += m_CurrentIdsPerGenerator)
            {
                Generator gen;
                if (gen.Connect(m_Address, m_Port))
                {
                    g_Generators.push_back(std::move(gen));
                    //m_ConsoleText = std::string("Successfully connected generator with id: ") + std::to_string(id);
                }
                else
                {
                    m_ConsoleText = "Failed to connect!";
                    hasFailed = true;
                    break;
                }
            }
            if (!hasFailed)
            {
                m_ConsoleText = std::string("Successfully connected ") + std::to_string(m_ConnectGeneratorCount) + " generators!";
                UpdateIds(m_CurrentIdsPerGenerator);
            }
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
            for (auto& g : g_Generators)
            {
                g.Socket.close();
            }
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
            auto start = std::chrono::high_resolution_clock::now();

            SendData(m_RequestCount);

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
        m_AccumulatedRequestNumber += m_RequestCount * dt;
        uint32_t overTimeNumber = std::floor(m_AccumulatedRequestNumber); 
        m_AccumulatedRequestNumber -= overTimeNumber;
       
        SendData(overTimeNumber);
    }

    {
        ImGui::Begin("Console");
        ImGui::AlignTextToFramePadding();

        ImGui::TextWrapped(m_ConsoleText.c_str());

        ImGui::End();
    }
}