#pragma once

//class UEditor;
class FAppWindow;

/**
 * @brief Main Client Class
 * Application Entry Point, Manage Overall Execution Flow
 *
 * @var AcceleratorTable 키보드 단축키 테이블 핸들
 * @var MainMessage 윈도우 메시지 구조체
 * @var Window 윈도우 객체 포인터
 */
class FClientApp
{
public:
    int Run(HINSTANCE InInstanceHandle, int InCmdShow);

    // Special Member Function
    FClientApp();
    ~FClientApp();

private:
    int InitializeSystem() const;
    void UpdateSystem() const;
    void MainLoop();
	void ShutdownSystem() const;

    HACCEL AcceleratorTable;
    MSG MainMessage;
    FAppWindow* Window;
	//UEditor* Editor = nullptr;
};
