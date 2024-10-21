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

	Matrix cameraToWorld = camera.CalculateCameraToWorld();

	// Convert angle to radians
	const float radFOV{ camera.fovAngle * PI / 180.f };

	// calculate FOV with radians NOT ANGLE
	const float FOV{ tan(radFOV / 2) };

	for (int px{}; px < m_Width; ++px)
	{
		const float aspectRatio{ float(m_Width) / float(m_Height) };
		
		for (int py{}; py < m_Height; ++py)
		{
			//BLACK GRADIENT BACKGROUND
			// 
			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;
			//
			//ColorRGB finalColor{ gradient, gradient, gradient };

			// store coordinates of pixels by using NDC
			rayDirection.x = ((2.f * (float(px) + 0.5f)/float(m_Width)) - 1.f) * aspectRatio * FOV;
			rayDirection.y = (1.f - 2.f * (float(py) + 0.5f)/float(m_Height)) * FOV;
			rayDirection.z = 1.f;
			
			// normalize ray Direction
			rayDirection.Normalize();
			
			// Transform raydirection with up, forward and right vector
			rayDirection = cameraToWorld.TransformVector(rayDirection);

			// TEST: test it out
			// 
			//Ray hitRay{ {}, rayDirection };
			//ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };

			// Ray we are casting from the camera towards each pixel
			Ray viewRay{ camera.origin, rayDirection };

			//color to write to the color buffer (Default = black)
			ColorRGB finalColor{};

			//hitrecord containing more information about a potential hit
			HitRecord closestHit{};
			pScene->GetClosestHit(viewRay, closestHit);

			// TEST: Make a testPlane to check if hittest_Plane works
			//
			//Plane testPlane{ {0.f, -50.f, 0.f}, {0.f, 1.f, 0.f}, 0 };
			//GeometryUtils::HitTest_Plane(testPlane, viewRay, closestHit);

			// TEST: Make a testsphere to check if hittest_Sphere works
			// 
			//Sphere testSphere{ {0.f, 0.f, 100.f}, 50.f, 0 };
			//GeometryUtils::HitTest_Sphere(testSphere, viewRay, closestHit);

			if (closestHit.didHit) {
				//finalColor = materials[closestHit.materialIndex]->Shade();

				for (uint16_t i{}; i < lights.size(); ++i) {

					Vector3 lightDirection{ LightUtils::GetDirectionToLight(lights[i], closestHit.origin) };
					Ray lightRay{};

					lightRay.origin = closestHit.origin + (closestHit.normal * 0.0001f);
					lightRay.direction = lightDirection.Normalized();
					lightRay.min = 0.00001f;
					lightRay.max = lightDirection.Normalize();

					
					if (pScene->DoesHit(lightRay) && m_ShadowsEnabled) {
						continue;
					}

					const float observedArea{ std::max(0.0f,Vector3::Dot(lightDirection.Normalized(), closestHit.normal)) };
					const ColorRGB radiance{ LightUtils::GetRadiance(lights[i], closestHit.origin) };
					const ColorRGB shade{ materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, -rayDirection) };

					switch (m_LightingMode)
					{
					case dae::Renderer::LightingMode::ObservedArea:
						finalColor += ColorRGB{ 1.f, 1.f, 1.f } * observedArea;
						break;
					case dae::Renderer::LightingMode::Radiance:
						finalColor += radiance;
						break;
					case dae::Renderer::LightingMode::BRDF:
						finalColor += shade;
						break;
					case dae::Renderer::LightingMode::Combined:
						finalColor += radiance  * shade * observedArea;
						break;
					default:
						break;
					}
				}

				// TEST: Test to see if the t_values are correct
				// 
				//const float scaled_t = (closestHit.t / 500.f);
				//finalColor = { scaled_t, scaled_t, scaled_t };
			}

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
