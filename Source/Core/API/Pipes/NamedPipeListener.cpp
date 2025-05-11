// Copyright 2025 Your Company
// SPDX-License-Identifier: MIT

#include "NamedPipeListener.h"

#include <Windows.h>
#include <iostream>
#include <vector>

NamedPipeListener::NamedPipeListener(const std::string& pipe_name)
    : m_pipe_name("\\\\.\\pipe\\" + pipe_name)
{
}

NamedPipeListener::~NamedPipeListener()
{
  Stop();
}

void NamedPipeListener::Start()
{
  if (m_running)
    return;

  m_running = true;
  m_thread = std::thread(&NamedPipeListener::ListenerThreadFunc, this);
}

void NamedPipeListener::Stop()
{
  if (!m_running)
    return;

  m_running = false;

  // Create a dummy client connection to unblock ConnectNamedPipe if needed
  HANDLE dummy = CreateFileA(m_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                             OPEN_EXISTING, 0, nullptr);
  if (dummy != INVALID_HANDLE_VALUE)
    CloseHandle(dummy);

  if (m_thread.joinable())
    m_thread.join();
}

bool NamedPipeListener::IsRunning() const
{
  return m_running;
}

std::optional<std::string> NamedPipeListener::GetNextCommand()
{
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  if (m_command_queue.empty())
    return std::nullopt;

  std::string cmd = std::move(m_command_queue.front());
  m_command_queue.pop();
  return cmd;
}

void NamedPipeListener::ListenerThreadFunc()
{
  while (m_running)
  {
    HANDLE pipe = CreateNamedPipeA(m_pipe_name.c_str(),  // name of the pipe
                                   PIPE_ACCESS_INBOUND,  // read-only access
                                   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                   1,         // max instances
                                   0,         // out buffer
                                   4096,      // in buffer
                                   0,         // default timeout
                                   nullptr);  // default security

    if (pipe == INVALID_HANDLE_VALUE)
    {
      std::cerr << "[NamedPipeListener] Failed to create pipe\n";
      return;
    }

    BOOL connected =
        ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
      CloseHandle(pipe);
      continue;
    }

    std::vector<char> buffer(1024);
    DWORD bytesRead = 0;
    BOOL result =
        ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()) - 1, &bytesRead, nullptr);

    if (result && bytesRead > 0)
    {
      buffer[bytesRead] = '\0';
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      m_command_queue.emplace(buffer.data());
    }

    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
  }
}

std::unique_ptr<NamedPipeListener> g_named_pipe_listener;
