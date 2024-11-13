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

#include "execution"
#define PARALLEL_EXECUTION

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
	Camera& camera  = pScene->GetCamera();
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();

	// Convert angle to radians
	const float radFOV{ camera.fovAngle * PI / 180.f };

	// calculate FOV with radians NOT ANGLE
	const float FOV{ tan(radFOV / 2) };

	const float aspectRatio{ float(m_Width) / float(m_Height) };

#if defined(PARALLEL_EXECUTION)
	//	Parallel logic
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
	std::vector<uint32_t> pixelIndices{};

	pixelIndices.reserve(amountOfPixels);
	for (uint32_t idx{}; idx < amountOfPixels; idx++) pixelIndices.emplace_back(idx);

	std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i) {
		RenderPixel(pScene, i, FOV, aspectRatio, cameraToWorld, camera.origin);
		});
#else
	// Synchronous logic (no threading)
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };

	for (uint16_t pixelIndex{}; pixelIndex < amountOfPixels; ++pixelIndex)
	{
		RenderPixel(pScene, pixelIndex, FOV, aspectRatio, cameraToWorld, camera.origin);
	}
#endif


	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Matrix cameraToWorld, const Vector3 cameraOrigin) const
{
	auto& materials{ pScene->GetMaterials() };
	auto& lights{ pScene->GetLights() };
	
	const uint32_t px{ pixelIndex % m_Width };
	const uint32_t py{ pixelIndex / m_Width };
	
	float rx{ px + 0.5f }, ry{ py + 0.5f };
	
	float cx{ (2.f * (rx / float(m_Width)) - 1.f) * aspectRatio * fov };
	float cy{ (1 - (2.f * (ry / float(m_Height)))) * fov};
	
	Vector3 rayDirection{cx, cy, 1.f};
	
	//// store coordinates of pixels by using NDC
	//rayDirection.x = ((2.f * (float(px) + 0.5f) / float(m_Width)) - 1.f) * aspectRatio * fov;
	//rayDirection.y = (1.f - 2.f * (float(py) + 0.5f) / float(m_Height)) * fov;
	//rayDirection.z = 1.f;
	
	// normalize ray Direction
	rayDirection.Normalize();
	
	// Transform raydirection with up, forward and right vector
	rayDirection = cameraToWorld.TransformVector(rayDirection);
	
	// Ray we are casting from the camera towards each pixel
	Ray viewRay{ cameraOrigin, rayDirection };
	
	//color to write to the color buffer (Default = black)
	ColorRGB finalColor{};
	
	//hitrecord containing more information about a potential hit
	HitRecord closestHit{};
	pScene->GetClosestHit(viewRay, closestHit);
	
	if (closestHit.didHit) {
	
		for (auto& light : lights) {
	
			Vector3 lightDirection{ LightUtils::GetDirectionToLight(light, closestHit.origin) };
			Ray lightRay{};
	
			lightRay.origin = closestHit.origin + (closestHit.normal * 0.0001f);
			lightRay.direction = lightDirection.Normalized();
			lightRay.min = 0.00001f;
			lightRay.max = lightDirection.Normalize();
	
	
			if (pScene->DoesHit(lightRay) && m_ShadowsEnabled) {
				continue;
			}
	
			const float observedArea{ std::max(0.0f,Vector3::Dot(lightDirection.Normalized(), closestHit.normal)) };
			const ColorRGB& radiance{ LightUtils::GetRadiance(light, closestHit.origin) };
			const ColorRGB& shade{ materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, -rayDirection) };
	
			switch (m_LightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				finalColor += colors::White * observedArea;
				break;
			case dae::Renderer::LightingMode::Radiance:
				finalColor += radiance;
				break;
			case dae::Renderer::LightingMode::BRDF:
				finalColor += shade;
				break;
			case dae::Renderer::LightingMode::Combined:
				finalColor += radiance * shade * observedArea;
				break;
			default:
				break;
			}
		}
	}
	
	finalColor.MaxToOne();
	
	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::CycleLightingMode()
{
	const int max{ 3 };
	const int reset{ 0 };

	if (static_cast<int>(m_LightingMode) < max)
	{
		m_LightingMode = static_cast<LightingMode>(static_cast<int>(m_LightingMode) + 1);
	}
	else {
		m_LightingMode = static_cast<LightingMode>(reset);
	}

	switch (m_LightingMode)
	{
	case dae::Renderer::LightingMode::ObservedArea:
		std::cout << "ObservedArea" << std::endl;
		break;
	case dae::Renderer::LightingMode::Radiance:
		std::cout << "Radiance" << std::endl;
		break;
	case dae::Renderer::LightingMode::BRDF:
		std::cout << "BRDF" << std::endl;
		break;
	case dae::Renderer::LightingMode::Combined:
		std::cout << "Combined" << std::endl;
		break;
	default:
		break;
	}
}
