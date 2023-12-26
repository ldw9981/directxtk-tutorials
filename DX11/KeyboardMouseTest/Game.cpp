//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

//#define ORBIT_STYLE

namespace
{
    const XMVECTORF32 START_POSITION = { 0.f, 0.f, 0.f, 0.f };
    const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };
    constexpr float ROTATION_GAIN = 0.004f;
    constexpr float MOVEMENT_GAIN = 0.07f;

    // Orbit-style
    constexpr float c_defaultPhi = XM_2PI / 6.0f;
    constexpr float c_defaultRadius = 3.3f;
    constexpr float c_minRadius = 0.1f;
    constexpr float c_maxRadius = 5.f;
}

Game::Game() noexcept(false) :
    // FPS-style
    m_pitch(0),
    m_yaw(0),
    m_cameraPos(START_POSITION),
    // Orbit-style
    m_theta(0.f),
    m_phi(c_defaultPhi),
    m_radius(c_defaultRadius),
    m_roomColor(Colors::White)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    AllocConsole();
    freopen("CONOUT$", "wb", stdout);//����� �ڵ鿡 ���� �ɼ��� �޶����ϴ�. �ɼ��� �Ʒ� ��ó Ȯ��.

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const&)
{
    // TODO: Add your game logic here.
    auto mouse = m_mouse->GetState();
    m_mouseButtons.Update(mouse);

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f)
            * ROTATION_GAIN;


        m_pitch += delta.y;
        m_yaw += delta.x;

    }

    m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

    auto kb = m_keyboard->GetState();

    m_keys.Update(kb);

    if (kb.Escape)
    {
        ExitGame();
    }

    if (kb.Home)
    {
        m_cameraPos = START_POSITION.v;
        m_yaw = m_pitch = 0.f;
    }

    Vector3 move = Vector3::Zero;


    float fowardScale = 0.0f;
    float rightScale = 0.0f;
    float upScale = 0.0f;
    if (kb.Up || kb.W)
        fowardScale = 1.f;

    if (kb.Down || kb.S)
        fowardScale = -1.f;

    if (kb.Left || kb.A)
        rightScale = -1.f;

    if (kb.Right || kb.D)
        rightScale = 1.f;

    if (kb.PageUp || kb.Space)
        upScale = 1.f;

    if (kb.PageDown || kb.X)
        upScale = -1.f;

    // ���� �ִ� ����-�ڻ��� ��¼�� �Լ��� ���溤�͸� ��� ����ȭ�� �ڵ�
    Vector3 forward, right;
    Matrix rotMatrix = Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
    forward = Vector3(rotMatrix._31, rotMatrix._32, rotMatrix._33);// -rotMatrix.Forward(); // Matrix�� ������ ��ǥ��� -�� �ٿ���� �Ѵ�.
    right = rotMatrix.Right();

    float speed = MOVEMENT_GAIN;       // �̵��ӵ�
    Vector3 worldDirection = forward * fowardScale + right * rightScale + Vector3::Up * upScale; // ȸ�����¿� Ű�� ����� ���忡���� �̵�����
    worldDirection.Normalize(); //  ���� ũ��1��  ����ȭ

    m_cameraPos = m_cameraPos + worldDirection * speed;   // ���麤��


    // ��ũ�� ����
    Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f))
        - Vector3(0.1f, 0.1f, 0.1f);

    // limit pitch to straight up or straight down
    constexpr float limit = XM_PIDIV2 - 0.01f;
    m_pitch = std::max(-limit, m_pitch);
    m_pitch = std::min(+limit, m_pitch);

    // keep longitude in sane range by wrapping
    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_2PI;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_2PI;
    }

    Vector3 lookAt = m_cameraPos + forward;//forward;   // ���麤��
    m_view = XMMatrixLookAtLH(m_cameraPos, lookAt, Vector3::Up);


    if (m_keys.pressed.Tab || m_mouseButtons.rightButton == Mouse::ButtonStateTracker::PRESSED)
    {
        if (m_roomColor == Colors::Red.v)
            m_roomColor = Colors::Green;
        else if (m_roomColor == Colors::Green.v)
            m_roomColor = Colors::Blue;
        else if (m_roomColor == Colors::Blue.v)
            m_roomColor = Colors::White;
        else
            m_roomColor = Colors::Red;
    }

    printf("pos: %f, %f, %f  ,   lookAt : %f, %f, %f \n", m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, lookAt.x, lookAt.y, lookAt.z);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
    context;

    // TODO: Add your rendering code here.
    Matrix world = Matrix::CreateTranslation(Vector3(0.f, 0.f, 20.f));
    m_room->Draw(world, m_view, m_proj, m_roomColor, m_roomTex.Get());

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
    m_keys.Reset();
    m_mouseButtons.Reset();
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_keys.Reset();
    m_mouseButtons.Reset();
    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    device;

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_room = GeometricPrimitive::CreateBox(context,
        XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]),
        false, true);

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"roomtexture.dds",
            nullptr, m_roomTex.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    auto size = m_deviceResources->GetOutputSize();
    m_proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(70.f), float(size.right) / float(size.bottom), 0.01f, 100.f);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_room.reset();
    m_roomTex.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
