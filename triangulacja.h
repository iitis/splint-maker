#pragma once



const float epsilon = 1e-6f;
/* NOWE */

// Struktura reprezentuj¹ca punkt w 2D
struct Point2 {
	float u, v;
};

// Struktura reprezentuj¹ca trójk¹t w 2D
struct Triangle2 {
	Point2 p1, p2, p3;
};


// Oblicza iloczyn wektorowy dla trzech punktów (a, b, c)
// Wynik > 0 oznacza, ¿e (c) jest po lewej stronie wektora (a->b)
float cross2D(const Point2& a, const Point2& b, const Point2& c) {
	return (b.u - a.u) * (c.v - a.v) - (b.v - a.v) * (c.u - a.u);
}


// Sprawdza, czy punkt p le¿y wewn¹trz trójk¹ta utworzonego przez a, b, c.
// Metoda oparta na sprawdzeniu, czy wszystkie "podzialy" maj¹ ten sam znak.
bool isPointInsideTriangle(const Point2& p, const Point2& a, const Point2& b, const Point2& c) {
	float cp1 = cross2D(p, a, b);
	float cp2 = cross2D(p, b, c);
	float cp3 = cross2D(p, c, a);
	return (cp1 >= 0 && cp2 >= 0 && cp3 >= 0) || (cp1 <= 0 && cp2 <= 0 && cp3 <= 0);
}

#include "Vector3.h"

// Funkcja obliczaj¹ca normaln¹ trójk¹ta T (na przyk³adzie pierwszych trzech wierzcho³ków)
CVector3d ComputeNormal(const CTriangle& T) {
	CVector3d u(T.a, T.b);
	CVector3d v(T.a, T.c);
	CVector3d n = u.crossProduct(v);
	n.normalize();
	return n;
}

// Funkcja tworz¹ca lokalne osie na podstawie normalnej
void ComputeLocalAxes(const CVector3d& normal, CVector3d& axisU, CVector3d& axisV) {
	CVector3d arbitrary(1.0, 0.0, 0.0);
	if (std::fabs(normal.dotProduct(arbitrary)) > 0.99)
		arbitrary = CVector3d(0.0, 1.0, 0.0);
	axisU = normal.crossProduct(arbitrary);
	axisU.normalize();
	axisV = normal.crossProduct(axisU);
	axisV.normalize();
}

// Rzutowanie punktu 3D do 2D przy u¿yciu lokalnych osi i punktu pocz¹tkowego (origin)
Point2 ProjectPointTo2D(const CVertex& pt, const CVertex& origin,
	const CVector3d& axisU, const CVector3d& axisV) {
	CVector3d vec(pt.x - origin.x, pt.y - origin.y, pt.z - origin.z);
	Point2 proj;
	proj.u = vec.dotProduct(axisU);
	proj.v = vec.dotProduct(axisV);
	return proj;
}

// Odwrócenie rzutowania – przekszta³cenie punktu 2D z powrotem do przestrzeni 3D
CVertex UnprojectPoint(const Point2& pt2D, const CVertex& origin,
	const CVector3d& axisU, const CVector3d& axisV) {
	CVertex result;
	// Zak³adamy, ¿e CVertex ma pola x, y, z
	result.x = origin.x + pt2D.u * axisU.x + pt2D.v * axisV.x;
	result.y = origin.y + pt2D.u * axisU.y + pt2D.v * axisV.y;
	result.z = origin.z + pt2D.u * axisU.z + pt2D.v * axisV.z;
	return result;
}



std::vector<Triangle2> ear_clipping_triangulate2D(const std::vector<Point2>& inputPolygon) {
	std::vector<Triangle2> triangles;
	if (inputPolygon.size() < 3)
		return triangles; // Wielok¹t musi mieæ co najmniej 3 punkty

	// Robocza kopia wejœciowego wielok¹ta
	std::vector<Point2> poly = inputPolygon;

	// Upewnij siê, ¿e wielok¹t jest uporz¹dkowany przeciwnie do ruchu wskazówek zegara (CCW).
	// Obliczamy "podpisane" pole metod¹ shoelace.
	float area = 0.0f;
	int n = poly.size();
	for (int i = 0; i < n; i++) {
		int j = (i + 1) % n;
		area += poly[i].u * poly[j].v - poly[j].u * poly[i].v;
	}
	// Jeœli pole jest ujemne, wielok¹t jest uporz¹dkowany zgodnie z ruchem wskazówek zegara – odwracamy.
	if (area < 0)
		std::reverse(poly.begin(), poly.end());

	// G³ówna pêtla algorytmu ear clipping
	while (poly.size() > 3) {
		bool earFound = false;
		n = poly.size();
		for (int i = 0; i < n; i++) {
			int prev = (i - 1 + n) % n;
			int next = (i + 1) % n;
			const Point2& a = poly[prev];
			const Point2& b = poly[i];
			const Point2& c = poly[next];

			// Sprawdzamy, czy b jest wypuk³ym wierzcho³kiem.
			// Dla CCW iloczyn wektorowy powinien byæ dodatni.
			if (cross2D(a, b, c) <= 0)
				continue; // b nie jest wypuk³y – nie mo¿e byæ "uszkiem"

			// Sprawdzamy, czy ¿aden inny punkt nie le¿y wewn¹trz trójk¹ta (a, b, c).
			bool ear = true;
			for (int j = 0; j < n; j++) {
				if (j == prev || j == i || j == next)
					continue;
				if (isPointInsideTriangle(poly[j], a, b, c)) {
					ear = false;
					break;
				}
			}

			if (!ear)
				continue;

			// Znaleziono "uszek" – tworzymy trójk¹t i usuwamy œrodkowy wierzcho³ek b.
			Triangle2 tri;
			tri.p1 = a;
			tri.p2 = b;
			tri.p3 = c;
			triangles.push_back(tri);

			poly.erase(poly.begin() + i);
			earFound = true;
			break; // Zaktualizowaliœmy wielok¹t, zaczynamy od nowa.
		}
		if (!earFound) {
			// Jeœli nie znaleziono ¿adnego "uszka", wielok¹t mo¿e byæ zdegenerowany lub nie jest prosty.
			break;
		}
	}

	// Na koñcu pozostan¹ trzy punkty – ostatni trójk¹t.
	if (poly.size() == 3) {
		Triangle2 tri;
		tri.p1 = poly[0];
		tri.p2 = poly[1];
		tri.p3 = poly[2];
		triangles.push_back(tri);
	}

	return triangles;
}

// Przyk³adowa funkcja triangulacji 2D – mo¿na tu wykorzystaæ Twój algorytm ear clipping
std::vector<Triangle2> TriangulatePolygon2D(const std::vector<Point2>& polygon2D) {
	std::vector<Triangle2> tris2D;
	// Tu umieœæ algorytm ear clipping dla 2D, operuj¹cy na strukturze Point2D.
	// Dla uproszczenia zak³adamy, ¿e taka funkcja ju¿ istnieje.



	tris2D = ear_clipping_triangulate2D(polygon2D);

	return tris2D;
}

// Nowa funkcja, która trianguluje wielok¹t (podany w globalnych wspó³rzêdnych) przy u¿yciu rzutowania do 2D
std::vector<CTriangle> TriangulatePolygon3D(const std::vector<CVertex>& polygon, const CTriangle& T_ref) {
	std::vector<CTriangle> triangles3D;
	if (polygon.size() < 3)
		return triangles3D;

	// U¿ywamy T_ref do okreœlenia lokalnego uk³adu – mo¿na te¿ obliczyæ p³aszczyznê na podstawie samego polygonu.
	CVector3d normal = ComputeNormal(T_ref);
	CVector3d axisU, axisV;
	ComputeLocalAxes(normal, axisU, axisV);
	// U¿yjmy jako origin pierwszego wierzcho³ka referencyjnego
	CVertex origin = T_ref[0];

	// Rzutujemy wszystkie punkty wielok¹ta do 2D
	std::vector<Point2> polygon2D;
	for (const CVertex& pt : polygon) {
		polygon2D.push_back(ProjectPointTo2D(pt, origin, axisU, axisV));
	}

	// Triangulujemy wielok¹t w 2D
	std::vector<Triangle2> tris2D = TriangulatePolygon2D(polygon2D);

	// Odwracamy rzutowanie – ka¿dy trójk¹t 2D przekszta³camy z powrotem do 3D
	for (const Triangle2& tri2D : tris2D) {
		CTriangle tri3D;
		tri3D[0] = UnprojectPoint(tri2D.p1, origin, axisU, axisV);
		tri3D[1] = UnprojectPoint(tri2D.p2, origin, axisU, axisV);
		tri3D[2] = UnprojectPoint(tri2D.p3, origin, axisU, axisV);
		triangles3D.push_back(tri3D);
	}

	return triangles3D;
}

/* END NOWE */




// Funkcja ExtractFaces – zwraca listê face'ów, gdzie ka¿dy face to uporz¹dkowany wektor wierzcho³ków
std::vector<std::vector<CVertex>> ExtractFaces(const std::map<CVertex, std::vector<CVertex>>& graph)
{
	// Zbiór u¿ytych skierowanych krawêdzi: para (u, v) oznacza, ¿e krawêdŸ u->v zosta³a ju¿ u¿yta.
	std::set<std::pair<CVertex, CVertex>> usedEdges;
	std::vector<std::vector<CVertex>> faces;

	// Dla ka¿dego wierzcho³ka u i ka¿dego s¹siada v (krawêdŸ skierowana)
	for (const auto& pair : graph)
	{
		const CVertex& u = pair.first;
		const auto& neighbors = pair.second;
		for (const auto& v : neighbors)
		{
			// Jeœli krawêdŸ (u->v) ju¿ by³a u¿yta, pomijamy
			if (usedEdges.find({ u, v }) != usedEdges.end())
				continue;

			std::vector<CVertex> face;
			CVertex currentU = u;
			CVertex currentV = v;

			// Rozpoczynamy cykl – zachowujemy pierwsz¹ krawêdŸ
			std::pair<CVertex, CVertex> startEdge = { u, v };

			while (true)
			{
				usedEdges.insert({ currentU, currentV });
				face.push_back(currentU);

				// W wierzcho³ku currentV, znajdŸ pozycjê currentU na liœcie s¹siadów
				const auto& nbrs = graph.at(currentV);
				auto it = std::find(nbrs.begin(), nbrs.end(), currentU);
				if (it == nbrs.end())
				{
					// B³¹d – powinno siê znaleŸæ currentU, jeœli graf jest spójny.
					break;
				}

				// Wybieramy s¹siada tu¿ przed currentU (przy cyklicznym uporz¹dkowaniu)
				if (it == nbrs.begin())
					it = nbrs.end();
				--it;
				CVertex nextV = *it;

				// Przygotowujemy kolejn¹ krawêdŸ: currentV -> nextV
				currentU = currentV;
				currentV = nextV;

				// Jeœli wróciliœmy do pocz¹tku, cykl zamkniêty
				if (currentU == u && currentV == v)
				{
					break;
				}

				// Zabezpieczenie przed nieskoñczon¹ pêtl¹ – mo¿na dodaæ dodatkowy warunek.
			}

			// Dla spójnoœci, jeœli face nie zawiera ostatniego wierzcho³ka (który zamyka cykl), dodajemy go.
			if (!face.empty() && face.front() != face.back())
				face.push_back(face.front());

			// Dodajemy wyznaczony face do listy
			faces.push_back(face);
		}
	}

	return faces;
}



// Funkcja pomocnicza obliczaj¹ca k¹t (w radianach) miêdzy osi¹ x a wektorem (v - center)
float AngleFromCenter(const CVertex& center, const CVertex& v)
{
	// Mo¿emy za³o¿yæ, ¿e wszystkie punkty le¿¹ w p³aszczyŸnie okreœlonej przez trójk¹t T.
	return std::atan2(v.y - center.y, v.x - center.x);
}


// Funkcja sortuj¹ca listê s¹siadów w kolejnoœci rosn¹cego k¹ta (przeciwnie do ruchu wskazówek zegara)
void SortNeighbors(std::map<CVertex, std::vector<CVertex>>& graph)
{
	for (auto& pair : graph)
	{
		CVertex center = pair.first;
		auto& neighbors = pair.second;
		std::sort(neighbors.begin(), neighbors.end(),
			[&center](const CVertex& a, const CVertex& b)
			{
				return AngleFromCenter(center, a) < AngleFromCenter(center, b);
			});
	}
}


// Funkcja, która buduje graf (mapê wierzcho³ek -> lista s¹siadów) na podstawie segmentów
std::map<CVertex, std::vector<CVertex>> BuildGraph(
	const CTriangle& T,
	const std::set<std::pair<CVertex, CVertex>>& intersectionSegments)
{
	std::map<CVertex, std::vector<CVertex>> graph;

	// Dodaj oryginalne krawêdzie trójk¹ta
	graph[T[0]].push_back(T[1]);
	graph[T[1]].push_back(T[2]);
	graph[T[2]].push_back(T[0]);

	graph[T[1]].push_back(T[0]);
	graph[T[2]].push_back(T[1]);
	graph[T[0]].push_back(T[2]);

	// Dodaj segmenty przeciêcia
	for (const auto& seg : intersectionSegments)
	{
		const CVertex& A = seg.first;
		const CVertex& B = seg.second;
		graph[A].push_back(B);
		graph[B].push_back(A);
	}

	// Opcjonalnie: posortuj listy s¹siadów k¹towo wzglêdem œrodka (w celu póŸniejszej ekstrakcji face'ów)
	// ...

	return graph;
}



// Funkcja ComputePolygonArea oblicza pole wielok¹ta (face)
// przy u¿yciu metody Newella. Przyjmuje on uporz¹dkowany wektor
// wierzcho³ków polygon, reprezentuj¹cy cykl (w kolejnoœci, w jakiej
// tworz¹ wielok¹t).
double ComputePolygonArea(const std::vector<CVertex>& polygon)
{
	int n = polygon.size();
	if (n < 3)
		return 0.0; // nie jest to poprawny wielok¹t

	// Suma sk³adników do wyznaczenia wektora normalnego
	double nx = 0.0, ny = 0.0, nz = 0.0;
	for (int i = 0; i < n; i++)
	{
		const CVertex& current = polygon[i];
		const CVertex& next = polygon[(i + 1) % n];
		nx += (current.y - next.y) * (current.z + next.z);
		ny += (current.z - next.z) * (current.x + next.x);
		nz += (current.x - next.x) * (current.y + next.y);
	}

	// Pole wielok¹ta to 0.5 * d³ugoœæ wektora normalnego
	double area = 0.5 * std::sqrt(nx * nx + ny * ny + nz * nz);
	return area;
}


std::vector<std::vector<CVertex>> FilterInternalFaces(const std::vector<std::vector<CVertex>>& faces, const CTriangle& T)
{
	std::vector<std::vector<CVertex>> internalFaces;
	// Opcjonalnie: oblicz pole trójk¹ta T
	double areaT = ComputePolygonArea({ T[0], T[1], T[2] });

	qInfo() << "  triangle area: " << areaT;

	for (const auto& face : faces)
	{
		// Mo¿esz obliczyæ pole face'a:
		double faceArea = ComputePolygonArea(face);
		qInfo() << "    face area: " << faceArea;

		// Metoda 1: sprawdzenie orientacji (przyk³adowo, przyjmujemy, ¿e wewnêtrzne maj¹ dodatnie pole)
		// if (faceArea < 0) continue; // pomijamy face zewnêtrzny

		// Metoda 2: sprawdzenie, czy pole face’a jest bliskie polu T (czyli jest to face zewnêtrzny)
		// Mo¿esz u¿yæ pewnej tolerancji (np. 0.99 * areaT < faceArea < 1.01 * areaT)
		if (std::fabs(faceArea - areaT) < 1e-6)
			continue; // to face zewnêtrzny – pomijamy

		// Metoda 3: sprawdzenie po³o¿enia centroidu
		// CVertex centroid = ComputeCentroid(face);
		// if (!IsPointInsideTriangle(centroid, T)) continue;

		internalFaces.push_back(face);
	}

	return internalFaces;
}



float Distance(const CVertex& a, const CVertex& b) {
	return std::sqrt((a.x - b.x) * (a.x - b.x) +
		(a.y - b.y) * (a.y - b.y) +
		(a.z - b.z) * (a.z - b.z));
}


std::vector<CVertex> RemoveDuplicatePoints(const std::vector<CVertex>& points, float epsilon) {
	std::vector<CVertex> sorted_points = points;
	std::sort(sorted_points.begin(), sorted_points.end());

	std::vector<CVertex> new_points;
	for (size_t i = 0; i < sorted_points.size(); ++i) {
		bool is_duplicate = false;
		for (size_t j = 0; j < new_points.size(); ++j) {
			if (Distance(sorted_points[i], new_points[j]) < epsilon) {
				is_duplicate = true;
				break;
			}
		}
		if (!is_duplicate) {
			new_points.push_back(sorted_points[i]);
		}
	}

	return new_points;
}


std::vector<CTriangle> SubdivideTriangle(const CTriangle& T,
	const std::set<std::pair<CVertex, CVertex>>& intersectionSegments)
{
	std::vector<CTriangle> subdividedTriangles;

	// Krok 1: Zbierz wszystkie wierzcho³ki
	std::vector<CVertex> allPoints = { T[0], T[1], T[2] };

	for (const auto& seg : intersectionSegments)
	{
		allPoints.push_back(seg.first);
		allPoints.push_back(seg.second);
	}

	allPoints = RemoveDuplicatePoints(allPoints, 1e-6f);


	// Krok 2: Budujemy graf – wierzcho³ki i krawêdzie (segmenty)
	std::map<CVertex, std::vector<CVertex>> graph = BuildGraph(T, intersectionSegments);

	// Krok 2a: Sortujemy listy s¹siadów dla ka¿dego wierzcho³ka
	SortNeighbors(graph);

	qInfo() << "  graph size: " << graph.size();

	if (graph.size() > 3) {

		for (auto g : graph) {
			std::cout << "    ( " << g.first.x << ", " << g.first.y << ", " << g.first.z << " ) --> ";
			std::cout << "no. neighbours: " << g.second.size() << "\n";
		}


		// Krok 3: Ekstrakcja face'ów z grafu
		std::vector<std::vector<CVertex>> faces = ExtractFaces(graph);

		//qInfo() << "  has faces: " << faces.size();

		faces = FilterInternalFaces(faces, T);


		//qInfo() << "  filtered faces: " << faces.size();

		// Krok 4: Ka¿dy face (wielok¹t) triangulujemy, o ile nie jest ju¿ trójk¹tem
		for (const auto& face : faces) {
			if (face.size() < 3)
				continue; // pomijamy nieprawid³owe lub zdegenerowane face'y

			if (face.size() == 3) {
				CTriangle tri;
				tri[0] = face[0];
				tri[1] = face[1];
				tri[2] = face[2];
				subdividedTriangles.push_back(tri);
			}
			else {
				// U¿ywamy nowej funkcji trianguluj¹cej 3D z rzutowaniem do lokalnego uk³adu
				std::vector<CTriangle> tris = TriangulatePolygon3D(face, T);
				subdividedTriangles.insert(subdividedTriangles.end(), tris.begin(), tris.end());
			}
		}



		qInfo() << "  subdivided triangles: " << subdividedTriangles.size();

		for (auto t : subdividedTriangles) {
			std::cout << "    ( " << t.a.toRowVector3() << ", " << t.b.toRowVector3() << ", " << t.c.toRowVector3() << " )\n";
		}

	}

	return subdividedTriangles;
}



// Funkcja porównuj¹ca punkty wzglêdem ich projekcji na wektor 'dir'.
bool CompareAlongDirection(const CVertex& p1, const CVertex& p2, const CVector3d& dir)
{
	float proj1 = p1.x * dir.x + p1.y * dir.y + p1.z * dir.z;
	float proj2 = p2.x * dir.x + p2.y * dir.y + p2.z * dir.z;
	return proj1 < proj2;
}




bool IntersectTriangles3D(
	const CTriangle& _T1,
	const CTriangle& _T2,
	std::set<std::pair<CVertex, CVertex>>& intersectionSegments)
{
	CTriangle T1(_T1);
	CTriangle T2(_T2);

	std::vector<CVertex> intersections;

	// Sprawdzamy ka¿d¹ krawêdŸ T1, czy przecina T2
	for (int i = 0; i < 3; i++) {
		const CVertex& A = T1[i];
		const CVertex& B = T1[(i + 1) % 3];

		// Kierunek krawêdzi
		CVector3d dir(A, B);
		CPoint3d point;

		if (T2.hit(A, dir, point)) {
			intersections.push_back(point);
		}
	}

	// Sprawdzamy ka¿d¹ krawêdŸ T2, czy przecina T1
	for (int i = 0; i < 3; i++) {
		const CVertex& A = T2[i];
		const CVertex& B = T2[(i + 1) % 3];

		CVector3d dir(A, B);
		CPoint3d point;

		if (T1.hit(A, dir, point)) {
			intersections.push_back(point);
		}
	}

	// Usuwamy duplikaty
	std::vector<CVertex> uniquePoints = RemoveDuplicatePoints(intersections, epsilon);

	// Jeœli mamy mniej ni¿ dwa punkty, mo¿e to oznaczaæ przypadek brzegowy:
	// np. przeciêcie w jednym punkcie, trójk¹ty s¹ wspó³p³aszczyznowe, lub jedna krawêdŸ le¿y na powierzchni drugiej.

	if (uniquePoints.size() < 2)
	{
		//if (AreTrianglesCoplanar(T1, T2))
		//{
		//	// Rozwi¹zanie dla trójk¹tów wspó³p³aszczyznowych
		//	return IntersectCoplanarTriangles(T1, T2, intersectionSegments);
		//}
		// Mo¿na te¿ dodaæ dodatkow¹ obs³ugê, np. gdy mamy tylko punkt wspólny lub wspóln¹ krawêdŸ.
		return false;
	}

	// Jeœli mamy dok³adnie dwa punkty, przeciêcie jest odcinkiem.
	if (uniquePoints.size() == 2)
	{
		intersectionSegments.insert({ uniquePoints[0], uniquePoints[1] });
		return true;
	}

	// Jeœli mamy wiêcej ni¿ dwa punkty, oznacza to, ¿e przeciêcie ma bardziej z³o¿ony kszta³t.
	// Sortujemy punkty wed³ug ich projekcji na kierunek wyznaczony np. przez pierwszy segment przeciêcia.
	CVector3d dir(uniquePoints[0], uniquePoints[1]);
	dir.normalize();

	std::sort(uniquePoints.begin(), uniquePoints.end(),
		[dir](const CVertex& p1, const CVertex& p2)
		{
			return CompareAlongDirection(p1, p2, dir);
		});

	// £¹czymy punkty kolejnymi segmentami
	for (size_t i = 0; i < uniquePoints.size() - 1; i++)
	{
		intersectionSegments.insert({ uniquePoints[i], uniquePoints[i + 1] });
	}

	return true;
}


std::vector<CTriangle> SplitTriangle2(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2, const std::pair<INDEX_TYPE, std::set<INDEX_TYPE>*>& crossed_face, CAnnotationEdges& ed) {
	CTriangle T(crossed_face.first, mesh1.get());

	std::set<std::pair<CVertex, CVertex>> intersectionSegments;
	intersectionSegments.clear();

	for (INDEX_TYPE i : *crossed_face.second) {
		IntersectTriangles3D(T, CTriangle(i, mesh2.get()), intersectionSegments);
	}

	qInfo() << "Trojkat: " << crossed_face.first << ", Segmenty: " << intersectionSegments.size();


	// TERAZ BEDZIE OSTRE CIÊCIE

	std::vector<CTriangle> nowe_trojkaty;

	nowe_trojkaty = SubdivideTriangle(T, intersectionSegments);


	//std::set<CVertex> cpts;

	for (auto s : intersectionSegments) {
		ed.addEdge(s.first, s.second);

		//	cpts.insert(s.first);
		//	cpts.insert(s.second);
	}

	//SubdivideTriangle(T, std::vector<CVertex>(cpts.begin(),cpts.end()), nowe_trojkaty);


	return nowe_trojkaty;
}


void SplitTriangles2(std::shared_ptr<CMesh> mesh1, std::shared_ptr<CMesh> mesh2, const std::map<INDEX_TYPE, std::set<INDEX_TYPE>*>& crossed_faces, CAnnotationEdges& ed)
{
	std::shared_ptr<CMesh> mesh3 = std::make_shared<CMesh>();

	std::vector<CTriangle> new_Ts;

	for (const auto& f : crossed_faces) {
		new_Ts = SplitTriangle2(mesh1, mesh2, f, ed);


		for (auto new_T : new_Ts) {
			int idx = mesh3->vertices().size();
			mesh3->addVertex(new_T.a);
			mesh3->addVertex(new_T.b);
			mesh3->addVertex(new_T.c);
			mesh3->addFace(idx, idx + 1, idx + 2);
		}
		//for (auto new_T : new_Ts) {
		//	int idx = mesh1->vertices().size();
		//	mesh1->addVertex(new_T.a);
		//	mesh1->addVertex(new_T.b);
		//	mesh1->addVertex(new_T.c);
		//	mesh1->addFace(idx, idx + 1, idx + 2);
		//}
	}

	mesh3->setLabel("ZNALEZIONE");
	AP::OBJECT::addChild(mesh1->getParentPtr(), mesh3);

	//CModel3D* szczeka_obj = new CModel3D();
	//szczeka_obj->addChild(mesh3);
	//szczeka_obj->importChildrenGeometry();
	//szczeka_obj->setTransform(mesh1->getGlobalTransformationMatrix());
	//szczeka_obj->setLabel("ZNALEZIONE");

	//AP::WORKSPACE::addObject(szczeka_obj);
}

