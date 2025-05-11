// Copyright 2009 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

class NamedPipeListener
{
public:
  NamedPipeListener(const std::string& pipe_name);
  ~NamedPipeListener();

  // Starts the pipe listener thread.
  void Start();

  // Stops the listener and joins the thread.
  void Stop();

  // Returns true if the listener is actively waiting for input.
  bool IsRunning() const;

  // Retrieves the next command received, if any.
  std::optional<std::string> GetNextCommand();

private:
  // Thread function that handles incoming pipe connections and reads.
  void ListenerThreadFunc();

  std::string m_pipe_name;
  std::thread m_thread;
  std::atomic<bool> m_running{false};

  std::mutex m_queue_mutex;
  std::queue<std::string> m_command_queue;
};

extern std::unique_ptr<NamedPipeListener> g_named_pipe_listener;
