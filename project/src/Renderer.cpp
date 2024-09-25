//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	Vector3 rayDirection{};

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;
			//
			//ColorRGB finalColor{ gradient, gradient, gradient };
			//
			////Update Color in Buffer
			//finalColor.MaxToOne();
			//
			//m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
			//	static_cast<uint8_t>(finalColor.r * 255),
			//	static_cast<uint8_t>(finalColor.g * 255),
			//	static_cast<uint8_t>(finalColor.b * 255));


			//Calculate ray for each pixel on the screen w1
			// 1: calculate aspectratio -> screenwidth / screenheight
			float aspectRatio{ float(m_Width)/ float(m_Height) };

			// 2: store coordinates of pixels by using NDC
			rayDirection.x = ((2.f * (float(px) + 0.5f)/float(m_Width)) - 1.f) * aspectRatio;
			rayDirection.y = (1.f - 2.f * (float(py) + 0.5f)/float(m_Height));
			rayDirection.z = 1.f;
			
			// 3: normalize ray Direction
			rayDirection.Normalize();

			// test it out
			Ray hitRay{ {}, rayDirection };
			ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };

			finalColor.MaxToOne();
			
			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}
