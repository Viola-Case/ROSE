/**

  @file      ROSE_application.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_preamble.h>
#include <ROSE/Core/ROSE_input.h>

namespace ROSE {
  using AppID = size_t;

  class Application;

  static class ApplicationManager {
  public:
    ApplicationManager();
    ~ApplicationManager();
    void Close(AppID);
    void CloseAll();
    Application &GetApplication(AppID idx = 0);
    void LinkApplication(Application &app);
  private:

  };

  class Application {
    friend class ApplicationManager;
  //public:
  private:
    Application();
    Application(const wchar_t *);
    int Init();
    void Run();
    void CleanUp();
    
    WString m_name;
    ApplicationManager *m_parent;

    AppID m_id;

    bool m_ShouldClose;
  };
}