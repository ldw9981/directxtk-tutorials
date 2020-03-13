//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // MSAA
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_offscreenRenderTarget;

    // Game state
    DX::StepTimer                                       m_timer;

    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;

#if 0
    using VertexType = DirectX::VertexPositionColor;
#else
    using VertexType = DirectX::VertexPositionTexture;
#endif

    std::unique_ptr<DirectX::BasicEffect> m_effect;
    std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
    std::unique_ptr<DirectX::CommonStates> m_states;

    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;

    enum Descriptors
    {
        Rocks,
        Count
    };

    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_proj;
};
