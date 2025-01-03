#pragma once
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"

#include <iostream>

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			////TODO W1
			//throw std::runtime_error("Not Implemented Yet");
			//return false;
			
			// discriminant = b^2 - 4ac
			// D > 0 -> intersecting on 2 points
			// D = 0 -> intersecting on one point
			// D < 0 -> not intersecting

			const Vector3 RayToSphere{ ray.origin - sphere.origin };

			const float a{ Vector3::Dot(ray.direction, ray.direction) };
			const float b{ 2.f * Vector3::Dot(ray.direction, RayToSphere) };
			const float c{ Vector3::Dot(RayToSphere,RayToSphere) - pow(sphere.radius, 2.f)};
			
			const float discriminant{ Square(b) - 4.f * a * c};
			
			if (discriminant > 0)
			{
				//todo ppt week 1 ->  p45 t < tmin
				const float t = (-b - sqrt(discriminant)) / (2.f * a);
				
				if (t < ray.max && t > ray.min && t < hitRecord.t) {
					hitRecord.t = t;
					hitRecord.origin = ray.origin + ray.direction * hitRecord.t;
					hitRecord.normal = (hitRecord.origin - sphere.origin) / sphere.radius;
					hitRecord.didHit = true;
					hitRecord.materialIndex = sphere.materialIndex;
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			////TODO W1
			//throw std::runtime_error("Not Implemented Yet");
			//return false;

			const Vector3 RayToOrigin{ plane.origin - ray.origin };

			const float t{ 
				(Vector3::Dot(RayToOrigin, plane.normal)) / 
				(Vector3::Dot(ray.direction, plane.normal)) };

			if (t < ray.max && t > ray.min && t < hitRecord.t) {
				hitRecord.t = t;
				hitRecord.origin = ray.origin + ray.direction * hitRecord.t;
				hitRecord.normal = plane.normal;
				hitRecord.didHit = true;
				hitRecord.materialIndex = plane.materialIndex;
				return true;
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			////todo W5
			//throw std::runtime_error("Not Implemented Yet");
			//return false;

			const float dot{ Vector3::Dot(triangle.normal, ray.direction) };
			
			if (triangle.cullMode == TriangleCullMode::BackFaceCulling && dot > 0.f)
			{
				return false;
			}
			if (triangle.cullMode == TriangleCullMode::FrontFaceCulling && dot < 0.f)
			{
				return false;
			}
			if (dot == 0.f)
			{
				return false;
			}
			
			const Vector3 L{triangle.v0 - ray.origin};
			const float t{ Vector3::Dot(L, triangle.normal) / dot };
			
			if (t < ray.min || t > ray.max) {
				return false;
			}
			
			const Vector3 P{ ray.origin + ray.direction * t };
			
			const std::vector<Vector3> vertices = { {triangle.v0}, {triangle.v1}, {triangle.v2} };

			for(uint16_t i{}; i < vertices.size(); ++i )
			{
				Vector3 e{};
				if (i < 2) {
					e = vertices[i + 1] - vertices[i];
				}
				else {
					e = vertices[0] - vertices[i];
				}
			
				Vector3 p{ P - vertices[i] };
			
				if (Vector3::Dot(Vector3::Cross(e, p), triangle.normal) < 0.f) {
					return false;
				}
			}
			
			hitRecord.t = t;
			hitRecord.origin = P;
			hitRecord.normal = triangle.normal;
			hitRecord.didHit = true;
			hitRecord.materialIndex = triangle.materialIndex;
			
			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool SlabTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray) {
			float tx1 = (mesh.transformedMinAABB.x - ray.origin.x) / ray.direction.x;
			float tx2 = (mesh.transformedMaxAABB.x - ray.origin.x) / ray.direction.x;

			float tmin = std::min(tx1, tx2);
			float tmax = std::max(tx1, tx2);

			float ty1 = (mesh.transformedMinAABB.y - ray.origin.y) / ray.direction.y;
			float ty2 = (mesh.transformedMaxAABB.y - ray.origin.y) / ray.direction.y;

			tmin = std::max(tmin, std::min(ty1, ty2));
			tmax = std::min(tmax, std::max(ty1, ty2));

			float tz1 = (mesh.transformedMinAABB.z - ray.origin.z) / ray.direction.z;
			float tz2 = (mesh.transformedMaxAABB.z - ray.origin.z) / ray.direction.z;

			tmin = std::max(tmin, std::min(tz1, tz2));
			tmax = std::min(tmax, std::max(tz1, tz2));

			return tmax > 0 && tmax >= tmin;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			//throw std::runtime_error("Not Implemented Yet");

			// slabtest
			if (!SlabTest_TriangleMesh(mesh, ray)) {
				return false;
			}

			Triangle triangle{};
			triangle.cullMode = mesh.cullMode;
			for (size_t i{}; i < mesh.indices.size(); i += 3)
			{
				HitRecord hitRec{};
				triangle.v0 = mesh.transformedPositions[mesh.indices[i]];
				triangle.v1 = mesh.transformedPositions[mesh.indices[i + 1]];
				triangle.v2 = mesh.transformedPositions[mesh.indices[i + 2]];

				triangle.normal = mesh.transformedNormals[i / 3];
				if (HitTest_Triangle(triangle, ray, hitRec, ignoreHitRecord))
				{
					if (hitRec.t < hitRecord.t)
					{
						hitRecord = hitRec;
						hitRecord.materialIndex = mesh.materialIndex;
						return true;
					}
				}
			}
			return false;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			////todo W3
			//throw std::runtime_error("Not Implemented Yet");
			//return {};

			switch (light.type)
			{
			case LightType::Point:
				return light.origin - origin;
				break;
			case LightType::Directional:
				return light.direction;
				break;
			default:
				return{};
				break;
			}
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			////todo W3
			//throw std::runtime_error("Not Implemented Yet");
			//return {};

			switch (light.type)
			{
			case LightType::Point:
				return light.color * (light.intensity / (light.origin - target).SqrMagnitude());
				break;
			case LightType::Directional:
				return light.color * light.intensity;
				break;
			default:
				return{};
				break;
			}
			
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}