#pragma once

#include "Siateczka1.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

class CSiateczka2 : public CSiateczka1
{
public:
	struct Vec2 {
		float x, y;
	};

	struct Triangle2 {
		Vec2 v1, v2, v3;
	};

	struct Hexagon {
		Vec2 vertices[6];
	};

	std::vector<Hexagon> generateHexagonalGrid(float width, float height, float a) {
		std::vector<Hexagon> hexagons;
		float hexHeight = a * std::sqrt(3);
		float horizSpacing = 2 * a;
		float vertSpacing = hexHeight;

		int rows = static_cast<int>(height / vertSpacing) + 2;
		int cols = static_cast<int>(width / horizSpacing) + 2;

		for (int row = 0; row < rows; row++) {
			float y = row * vertSpacing;
			for (int col = 0; col < cols; col++) {
				float x = col * horizSpacing + (row % 2 == 0 ? 0.0f : a);
				if (x <= width && y <= height) {
					Hexagon hex;
					for (int i = 0; i < 6; i++) {
						float angle = M_PI / 180.0f * (60.0f * i);
						hex.vertices[i] = { x + a * std::cos(angle), y + a * std::sin(angle) };
					}
					hexagons.push_back(hex);
				}
			}
		}

		return hexagons;
	}

	std::vector<Triangle2> subdivideHexagons(const std::vector<Hexagon>& hexagons, float a) {
		std::vector<Triangle2> triangles;
		for (const auto& hex : hexagons) {
			Vec2 center = { 0.0f, 0.0f };
			for (const auto& v : hex.vertices) {
				center.x += v.x;
				center.y += v.y;
			}
			center.x /= 6.0f;
			center.y /= 6.0f;

			for (int i = 0; i < 6; i++) {
				Vec2 v1 = hex.vertices[i];
				Vec2 v2 = hex.vertices[(i + 1) % 6];
				triangles.push_back({ center, v1, v2 });
			}
		}

		return triangles;
	}

	void createAdditionalTriangles(const std::vector<Hexagon>& hexagons, std::vector<Triangle2> &triangles, float a) {
		for (const auto& hex : hexagons) {
			{
				Vec2 v1 = hex.vertices[1];
				Vec2 v2 = hex.vertices[2];
				// Oblicz wektor normalny do krawêdzi v4-v5
				float dx = v2.x - v1.x;
				float dy = v2.y - v1.y;
				float length = std::sqrt(dx * dx + dy * dy);
				float nx = -dy / length;
				float ny = dx / length;
				// Œrodek krawêdzi
				float mx = (v1.x + v2.x) / 2.0f;
				float my = (v1.y + v2.y) / 2.0f;
				// Wierzcho³ek trójk¹ta na zewn¹trz
				Vec2 apex = { mx + nx * a * std::sqrt(3) / 2.0f, my - ny * a * std::sqrt(3) / 2.0f };
				triangles.push_back({ v2, v1, apex });
			}

			{
				Vec2 v4 = hex.vertices[4];
				Vec2 v5 = hex.vertices[5];
				// Oblicz wektor normalny do krawêdzi v4-v5
				float dx = v5.x - v4.x;
				float dy = v5.y - v4.y;
				float length = std::sqrt(dx * dx + dy * dy);
				float nx = -dy / length;
				float ny = dx / length;
				// Œrodek krawêdzi
				float mx = (v4.x + v5.x) / 2.0f;
				float my = (v4.y + v5.y) / 2.0f;
				// Wierzcho³ek trójk¹ta na zewn¹trz
				Vec2 apex = { mx + nx * a * std::sqrt(3) / 2.0f, my - ny * a * std::sqrt(3) / 2.0f };
				triangles.push_back({ v5, v4, apex });
			}
		}
	}

	std::vector<Triangle2> generateTriangularGrid2D(float width, float height, float a) {
		auto hexagons = generateHexagonalGrid(width, height, a);
		auto triangles = subdivideHexagons(hexagons, a);
		createAdditionalTriangles(hexagons, triangles, a);

		return triangles;
	}

	void removeZeroVertices(std::shared_ptr<CMesh> rzutnia);;


	void tworzMesh(std::shared_ptr<CMesh> rzutnia);

	void zbudujSiatke(std::shared_ptr<CModel3D> o);

	std::shared_ptr<CModel3D> daj_model();

	void daj_mi_wycisk(std::shared_ptr<CModel3D> o1, int mode=0);

	void zrob_dziure(std::shared_ptr<CModel3D> o1);

	CSiateczka2(unsigned int sizeXmm, unsigned int sizeYmm, unsigned int sizeZmm, unsigned int divider);


	~CSiateczka2() {
		//delete obj;
	}
};
